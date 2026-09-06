#!/usr/bin/env python3
"""Minimal Standard MIDI File reader, for VocalWriter's File > Export output.

VocalWriter exports one MIDI track per song track, and -- per its manual --
writes each note's lyric text as a Text meta event (0x01) and the phonemes it
derived for that note as a Lyric meta event (0x05), both at the note's own tick.
That makes an export the exact ground truth for what a .trk file contains, which
is what the .trk note-event layout was worked out against.
"""
import re
import struct

META_TEXT = 0x01
META_TRACK_NAME = 0x03
META_LYRIC = 0x05
META_TEMPO = 0x51
META_TIME_SIG = 0x58
META_END = 0x2F


# VocalWriter writes phonemes with case carrying the length: every uppercase
# symbol is exactly two letters and every lowercase one exactly one, so a string
# splits without a dictionary. Verified over Daisy.trk's exported phonemes --
# every uppercase run is of even length. The inventory is ARPABET plus
# allophones: DX (flap), TX (unreleased t), LX (dark l), EL (syllabic l), and
# AX/UX (schwas).
_PHONEME_RE = re.compile(r'[A-Z]{2}|[a-z]')


def split_phonemes(s):
    """Split a Lyric meta event's phoneme string into symbols.

    >>> split_phonemes('swIYt')
    ['s', 'w', 'IY', 't']
    >>> split_phonemes('kAEnTX')
    ['k', 'AE', 'n', 'TX']
    """
    return _PHONEME_RE.findall(s)


class Note(object):
    __slots__ = ('tick', 'pitch', 'velocity', 'duration', 'text', 'phonemes',
                 'channel')

    def __init__(self, tick, pitch, velocity, duration, text='', phonemes='',
                 channel=0):
        self.tick = tick
        self.pitch = pitch
        self.velocity = velocity
        self.duration = duration
        self.text = text
        self.phonemes = phonemes
        #: One MIDI track can carry sixteen parts at once, one per channel --
        #: a format 0 file always does, and plenty of format 1 files do too.
        #: They are separate parts and want separating, so every note
        #: remembers which one it belongs to.
        self.channel = channel

    def __repr__(self):
        return '<Note t=%d p=%d v=%d d=%d ch=%d %r %r>' % (
            self.tick, self.pitch, self.velocity, self.duration,
            self.channel, self.text, self.phonemes)


class Track(object):
    def __init__(self, name=''):
        self.name = name
        self.notes = []
        self.tempos = []
        #: [(tick, value)] where value is -8192..8191
        self.bends = []
        #: [(tick, semitones)] from RPN 0, pitch-bend sensitivity
        self.bend_range = []
        #: [(tick, program)]
        self.programs = []
        #: [(tick, beats per bar, beat note)] -- 4/4 is (4, 4). The engine has
        #: no use for it; the editor does, to say which bar a note falls in.
        self.time_sigs = []
        #: the same three lists with the channel kept, which is what
        #: `on_channel` divides them by. The plain ones above are every
        #: channel at once, which is right for a track that only has one.
        self._bends = []
        self._bend_range = []
        self._programs = []

    def channels(self):
        """The channels this track has notes on, lowest first."""
        return sorted({n.channel for n in self.notes})

    def on_channel(self, chan):
        """Just the part of this track that plays on one channel.

        Everything that is the file's rather than the part's -- the tempo map
        and the time signatures -- comes along, since it is as true of one
        channel as of any other.
        """
        sub = Track(self.name)
        sub.notes = [n for n in self.notes if n.channel == chan]
        sub.tempos = list(self.tempos)
        sub.time_sigs = list(self.time_sigs)
        sub.bends = [(t, v) for t, v, c in self._bends if c == chan]
        sub.bend_range = [(t, v) for t, v, c in self._bend_range if c == chan]
        sub.programs = [(t, p) for t, p, c in self._programs if c == chan]
        sub._bends = [e for e in self._bends if e[2] == chan]
        sub._bend_range = [e for e in self._bend_range if e[2] == chan]
        sub._programs = [e for e in self._programs if e[2] == chan]
        return sub


def _vlq(b, i):
    v = 0
    while True:
        c = b[i]
        i += 1
        v = (v << 7) | (c & 0x7F)
        if not c & 0x80:
            return v, i


class MidiFile(object):
    def __init__(self, blob):
        if blob[:4] != b'MThd':
            raise ValueError('not a Standard MIDI File')
        hlen = struct.unpack('>I', blob[4:8])[0]
        self.format, n_tracks, self.division = struct.unpack('>3H', blob[8:14])
        self.tracks = []
        pos = 8 + hlen
        for _ in range(n_tracks):
            if blob[pos:pos + 4] != b'MTrk':
                raise ValueError('expected MTrk at 0x%x' % pos)
            ln = struct.unpack('>I', blob[pos + 4:pos + 8])[0]
            self.tracks.append(self._read_track(blob[pos + 8:pos + 8 + ln]))
            pos += 8 + ln

    @classmethod
    def from_file(cls, path):
        with open(path, 'rb') as fh:
            return cls(fh.read())

    @staticmethod
    def _read_track(b):
        trk = Track()
        i, tick, running = 0, 0, None
        rpn = {}
        sounding = {}
        # text and lyric arrive at the same tick as their note, but not
        # necessarily before it, so they are collected and attached afterwards
        text_at, lyric_at = {}, {}
        while i < len(b):
            dt, i = _vlq(b, i)
            tick += dt
            status = b[i]
            if status & 0x80:
                running = status
                i += 1
            else:
                status = running
            if status == 0xFF:
                mtype = b[i]
                i += 1
                ln, i = _vlq(b, i)
                data = b[i:i + ln]
                i += ln
                if mtype == META_TRACK_NAME:
                    trk.name = data.decode('mac-roman', 'replace')
                elif mtype == META_TEXT:
                    text_at[tick] = data.decode('mac-roman', 'replace')
                elif mtype == META_LYRIC:
                    lyric_at[tick] = data.decode('mac-roman', 'replace')
                elif mtype == META_TEMPO and ln == 3:
                    trk.tempos.append((tick, (data[0] << 16) | (data[1] << 8) | data[2]))
                elif mtype == META_TIME_SIG and ln >= 2:
                    # stored as the denominator's power of two
                    trk.time_sigs.append((tick, data[0], 1 << data[1]))
            elif status in (0xF0, 0xF7):
                ln, i = _vlq(b, i)
                i += ln
            else:
                high = status & 0xF0
                chan = status & 0x0F
                if high == 0x90 and b[i + 1] > 0:
                    sounding.setdefault((chan, b[i]), []).append(
                        (tick, b[i + 1]))
                    i += 2
                elif high == 0x80 or (high == 0x90 and b[i + 1] == 0):
                    key = (chan, b[i])
                    if sounding.get(key):
                        start, vel = sounding[key].pop(0)
                        trk.notes.append(
                            Note(start, b[i], vel, tick - start,
                                 channel=chan))
                    i += 2
                elif high == 0xE0:
                    value = ((b[i + 1] << 7) | b[i]) - 8192
                    trk.bends.append((tick, value))
                    trk._bends.append((tick, value, chan))
                    i += 2
                elif high == 0xB0:
                    # RPN 0 is pitch-bend sensitivity; VocalWriter changes it
                    # mid-song (8, then 6, then 12 semitones in Daisy). Each
                    # channel selects its own RPN, so the selection is kept
                    # per channel rather than for the track as a whole.
                    cc, val = b[i], b[i + 1]
                    sel = rpn.get(chan, (127, 127))
                    if cc == 6 and sel == (0, 0):
                        trk.bend_range.append((tick, val))
                        trk._bend_range.append((tick, val, chan))
                    elif cc == 101:
                        rpn[chan] = (val, sel[1])
                    elif cc == 100:
                        rpn[chan] = (sel[0], val)
                    i += 2
                elif high == 0xC0:
                    trk.programs.append((tick, b[i]))
                    trk._programs.append((tick, b[i], chan))
                    i += 1
                else:
                    i += 1 if high in (0xC0, 0xD0) else 2
        trk.notes.sort(key=lambda n: (n.tick, n.pitch))
        for n in trk.notes:
            n.text = text_at.get(n.tick, '')
            lyric = lyric_at.get(n.tick, '')
            if n.text:
                # VocalWriter writes both: the word as text, the phonemes it
                # derived for it as the lyric
                n.phonemes = lyric
            else:
                # Everyone else writes the sung word in the Lyric event, which
                # is what that event is for. Read as phonemes it comes out as
                # one phoneme per letter -- "dai-" sung as d, a, i.
                n.text, n.phonemes = lyric, ''
        return trk


def main():
    import sys
    mf = MidiFile.from_file(sys.argv[1])
    print('format %d, %d tracks, %d ticks/quarter'
          % (mf.format, len(mf.tracks), mf.division))
    for t in mf.tracks:
        print()
        print('-- %r: %d notes --' % (t.name, len(t.notes)))
        for n in t.notes:
            print('   t=%-6d p=%-3d v=%-3d d=%-4d ch=%-2d %-9s %s'
                  % (n.tick, n.pitch, n.velocity, n.duration, n.channel,
                     n.text, n.phonemes))


if __name__ == '__main__':
    main()
