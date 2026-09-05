#!/usr/bin/env python3
"""Playing a song that is already in memory, and knowing when it has finished.

The renderer hands back an array of samples. This hands that array to the
sound card. Nothing goes to disk on the way: a temporary WAV was never
anything but a way of reaching a player that could only open files, and it
cost a copy of every render, a file the next render could not overwrite while
it was still open, and a round trip through the filesystem on every press of
the space bar. No sequencer plays a song by saving it first, and this one no
longer does either.

Windows is spoken to through waveOut, a chunk at a time. Not because the song
has to be streamed -- it is all in memory, in full -- but because what plays
next has to be changeable without stopping what is playing now. Stopping the
singing is not stopping the sound: the room goes on ringing, and the card has
to be handed that tail without ever running dry. Twenty milliseconds a chunk
with three of them in the card's hands, so a stop takes effect within sixty
milliseconds, and sixty is long enough to work out what the room is still
doing and follow straight on with it.

The Mac is spoken to through AudioQueue, which is handed the whole buffer, and
what follows a stop is started after it rather than spliced onto it -- there
is a small gap there that Windows does not have.

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
import threading
import time

import numpy as np

STOPPED, PLAYING = 'stopped', 'playing'

#: what the engine renders at, and so what the card is opened at
SAMPLE_RATE = 44100

#: A chunk, and how many of them are kept in the card's hands. Multiplied
#: together they are the lag between pressing stop and the singing stopping,
#: so they are small; each on its own has to be big enough that the thread
#: doing the feeding is never late with the next one.
CHUNK_FRAMES = 882                      # 20 ms at 44100
CHUNKS_AHEAD = 3                        # 60 ms in the card's hands


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


class Chunk(object):
    """One header and the memory the card reads it out of.

    Both are held for as long as the device is open. Letting go of the memory
    while the card is still reading out of it is a crash rather than an error,
    so the header remembers whether it is out on loan.
    """

    def __init__(self, nbytes):
        self.buf = ctypes.create_string_buffer(nbytes)
        self.hdr = WAVEHDR()
        self.hdr.lpData = ctypes.cast(self.buf, ctypes.c_void_p)
        self.lent = False


class WaveOut(object):
    """The default output device, fed a chunk at a time by a thread of its own.

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
        self._lock = threading.RLock()
        self._h = None
        self._feeder = None
        self._quit = False
        self._chunks = []
        self._channels = 1
        self._rate = SAMPLE_RATE
        #: what is being read, and how far into it the card has been fed
        self._src = None
        self._pos = 0
        #: frames of the song handed over altogether, which is where a stop
        #: has to take effect: everything up to here is already the card's
        self._queued = 0
        self._singing = True
        self._stop_at = None
        self._follow = None
        #: silence fed while waiting for something to follow with, so that
        #: what does arrive is picked up where it had got to by then
        self._owed = 0

    # -- what the window calls --------------------------------------------

    def play(self, samples, rate):
        self.stop()
        y = np.ascontiguousarray(np.asarray(samples, dtype=np.float32))
        channels = 1 if y.ndim == 1 else y.shape[1]
        fmt = WAVEFORMATEX(WAVE_FORMAT_PCM, channels, rate,
                           rate * channels * 2, channels * 2, 16, 0)
        h = ctypes.c_void_p()
        if self._mm.waveOutOpen(ctypes.byref(h), WAVE_MAPPER,
                                ctypes.byref(fmt), 0, 0, CALLBACK_NULL):
            return False
        with self._lock:
            self._h = h
            self._channels, self._rate = channels, rate
            self._src, self._pos, self._queued = y, 0, 0
            self._singing = True
            self._stop_at = self._follow = None
            self._owed = 0
            self._chunks = [Chunk(CHUNK_FRAMES * channels * 2)
                            for _ in range(CHUNKS_AHEAD + 1)]
            self._top_up()               # the card starts out already fed
        self._quit = False
        self._feeder = threading.Thread(target=self._feed, daemon=True)
        self._feeder.start()
        return True

    def stop_at(self):
        """Stop the song after what the card already has, and say where.

        That is the earliest it can stop without a gap -- what has been handed
        over cannot be taken back -- and it is the frame whatever follows has
        to carry on from.
        """
        with self._lock:
            if self._h is None or not self._singing:
                return 0
            self._stop_at = self._queued
            return self._stop_at

    def follow_with(self, samples):
        """What to play where the song leaves off. None lets it end there.

        Nothing happens unless a stop is waiting to be followed. A follow-on
        worked out for a song that has since been stopped outright, or that
        something else has been started over the top of, arrives to find
        nothing to attach itself to -- which is what should become of it,
        rather than its cutting into whatever is playing now.
        """
        with self._lock:
            if self._h is None or self._stop_at is None or not self._singing:
                return
            if samples is None or not len(samples):
                self._follow = np.zeros((0,), dtype=np.float32)
                return
            self._follow = np.ascontiguousarray(
                np.asarray(samples, dtype=np.float32))

    def playing(self):
        with self._lock:
            if self._h is None:
                return False
            if any(c.lent and not (c.hdr.dwFlags & WHDR_DONE)
                   for c in self._chunks):
                return True
            if not self._spent():
                return True
        self.stop()                      # finished: give the device back
        return False

    def position(self):
        """How many frames have been played, straight from the device."""
        with self._lock:
            if self._h is None:
                return 0
            mm = MMTIME()
            mm.wType = TIME_SAMPLES
            if self._mm.waveOutGetPosition(self._h, ctypes.byref(mm),
                                           ctypes.sizeof(mm)):
                return 0
            if mm.wType != TIME_SAMPLES:  # the device offered another unit
                return 0
            return int.from_bytes(bytes(mm.u)[:4], 'little')

    def stop(self):
        self._quit = True
        feeder = self._feeder
        if feeder is not None and feeder is not threading.current_thread():
            feeder.join(timeout=1.0)
        self._feeder = None
        with self._lock:
            h, chunks = self._h, self._chunks
            self._h, self._chunks = None, []
            self._src = self._follow = None
        if h is None:
            return
        try:
            self._mm.waveOutReset(h)     # a lent block has to be taken back
            for c in chunks:             # before its memory can be let go of
                if c.lent:
                    self._mm.waveOutUnprepareHeader(
                        h, ctypes.byref(c.hdr), ctypes.sizeof(c.hdr))
                    c.lent = False
        finally:
            self._mm.waveOutClose(h)
        del chunks

    # -- the feeding -------------------------------------------------------

    def _feed(self):
        """Keep the card's hands full until there is nothing left to put in."""
        while not self._quit:
            with self._lock:
                if self._h is None:
                    return
                self._top_up()
                if self._spent() and not any(c.lent for c in self._chunks):
                    return               # played out; playing() will notice
            time.sleep(0.004)

    def _top_up(self):
        """Take back what the card has finished with, and hand it more."""
        for c in self._chunks:
            if c.lent:
                if not (c.hdr.dwFlags & WHDR_DONE):
                    continue
                self._mm.waveOutUnprepareHeader(
                    self._h, ctypes.byref(c.hdr), ctypes.sizeof(c.hdr))
                c.lent = False
            block = self._next_block()
            if block is None:
                continue
            ctypes.memmove(c.buf, block, len(block))
            c.hdr.dwBufferLength = len(block)
            c.hdr.dwFlags = 0
            size = ctypes.sizeof(c.hdr)
            if self._mm.waveOutPrepareHeader(self._h, ctypes.byref(c.hdr),
                                             size):
                continue
            if self._mm.waveOutWrite(self._h, ctypes.byref(c.hdr), size):
                self._mm.waveOutUnprepareHeader(self._h, ctypes.byref(c.hdr),
                                                size)
                continue
            c.lent = True

    def _limit(self):
        """How far into what is being read the card may be fed."""
        if self._src is None:
            return 0
        if self._singing and self._stop_at is not None:
            return min(len(self._src), self._stop_at)
        return len(self._src)

    def _spent(self):
        # holding the line for something that has not arrived is not finished
        if (self._singing and self._stop_at is not None
                and self._follow is None):
            return False
        return self._pos >= self._limit()

    def _next_block(self):
        """The next chunk of PCM, or None when there is nothing to send."""
        if self._src is None:
            return None
        if self._pos < self._limit():
            take = self._src[self._pos:self._pos + CHUNK_FRAMES]
            self._pos += len(take)
            if self._singing:
                self._queued += len(take)
            return to_pcm(take)[0]
        if self._singing and self._stop_at is not None:
            if self._follow is None:
                # Nothing to follow with yet. Hold the line rather than let
                # the card run dry, and count what was held, so that what does
                # arrive is picked up where it had got to rather than replayed.
                self._owed += CHUNK_FRAMES
                return b'\0' * (CHUNK_FRAMES * self._channels * 2)
            self._singing = False
            self._src = self._follow
            self._pos = min(self._owed, len(self._src))
            self._owed = 0
            self._follow = None
            return self._next_block()
        return None


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
    """AudioToolbox, handed the whole buffer at once.

    The queue wants a callback for the moment a buffer has been consumed, so
    it is given one, but the callback fires when the queue has finished
    reading the samples rather than when the speaker has finished making the
    sound. The length of the audio is what says whether it is still playing.

    What follows a stop is started after it rather than spliced onto it, so
    the join is not the seamless one waveOut gets. What was worked out while
    the queue was being turned round is skipped rather than played, so at
    least the room goes on decaying at the rate it should.
    """

    def __init__(self):
        self._at = ctypes.cdll.LoadLibrary(AUDIO_TOOLBOX)
        #: held for as long as the queue may call it
        self._cb = CALLBACK(lambda user, queue, buf: None)
        self._q = None
        self._ends = 0.0
        self._began = 0.0
        self._rate = SAMPLE_RATE
        self._stopped = 0.0

    def play(self, samples, rate):
        self.stop()
        data, channels = to_pcm(samples)
        if not len(data):
            return False
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

    def stop_at(self):
        at = self.position()
        self.stop()
        self._stopped = time.monotonic()
        return at

    def follow_with(self, samples):
        if samples is None or not len(samples):
            return
        gone = int(max(0.0, time.monotonic() - self._stopped) * self._rate)
        if gone < len(samples):
            self.play(samples[gone:], self._rate)

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
        self._began = 0.0
        self._rate = SAMPLE_RATE
        self._stopped = 0.0

    def play(self, samples, rate):
        from ppc.render import write_wav
        self.stop()
        y = np.asarray(samples, dtype=np.float32)
        path = os.path.join(tempfile.gettempdir(), 'vw_playback.wav')
        try:
            write_wav(path, y, rate)
        except OSError:
            return False
        self._path = path
        self._began = time.monotonic()
        self._rate = rate
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

    def stop_at(self):
        at = self.position()
        self.stop()
        self._stopped = time.monotonic()
        return at

    def follow_with(self, samples):
        if samples is None or not len(samples):
            return
        gone = int(max(0.0, time.monotonic() - self._stopped) * self._rate)
        if gone < len(samples):
            self.play(samples[gone:], self._rate)

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
        produced them.
        """
        if self._out is None:
            return False
        y = np.asarray(samples, dtype=np.float32)
        if not len(y):
            return False
        try:
            return bool(self._out.play(y, rate))
        except Exception:                                    # noqa: BLE001
            return False

    def stop_at(self):
        """Stop the singing as soon as it can be, and say where that was.

        The frame it comes back with is where whatever follows has to carry on
        from. It is a little past where the key was pressed, because what the
        card already has cannot be taken back -- sixty milliseconds at the
        most, which is not enough to notice, and taking it back would leave a
        hole in the sound, which is.
        """
        if self._out is None:
            return 0
        try:
            return int(self._out.stop_at())
        except Exception:                                    # noqa: BLE001
            return 0

    def follow_with(self, samples):
        """What to play where the song left off. None lets it end there."""
        if self._out is None:
            return
        try:
            self._out.follow_with(samples)
        except Exception:                                    # noqa: BLE001
            pass

    def stop(self):
        if self._out is None:
            return
        try:
            self._out.stop()
        except Exception:                                    # noqa: BLE001
            pass

    def position(self):
        """How far into the sound it has got, in frames. 0 when stopped."""
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
