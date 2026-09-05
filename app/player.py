#!/usr/bin/env python3
"""Playing a song that is already in memory, and knowing when it has finished.

The renderer hands back an array of samples. This hands that array to the
sound card. Nothing goes to disk on the way: a temporary WAV was never
anything but a way of reaching a player that could only open files, and it
cost a copy of every render, a file the next render could not overwrite while
it was still open, and a round trip through the filesystem on every press of
the space bar. No sequencer plays a song by saving it first, and this one no
longer does either.

Windows is spoken to through waveOut, which takes a block of PCM and plays it.
The whole song goes in as one block -- it is already in memory, in full, so
there is nothing to stream and no way to starve the card -- and the header it
was queued with raises a flag when the block has finished, which is what lets
one key both start the song and stop it.

The Mac is spoken to through AudioQueue, in the same shape: one buffer,
enqueued whole, started, and timed.

Neither needs anything installed: both are ctypes calls into a library the
system already has. Where neither can be opened there is a fallback that does
write a file and hand it to whatever will play one, because silence with no
explanation is worse than a temporary file.
"""
import ctypes
import os
import subprocess
import sys
import tempfile
import time

import numpy as np

STOPPED, PLAYING = 'stopped', 'playing'

#: what the engine renders at, and so what the card is opened at
SAMPLE_RATE = 44100


def to_pcm(samples):
    """Samples to (bytes, channels): interleaved little-endian 16-bit.

    One column is mono and two are stereo, the same shape `write_wav` takes,
    and the frames of an (n, 2) array are already left-then-right in memory --
    which is the order a sound card wants them in, so the conversion is a
    clip, a scale and a cast.
    """
    y = np.ascontiguousarray(np.asarray(samples, dtype=np.float32))
    channels = 1 if y.ndim == 1 else y.shape[1]
    pcm = (np.clip(y, -1.0, 1.0) * 32767).astype('<i2')
    return pcm.tobytes(), channels


# -- Windows ---------------------------------------------------------------

WAVE_MAPPER = 0xFFFFFFFF
WAVE_FORMAT_PCM = 1
CALLBACK_NULL = 0
WHDR_DONE = 0x00000001
TIME_SAMPLES = 0x0002


class MMTIME(ctypes.Structure):
    """waveOutGetPosition's answer. The union is eight bytes whatever is in
    it, and for TIME_SAMPLES the first four of them are the sample count."""

    _fields_ = [('wType', ctypes.c_uint), ('u', ctypes.c_ubyte * 8)]


class WAVEFORMATEX(ctypes.Structure):
    _fields_ = [('wFormatTag', ctypes.c_ushort),
                ('nChannels', ctypes.c_ushort),
                ('nSamplesPerSec', ctypes.c_uint),
                ('nAvgBytesPerSec', ctypes.c_uint),
                ('nBlockAlign', ctypes.c_ushort),
                ('wBitsPerSample', ctypes.c_ushort),
                ('cbSize', ctypes.c_ushort)]


class WAVEHDR(ctypes.Structure):
    pass


WAVEHDR._fields_ = [('lpData', ctypes.c_void_p),
                    ('dwBufferLength', ctypes.c_uint),
                    ('dwBytesRecorded', ctypes.c_uint),
                    ('dwUser', ctypes.c_void_p),
                    ('dwFlags', ctypes.c_uint),
                    ('dwLoops', ctypes.c_uint),
                    ('lpNext', ctypes.POINTER(WAVEHDR)),
                    ('reserved', ctypes.c_void_p)]


class WaveOut(object):
    """One block of PCM on the default output device.

    The device is opened for the song and closed when the song ends or is
    stopped, rather than held open for the life of the program: an editor that
    is being typed in has no business owning the sound card.
    """

    def __init__(self):
        self._mm = ctypes.windll.winmm
        # spelled out rather than left to ctypes to guess: a handle is a
        # pointer and the device id is unsigned, and neither is an int on
        # every build this runs on
        hdr = ctypes.POINTER(WAVEHDR)
        self._mm.waveOutOpen.argtypes = [
            ctypes.POINTER(ctypes.c_void_p), ctypes.c_uint,
            ctypes.POINTER(WAVEFORMATEX), ctypes.c_void_p, ctypes.c_void_p,
            ctypes.c_uint]
        for name in ('waveOutPrepareHeader', 'waveOutUnprepareHeader',
                     'waveOutWrite'):
            getattr(self._mm, name).argtypes = [ctypes.c_void_p, hdr,
                                                ctypes.c_uint]
        for name in ('waveOutReset', 'waveOutClose'):
            getattr(self._mm, name).argtypes = [ctypes.c_void_p]
        self._mm.waveOutGetPosition.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(MMTIME), ctypes.c_uint]
        self._h = None
        self._hdr = None
        self._buf = None

    def play(self, data, channels, rate):
        self.stop()
        fmt = WAVEFORMATEX(WAVE_FORMAT_PCM, channels, rate,
                           rate * channels * 2, channels * 2, 16, 0)
        h = ctypes.c_void_p()
        if self._mm.waveOutOpen(ctypes.byref(h), WAVE_MAPPER,
                                ctypes.byref(fmt), 0, 0, CALLBACK_NULL):
            return False
        # the card reads out of this buffer for as long as it is playing, so
        # it is held here and not let go of until after waveOutReset
        buf = ctypes.create_string_buffer(data)
        hdr = WAVEHDR()
        hdr.lpData = ctypes.cast(buf, ctypes.c_void_p)
        hdr.dwBufferLength = len(data)
        size = ctypes.sizeof(hdr)
        if self._mm.waveOutPrepareHeader(h, ctypes.byref(hdr), size):
            self._mm.waveOutClose(h)
            return False
        if self._mm.waveOutWrite(h, ctypes.byref(hdr), size):
            self._mm.waveOutUnprepareHeader(h, ctypes.byref(hdr), size)
            self._mm.waveOutClose(h)
            return False
        self._h, self._hdr, self._buf = h, hdr, buf
        return True

    def playing(self):
        if self._h is None:
            return False
        if self._hdr.dwFlags & WHDR_DONE:
            self.stop()               # finished: give the device back
            return False
        return True

    def position(self):
        """How many frames have been played, straight from the device."""
        if self._h is None:
            return 0
        mm = MMTIME()
        mm.wType = TIME_SAMPLES
        if self._mm.waveOutGetPosition(self._h, ctypes.byref(mm),
                                       ctypes.sizeof(mm)):
            return 0
        if mm.wType != TIME_SAMPLES:      # the device offered another unit
            return 0
        return int.from_bytes(bytes(mm.u)[:4], 'little')

    def stop(self):
        if self._h is None:
            return
        # kept on the stack until the device is closed: dropping the buffer
        # while the card is still reading out of it is a crash, not an error
        h, hdr, buf = self._h, self._hdr, self._buf
        self._h = self._hdr = self._buf = None
        size = ctypes.sizeof(hdr)
        try:
            self._mm.waveOutReset(h)          # a queued block must be taken
            self._mm.waveOutUnprepareHeader(  # back before it can be freed
                h, ctypes.byref(hdr), size)
        finally:
            self._mm.waveOutClose(h)
        del buf


# -- the Mac ---------------------------------------------------------------

AUDIO_TOOLBOX = ('/System/Library/Frameworks/AudioToolbox.framework'
                 '/AudioToolbox')

LINEAR_PCM = 0x6C70636D                     # 'lpcm'
#: signed integer, packed
PCM_FLAGS = 0x4 | 0x8


class ASBD(ctypes.Structure):
    """AudioStreamBasicDescription: what the samples are."""

    _fields_ = [('mSampleRate', ctypes.c_double),
                ('mFormatID', ctypes.c_uint),
                ('mFormatFlags', ctypes.c_uint),
                ('mBytesPerPacket', ctypes.c_uint),
                ('mFramesPerPacket', ctypes.c_uint),
                ('mBytesPerFrame', ctypes.c_uint),
                ('mChannelsPerFrame', ctypes.c_uint),
                ('mBitsPerChannel', ctypes.c_uint),
                ('mReserved', ctypes.c_uint)]


class AudioQueueBuffer(ctypes.Structure):
    _fields_ = [('mAudioDataBytesCapacity', ctypes.c_uint),
                ('mAudioData', ctypes.c_void_p),
                ('mAudioDataByteSize', ctypes.c_uint),
                ('mUserData', ctypes.c_void_p),
                ('mPacketDescriptionCapacity', ctypes.c_uint),
                ('mPacketDescriptions', ctypes.c_void_p),
                ('mPacketDescriptionCount', ctypes.c_uint)]


CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_void_p,
                            ctypes.POINTER(AudioQueueBuffer))


class AudioQueue(object):
    """The same idea through AudioToolbox: one buffer, enqueued and started.

    The queue wants a callback for the moment a buffer has been consumed, so
    it is given one, but the callback fires when the queue has finished
    reading the samples rather than when the speaker has finished making the
    sound. The length of the audio is what says whether it is still playing.
    """

    def __init__(self):
        self._at = ctypes.cdll.LoadLibrary(AUDIO_TOOLBOX)
        #: held for as long as the queue may call it
        self._cb = CALLBACK(lambda user, queue, buf: None)
        self._q = None
        self._ends = 0.0
        self._began = 0.0
        self._rate = SAMPLE_RATE

    def play(self, data, channels, rate):
        self.stop()
        frame = channels * 2
        fmt = ASBD(float(rate), LINEAR_PCM, PCM_FLAGS, frame, 1, frame,
                   channels, 16, 0)
        q = ctypes.c_void_p()
        if self._at.AudioQueueNewOutput(ctypes.byref(fmt), self._cb, None,
                                        None, None, 0, ctypes.byref(q)):
            return False
        buf = ctypes.POINTER(AudioQueueBuffer)()
        if self._at.AudioQueueAllocateBuffer(q, len(data), ctypes.byref(buf)):
            self._at.AudioQueueDispose(q, True)
            return False
        ctypes.memmove(buf.contents.mAudioData, data, len(data))
        buf.contents.mAudioDataByteSize = len(data)
        if (self._at.AudioQueueEnqueueBuffer(q, buf, 0, None)
                or self._at.AudioQueueStart(q, None)):
            self._at.AudioQueueDispose(q, True)
            return False
        self._q = q
        self._began = time.monotonic()
        self._rate = rate
        self._ends = self._began + len(data) / float(rate * frame)
        return True

    def playing(self):
        if self._q is None:
            return False
        if time.monotonic() >= self._ends:
            self.stop()
            return False
        return True

    def position(self):
        """Off the clock rather than the queue: near enough for a keypress."""
        if self._q is None:
            return 0
        return int(max(0.0, time.monotonic() - self._began) * self._rate)

    def stop(self):
        if self._q is None:
            return
        q, self._q = self._q, None
        self._at.AudioQueueStop(q, True)
        self._at.AudioQueueDispose(q, True)


# -- anywhere else ---------------------------------------------------------

class FilePlayer(object):
    """The old way, kept for machines neither of the above can open.

    This is the one path that still writes a file. It is a fallback and not
    the plan: it exists so that a machine with an output device this program
    does not know how to talk to still makes a sound.
    """

    def __init__(self):
        self._proc = None
        self._path = None
        self._ends = 0.0
        self._began = 0.0
        self._rate = SAMPLE_RATE

    def play(self, data, channels, rate):
        from ppc.render import write_wav
        self.stop()
        y = (np.frombuffer(data, '<i2').astype(np.float32) / 32767.0)
        if channels > 1:
            y = y.reshape(-1, channels)
        path = os.path.join(tempfile.gettempdir(), 'vw_playback.wav')
        try:
            write_wav(path, y, rate)
        except OSError:
            return False
        self._path = path
        self._began = time.monotonic()
        self._rate = rate
        self._ends = self._began + len(data) / float(rate * channels * 2)
        if sys.platform == 'darwin':
            self._proc = subprocess.Popen(['afplay', path])
            return True
        return False

    def playing(self):
        if self._proc is not None:
            return self._proc.poll() is None
        return False

    def position(self):
        if self._proc is None:
            return 0
        return int(max(0.0, time.monotonic() - self._began) * self._rate)

    def stop(self):
        if self._proc is None:
            return
        try:
            self._proc.terminate()
        except Exception:                                    # noqa: BLE001
            pass
        self._proc = None


def open_output():
    """Whichever of the three this machine can actually use."""
    for backend in (WaveOut if sys.platform == 'win32' else None,
                    AudioQueue if sys.platform == 'darwin' else None,
                    FilePlayer):
        if backend is None:
            continue
        try:
            return backend()
        except Exception:                                    # noqa: BLE001
            continue
    return None


class Player(object):
    """One song at a time: play it, stop it, ask what it is doing."""

    def __init__(self):
        self._out = open_output()

    def play(self, samples, rate=SAMPLE_RATE):
        """Start playing `samples`. True if it is playing.

        The samples are float, one column or two, exactly as the renderer
        produced them; they are not held on to after this returns.
        """
        if self._out is None:
            return False
        data, channels = to_pcm(samples)
        if not len(data):
            return False
        try:
            return bool(self._out.play(data, channels, rate))
        except Exception:                                    # noqa: BLE001
            return False

    def stop(self):
        if self._out is None:
            return
        try:
            self._out.stop()
        except Exception:                                    # noqa: BLE001
            pass

    def position(self):
        """How far into the sound it has got, in frames. 0 when stopped.

        This is what stopping needs: the reverb that is still ringing depends
        on how much of the song was actually sung before the key was pressed.
        """
        if self._out is None:
            return 0
        try:
            return int(self._out.position())
        except Exception:                                    # noqa: BLE001
            return 0

    def state(self):
        """Whether a sound is playing, so one key can start and stop it."""
        if self._out is None:
            return STOPPED
        try:
            return PLAYING if self._out.playing() else STOPPED
        except Exception:                                    # noqa: BLE001
            return STOPPED
