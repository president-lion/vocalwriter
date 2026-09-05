#!/usr/bin/env python3
"""The engine the editor asks for pronunciations and audio.

It used to be a process of its own, spoken to in JSON lines, because a render
under the PowerPC interpreter ran tens of millions of guest instructions and
took about as long as the music lasts -- something that cannot happen on the
thread drawing the window. The C engine renders a minute of singing in a few
hundredths of a second, so there is nothing left to put behind a process
boundary: this is imported and called.

What it holds is worth holding: the voice bank, the dictionary and the shared
tables are read once, and rendered audio is kept in memory under a key made
from the song, so playing the same thing twice renders once.

The audio comes back as samples. It used to come back as a path -- every
render went to a file, and playing a song meant writing one and handing it to
a player that could only read files. Nothing here needs that: the mix is an
array by the time it is finished, the player takes an array, and a file is
written only when somebody asks for one by name, which is what exporting is.
"""
import collections
import hashlib
import json          # only for the cache key
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from ppc import paths                                        # noqa: E402
from ppc.lexicon import open_lexicon                         # noqa: E402
from ppc.midi import syllable_lengths                        # noqa: E402
from ppc.phonology import is_nucleus, targets                # noqa: E402
from ppc.render import (SAMPLE_RATE, Note, Renderer,         # noqa: E402
                        write_wav)
from ppc.render import engine_name, open_engine              # noqa: E402
from tools.ttvi import load as load_ttvi, phoneme_order      # noqa: E402


PALETTE_FILE = paths.bundled('emu', 'phoneme_palette.json')

#: How much rendered audio to keep in memory. Stereo float samples are 345 KB
#: a second, so this is about twelve minutes of singing -- enough that an
#: afternoon of playing the same song back never renders it twice, and bounded
#: so that an afternoon of playing different ones does not grow without end.
CACHE_BYTES = 256 * 1024 * 1024

#: the palette was read out of the running application, and spells two
#: phonemes differently from the engine's own table
PALETTE_ALIAS = {'OH': 'O', 'DX': 'DD'}

#: What the application's own palette leaves out.
#:
#: `emu/phoneme_palette.json` was read out of VocalWriter's running interface
#: and holds the fifty phonemes it offers. Its engine has fifty-seven. The
#: other seven are real -- they have formants, manners and durations like any
#: other, and the dictionary and the letter-to-sound rules produce some of them
#: -- so leaving them out would mean a sound the engine can make that this
#: program cannot reach.
#:
#: The examples are observed rather than assumed: each one is a word the
#: dictionary or VocalWriter's own scores actually spell with that phoneme.
UNLISTED = [
    ('IX', 'rosES, beatEn'),        # lexicon: r OW z IX z, b IY t IX n
    ('Q',  'greaTer, heaTer'),      # letter-to-sound: g r EY Q ER
    ('DD', 'beTTer, whaT'),         # the scores' DX: b EH / DD ER, w UX DD
    ('TX', "iT, can'T"),            # the scores: IH TX, k AE n TX
    ('RX', 'a short r-coloured vowel'),
    ('QX', 'a longer Q'),
    ('%',  'silence, a rest'),
]

#: how long after the last note the file runs on, so its decay is not clipped
TAIL_SECONDS = 0.4

#: The metronome. This is the one sound here that is not VocalWriter's -- it
#: is a ruler held up against the singing, not part of it -- so it is mixed in
#: only when the song is played and never written into an exported file.
CLICK_HZ = 1000.0            # the beats
CLICK_ACCENT_HZ = 1600.0     # the first beat of each bar
CLICK_SECONDS = 0.035
CLICK_LEVEL = 0.22


def click(rate=SAMPLE_RATE, hz=CLICK_HZ, level=CLICK_LEVEL):
    """One tick: a tone that dies away immediately, so it reads as a tap."""
    n = int(rate * CLICK_SECONDS)
    t = np.arange(n, dtype=np.float32) / float(rate)
    return (level * np.sin(2 * np.pi * hz * t)
            * np.exp(-t * 45.0)).astype(np.float32)


def with_metronome(y, bpm, bar_beats, start=0.0, rate=SAMPLE_RATE, lead=0.0):
    """The audio with a tick on every beat, accented at each bar line.

    `start` is the beat the audio itself begins on, so that playing from
    partway through a song still puts the ticks on the song's beats and the
    accents on its bar lines, rather than counting a fresh bar from wherever
    the cursor happened to be.

    `lead` is the silence a part singing early has put in front of everything
    (see `lead_seconds`). The click is on the song's beats, and the song has
    moved, so the click moves with it -- otherwise turning the metronome on
    would report that a humanised song is a quarter of a second out of time.
    """
    spb = 60.0 / max(bpm, 1e-6)
    plain, accent = click(rate), click(rate, CLICK_ACCENT_HZ,
                                       CLICK_LEVEL * 1.4)
    out = np.array(y, dtype=np.float32)
    n = out.shape[0]
    ticks = np.zeros(n, dtype=np.float32)
    beats = max(1, int(round(bar_beats)))             # keep 3/4, 6/8 and 7/8
    k = int(math.ceil(start - 1e-9))
    ahead = int(round(lead * rate))
    while True:
        at = int(round((k - start) * spb * rate)) + ahead
        if at >= n:
            break
        tick = accent if (k % beats == 0) else plain
        end = min(at + len(tick), n)
        ticks[at:end] += tick[:end - at]
        k += 1
    out += ticks if out.ndim == 1 else ticks[:, None]
    peak = float(np.abs(out).max())
    if peak > 1.0:                        # the ticks must not push it into clip
        out /= peak
    return out

#: The engine's pitch grid: it moves in steps of a 256th of an octave, which
#: is 4.6875 cents, and it takes the step at or below what it is asked for.
#: Measured on a held vowel by sweeping the bend a cent at a time -- every
#: value from -4 to -1 cents came out at -4.69 and every value from 0 to +4
#: came out at 0. Asking for a cent and being given five flat is not a detune
#: control, so half a step is added before the engine floors it, which turns
#: the floor into a round and leaves the worst error at 2.3 cents rather than
#: 4.7 in one direction and 0 in the other.
PITCH_STEP = 12.0 / 256.0

#: The bend range the engine is put into before any bend is sent. A song
#: carries its bend in semitones, so the range is ours to choose; twelve covers
#: anything a voice would do and keeps the 14-bit value comfortably fine --
#: one step is a fifth of a cent.
BEND_RANGE = 12.0


#: How often a bend in motion is sent to the engine. One frame is 220
#: samples, about 5 ms, which is as often as the engine can act on anything.
BEND_STEP = 0.005


def glide(points, step=BEND_STEP):
    """Fill in between the written points so a bend moves rather than jumps.

    A bend is written down as the places it passes through -- start here, end
    there -- but the engine is not given a curve, only individual values at
    individual moments. Two points a second apart would hold the first value
    for that whole second and then snap to the second one, which is a jump, not
    a slide. The values in between are worked out here, at the rate the engine
    can actually use them.

    Only where the point says to. A point carries a third field saying whether
    it slides into the next one or simply holds until then; filling in between
    every pair regardless is how a bend that finished on one note went on
    sliding into the next bend written anywhere later in the song.
    """
    pts = [(t, v, bool(sl)) for t, v, sl in _triples(points)]
    if len(pts) < 2:
        return [(t, v) for t, v, _sl in pts]
    out = []
    for (t0, v0, slides), (t1, v1, _) in zip(pts, pts[1:]):
        out.append((t0, v0))
        if not slides or v1 == v0 or t1 - t0 <= step:
            continue                        # a hold needs nothing in between
        n = int((t1 - t0) / step)
        for k in range(1, n):
            f = k * step / (t1 - t0)
            out.append((t0 + k * step, v0 + (v1 - v0) * f))
    out.append((pts[-1][0], pts[-1][1]))
    return out


def _triples(points):
    """Accept points with or without the slide flag; without it, they hold."""
    for pt in points:
        if len(pt) >= 3:
            yield pt[0], pt[1], pt[2]
        else:
            yield pt[0], pt[1], False


def bend_events(points, t0, t1, detune=0.0):
    """Bend points for one phrase, in seconds from its own start.

    Everything is rebased on the phrase because each phrase is rendered on its
    own engine. Whatever bend was in force when the phrase began is carried in
    ahead of it, or a phrase starting partway through a slide would begin on
    the wrong pitch.

    `detune` is semitones added to every bend, and to the pitch the phrase
    starts on where there is no bend at all: it is a part sung a shade sharp
    or flat from end to end. It rides on the bend rather than on the engine's
    own Speech_Detune because the bend is what moves the pitch -- Speech_Detune
    was measured across its whole range and shifts a note by about sixteen
    cents flat to five sharp, doing nothing at all between -256 and +2048.
    """
    if not points and not detune:
        return []
    ev = [(-1.0, 'sens', int(BEND_RANGE))]
    before = [v for t, v, _sl in _triples(points) if t < t0]
    base = (before[-1] if before else 0.0) + detune
    last = None
    # With no bend and no detune the phrase starts where it always did, and
    # this is the value that would have been sent anyway.
    if before or detune:
        last = _raw(base)
        ev.append((-1.0, 'bend', last))
    for t, v in glide(points):
        if t0 <= t <= t1:
            raw = _raw(v + detune)
            if raw != last:                 # sending the same value twice is
                ev.append((t - t0, 'bend', raw))   # work the engine need not do
                last = raw
    return ev


def _raw(semitones):
    """Semitones as the 14-bit number Speech_PitchBend is given."""
    v = int(round(semitones / BEND_RANGE * 8192))
    return max(-8192, min(8191, v))


def pan_gains(pan):
    """The left and right gains for a pan of -1 (hard left) to +1 (right).

    Constant power -- a track keeps its loudness as it moves across -- but
    normalised so that the middle is unity in both channels rather than the
    usual three decibels down. That way a track left in the middle sounds
    exactly as it did before there was any panning at all, and switching a
    song between one channel and two changes nothing you can hear.
    """
    theta = (max(-1.0, min(1.0, float(pan))) + 1.0) * (math.pi / 4.0)
    return (math.sqrt(2.0) * math.cos(theta),
            math.sqrt(2.0) * math.sin(theta))


#: VocalWriter's own defaults for a new song, from its reverb dialog.
DEFAULT_REVERB = (40, 24)

#: How long the reverb rings on after the singing stops, at the largest room.
#: Not to be confused with TAIL_SECONDS above, which is the much shorter decay
#: a single note is given room for.
#: The delay lines scale with the room, and so does the tail: measured from an
#: impulse, it is 5.2 seconds at 100 and in proportion all the way down --
#: 2.1 at the default 40. The render gives it that much silence to decay into
#: and trims back whatever is still silent, so a song ends when the room does
#: and not a second after the last note, which is where it used to end.
REVERB_TAIL_SECONDS = 5.2

#: What a voice at full volume is aimed at, as a fraction of full scale.
#: Not 1.0: several parts are going to be added together, and a voice that
#: only just fits on its own leaves nothing for the others.
HEADROOM = 0.7

#: The phrase every voice is measured with -- one plain note, long enough to
#: reach its steady level and short enough to measure in a few milliseconds.
TEST_NOTE = ('d', 'AA')

#: The most of a note that may be given to the note in front of it, so that a
#: note opening with consonants can start early enough for its vowel to land
#: on the beat. Half: take more than that and what is left is a grace note.
ANTICIPATE_MOST = 0.5


def onset_beats(note, bpm):
    """How long a note's opening consonants last, in beats.

    The phonemes before the first vowel: what stands between the note starting
    and the note being heard. `Syllable_Duration` scales a syllable to fill its
    note, and these durations already add up to the note, so their lengths are
    what they will be.
    """
    t = targets()
    ms = 0.0
    for sym, dur in zip(note.phonemes, note.durations or []):
        if is_nucleus(sym, t):
            break
        ms += dur
    return ms * bpm / 60000.0


def anticipate(notes, bpm, consonants, room):
    """Move every note's consonants in front of its beat, in place.

    A note is heard where its vowel is, not where it starts: "day" on beat two
    with a 60 ms /d/ in front of it is heard 60 ms after beat two, and how late
    depends on how the word is spelled -- nothing before a vowel, a little
    before /d/, a lot before "str-". That is what makes a line with hard onsets
    sound as though it is dragging, and it gets worse the longer the consonants
    are allowed to be.

    So a note that opens with consonants is started that much earlier and given
    that much more time, and the time is taken from the note in front of it,
    which is shortened by the same amount. Nothing moves: the vowels land where
    the notes were, the phrase is the same length, and the consonants are sung
    into the end of the note before -- which is what a singer does.

    `room` is how much silence there is before the phrase, since the first note
    has no note in front of it to borrow from. Returns how much earlier the
    phrase now starts.
    """
    def retime(note, beats):
        note.beats = beats
        note.durations = syllable_lengths(note.phonemes,
                                          beats * 60000.0 / bpm, consonants)

    lead = max(0.0, min(onset_beats(notes[0], bpm), room))
    if lead > 0:
        retime(notes[0], notes[0].beats + lead)
    for i in range(1, len(notes)):
        give = min(onset_beats(notes[i], bpm),
                   ANTICIPATE_MOST * notes[i - 1].beats)
        if give <= 0:
            continue
        retime(notes[i - 1], notes[i - 1].beats - give)
        retime(notes[i], notes[i].beats + give)
    return lead


def clean_reverb(values):
    """(room, wet) as whole percentages, clamped. None is no reverb at all.

    Two numbers, because they are the two the application has: the room scales
    the four delay lines and the wet is how much of the result is heard, the
    dry part being what is left.
    """
    if not values:
        return (0, 0)
    try:
        room = int(round(float(values.get('room', DEFAULT_REVERB[0]))))
        wet = int(round(float(values.get('wet', DEFAULT_REVERB[1]))))
    except (AttributeError, TypeError, ValueError):
        return (0, 0)
    return (max(0, min(100, room)), max(0, min(100, wet)))


def tracks_of(song):
    """A song's parts, whichever way it was written down.

    A song used to be one voice and one list of notes, and single notes are
    still asked for that way when one is previewed. Both shapes arrive here as
    a list of parts so that nothing further down has to know the difference.
    """
    parts = song.get('tracks')
    if parts:
        return parts
    return [{'program': song.get('program', 0),
             'notes': song.get('notes') or [],
             'bends': song.get('bends') or [],
             'velocity': song.get('velocity', 64)}]


def lead_seconds(song):
    """The silence in front of the song, so that a part can come in early.

    A part's offset moves it off the beat, and a part cannot be moved earlier
    than the beginning of the song -- there is nothing there to move it into,
    and trimming its first few milliseconds instead is cutting the attack off
    the very note the offset was meant to place. So the earliest part decides:
    everything, and the metronome with it, is delayed by as much as that part
    is early, which leaves every part in the right place relative to every
    other and costs a quarter of a second of silence at the front at worst.

    A song where nothing is early -- which is every song that has not been
    humanised -- gets none, and renders exactly as it did before.
    """
    offsets = [float(t.get('offset', 0) or 0) for t in tracks_of(song)]
    return max(0.0, -min(offsets)) / 1000.0 if offsets else 0.0


def is_rest(phonemes):
    """A note nobody sings: no phonemes, or nothing but silence."""
    return not phonemes or all(p == '%' for p in phonemes)


def phrases(entries):
    """[(start in beats, [note...])] between the rests, and the total length.

    The rests themselves are dropped -- they are not sung, they are the gaps
    the phrases are placed around.
    """
    runs, cur, start, at = [], [], 0.0, 0.0
    for e in entries:
        beats = float(e.get('beats', 0.5))
        if is_rest(e.get('phonemes')):
            if cur:
                runs.append((start, cur))
                cur = []
        else:
            if not cur:
                start = at
            cur.append(e)
        at += beats
    if cur:
        runs.append((start, cur))
    return runs, at


class Engine(object):
    def __init__(self):
        self._lex = None
        self._voices = None
        self._palette = None
        #: key -> (samples, peak, whether it stopped short). A render costs
        #: seconds and hearing the same thing twice should cost none, so the
        #: mix is kept rather than the file it used to be written to. The
        #: oldest goes when the total passes CACHE_BYTES.
        self._cache = collections.OrderedDict()
        #: voice -> how far it has to be turned down, worked out once
        self._headroom = {}
        #: whether the last render ended early, for the caller to pass on
        self.stopped_short = False

    @property
    def lex(self):
        if self._lex is None:
            self._lex = open_lexicon()
        return self._lex

    def ping(self):
        """What is running, for the window to say out loud."""
        eng = open_engine()
        bank = bool(getattr(eng, 'has_bank', True))
        voices = len([n for n in eng.voice_names() if n])
        eng.close()
        return {'engine': engine_name(),
                'python': '.'.join(str(v) for v in sys.version_info[:3]),
                'voices': voices, 'bank': bank}

    def phonemes(self, words):
        out = {}
        for w in words:
            clean = ''.join(c for c in w if c.isalpha() or c == "'")
            out[w] = (self.lex.phonemes(clean) or []) if clean else []
        return out

    def headroom(self, voice_id):
        """What this voice has to be turned down by to fit inside full scale.

        The bank's voices are nothing like each other in level. Robert peaks at
        about seven tenths of full scale with the engine wide open; Strings,
        Trumpet and the rest of the wavetable voices come out forty or fifty
        times over it, and the engine clamps every sample that goes past --
        which is distortion baked into the samples before any volume of ours
        can touch it. So each voice is sung once, quietly, to see how loud it
        really is, and told to stay inside afterwards.

        A voice that already fits is left alone: 1.0 is the engine's own level
        and is what the natural voices get, so they render exactly as before.
        """
        if voice_id in self._headroom:
            return self._headroom[voice_id]
        probe = 0.01
        r = Renderer(program=0, bpm=100, voice_id=voice_id, level=probe)
        y = r.render([Note(62, 0.6, list(TEST_NOTE))])
        peak = float(np.abs(y).max()) / probe if len(y) else 1.0
        # A voice that already fits is left exactly alone -- the test is
        # whether it clips, not whether it is loud -- so every voice that
        # worked before this renders as it did before this.
        gain = 1.0 if peak <= 1.0 else HEADROOM / max(peak, 1e-6)
        self._headroom[voice_id] = gain
        return gain

    def voices(self):
        """Every voice the bank holds, in its own order.

        Not the sixteen a program change reaches: the bank has 87, and the
        ones with instrument names sing lyrics as readily as the ones with
        people's names -- "special synthetic models of musical instruments
        with dynamic vocal tracts", as the manual has it. Five more sit in the
        bank with nothing in the program map pointing at them at all.
        """
        if self._voices is None:
            eng = open_engine()
            self._voices = [n for n in eng.voice_names() if n]
            eng.close()
        return self._voices

    def program_voices(self, programs):
        """Which voice each program number picks, for reading an old song."""
        eng = open_engine()
        out = [eng.program_voice(int(p)) for p in programs]
        eng.close()
        return out

    def palette(self):
        """Every phoneme the engine has, with an example word for each.

        The engine's own table is what decides the list, so nothing it can
        pronounce is missing. The application's palette supplies the example
        words for the ones it shows, and anything left over is filled in from
        UNLISTED and, failing that, listed bare rather than dropped.
        """
        if self._palette is None:
            examples = dict(UNLISTED)
            try:
                with open(PALETTE_FILE) as fh:
                    for sym, example in json.load(fh):
                        examples[PALETTE_ALIAS.get(sym, sym)] = example
            except (OSError, ValueError):
                pass
            order = phoneme_order(load_ttvi())
            self._palette = [[sym, examples.get(sym, '')] for sym in order]
            for sym, example in UNLISTED:    # in the table under another name
                if sym not in order:
                    self._palette.append([sym, example])
        return self._palette

    def preview(self, phoneme, pitch=60, program=0, beats=0.45, out=None):
        """Render one phoneme on its own, for the picker's Preview button."""
        song = {'bpm': 120, 'program': program,
                'notes': [{'pitch': pitch, 'beats': beats,
                           'phonemes': [phoneme]}]}
        return self.render(song, out)

    def render(self, song, out=None):
        """Sing a song, and hand back the samples.

        `out` writes them to a WAV as well, and is what exporting passes. It
        is not what playing passes: the caller gets the mix itself under
        'audio', one column or two, and plays it out of memory.
        """
        # The metronome is mixed on afterwards and is deliberately left out of
        # the key, so switching it on and off never re-sings the song.
        metro = song.get('metronome') or None
        core = {k: v for k, v in song.items() if k != 'metronome'}
        # A render costs seconds, so never repeat one. The key covers
        # everything that changes the samples.
        key = hashlib.sha256(
            json.dumps(core, sort_keys=True).encode()).hexdigest()
        hit = self._cache.get(key)
        if hit is not None:
            self._cache.move_to_end(key)
            y, peak, short, _sends = hit
            cached = True
        else:
            y, peak = self._samples(core)
            short = self.stopped_short
            self._remember(key, y, peak, short, self.reverb_sends)
            cached = False
        audio = self._ticked(y, metro, core)
        res = {'seconds': len(y) / float(SAMPLE_RATE), 'peak': peak,
               'cached': cached, 'stopped_short': short,
               'audio': audio, 'rate': SAMPLE_RATE}
        if out is not None:
            write_wav(out, audio)
            res['path'] = out
        return res

    def tail(self, song, at):
        """What the room is still doing when the song is stopped `at` frames in.

        Stopping a song should not stop the room it was sung in. The singing
        stops dead, which is what stopping means; this is what carries on --
        the reverb of everything sung up to that moment, and none of what was
        going to come after it, which is why the wet already computed for the
        whole song is no use here and this is worked out instead.

        Only the recent past goes through. The delay lines have forgotten
        anything older than the tail itself, so feeding them the last tail's
        worth of the song puts them in the state they were actually in, and
        the answer is the same to within a couple of parts in 32768 -- while
        costing the same twenty-five milliseconds whether the song was one
        minute long or ten. It is a keypress: it has to be quick.

        None when there is nothing ringing: no reverb, nothing played yet, or
        a song this engine has not got a render of.
        """
        core = {k: v for k, v in song.items() if k != 'metronome'}
        key = hashlib.sha256(
            json.dumps(core, sort_keys=True).encode()).hexdigest()
        hit = self._cache.get(key)
        if hit is None:
            return None
        y, _peak, _short, sends = hit
        at = int(at)
        if not sends or at <= 0 or at >= len(y):
            return None
        pieces = []
        for send, room, wet in sends:
            room_tail = int(SAMPLE_RATE
                            * (REVERB_TAIL_SECONDS * room / 100.0 + 0.5))
            # The singing is shorter than the song: stopping during the
            # reverb's own tail is stopping after the end of the send, and
            # the room is still ringing there. Feed it what there is and
            # count the silence in between.
            start = max(0, at - room_tail)
            warm = send[start:min(at, len(send))]
            if not len(warm):        # older than the room can remember
                continue
            skip = at - start
            eng = open_engine()
            try:
                if not eng.reverb(room / 100.0, 1.0):
                    continue
                padded = np.concatenate(
                    [warm, np.zeros((room_tail, 2), dtype=np.float32)])
                frames = (len(padded) // 220) * 220
                pcm = np.clip(padded[:frames], -1.0, 1.0)
                pcm = (pcm * 32767).astype('<i2').reshape(-1)
                eng.reverberate(pcm)
                rung = pcm.reshape(-1, 2).astype(np.float32) / 32767.0
            finally:
                eng.close()
            pieces.append(rung[skip:] * (wet / 100.0))
        pieces = [p for p in pieces if len(p)]
        if not pieces:
            return None
        out = np.zeros((max(len(p) for p in pieces), 2), dtype=np.float32)
        for p in pieces:
            out[:len(p)] += p
        # a room that has already fallen silent is not worth playing
        loud = np.abs(out).max(axis=1) > (1.0 / 32767.0)
        if not loud.any():
            return None
        return out[:int(np.nonzero(loud)[0][-1]) + 1]

    @staticmethod
    def _entry_bytes(entry):
        y, _peak, _short, sends = entry
        return y.nbytes + sum(send.nbytes for send, _room, _wet in sends)

    def _remember(self, key, y, peak, short, sends):
        """Keep this render, and drop the oldest until the total fits.

        What the reverb was fed is kept beside the mix, because stopping the
        song needs it and a stop can come long after the render did.

        A song larger than the whole allowance is simply not kept: making room
        for it would mean throwing away everything else to hold one thing that
        would itself be thrown away by the next render.
        """
        entry = (y, peak, short, list(sends))
        if self._entry_bytes(entry) > CACHE_BYTES:
            return
        self._cache[key] = entry
        self._cache.move_to_end(key)
        total = sum(self._entry_bytes(v) for v in self._cache.values())
        while total > CACHE_BYTES and len(self._cache) > 1:
            _old, dropped = self._cache.popitem(last=False)
            total -= self._entry_bytes(dropped)

    @staticmethod
    def _ticked(y, metro, song):
        if not metro:
            return y
        return with_metronome(y, float(song.get('bpm', 120)),
                              float(metro.get('bar', 4)),
                              float(song.get('start', 0.0)),
                              lead=lead_seconds(song))

    def _samples(self, song):
        """Mix every track of a song, and say how loud the result came out.

        Returns (samples, peak). The samples are one column if every track sits
        in the middle with no reverb on it, and two otherwise, so a song that
        uses neither is the same file it always was.

        Reverb is a property of the song that a part may take over, like the
        consonant length. Parts are therefore grouped by the reverb they end up
        with, each group mixed and reverberated on its own, and the groups
        added together -- so two parts in the same room go through one
        reverberator and share its tail, and a part in a room of its own is not
        dragged into theirs.
        """
        bpm = float(song.get('bpm', 120))
        consonants = float(song.get('consonants', 1.0))
        start = float(song.get('start', 0.0))
        early = song.get('anticipate', True)
        tracks = tracks_of(song)
        if not any(t.get('notes') for t in tracks):
            raise ValueError('nothing to sing')
        self.stopped_short = False
        #: what each reverb was fed, kept so that stopping the song can work
        #: out what the room is still doing. Set here the way stopped_short
        #: is, rather than returned, so that _samples keeps the shape every
        #: caller already expects of it.
        self.reverb_sends = []
        song_reverb = clean_reverb(song.get('reverb'))
        groups = {}
        lead = lead_seconds(song)
        for t in tracks:
            y = self._track(t, bpm, consonants, start, early, lead)
            # the volume went into the engine, where it can prevent clipping
            # rather than merely quieten it
            vol = 1.0
            left, right = pan_gains(t.get('pan', 0.0))
            own = t.get('reverb')
            rev = song_reverb if own is None else clean_reverb(own)
            groups.setdefault(rev, []).append((y, vol * left, vol * right,
                                               vol))
        parts = [p for group in groups.values() for p in group]
        n = max(len(y) for y, _l, _r, _v in parts)
        stereo = (any(abs(float(t.get('pan', 0.0))) > 1e-6 for t in tracks)
                  or any(wet > 0 for _room, wet in groups))
        if stereo:
            mixes = []
            for rev, group in groups.items():
                dry = np.zeros((n, 2), dtype=np.float32)
                send = np.zeros((n, 2), dtype=np.float32)
                for y, gl, gr, vol in group:
                    dry[:len(y), 0] += y * gl
                    dry[:len(y), 1] += y * gr
                    # What the reverb hears is the same parts with no panning
                    # on them: panning a part moves the part across the room,
                    # it does not carry the room along with it.
                    send[:len(y), 0] += y * vol
                    send[:len(y), 1] += y * vol
                mixes.append((rev, dry, send))
                if rev[1] > 0:
                    self.reverb_sends.append((send, rev[0], rev[1]))
            # Several voices at once can add up past full scale. Turning the
            # mix down is a great deal better than clipping it -- and it has
            # to happen before the reverb, which works on 16-bit samples and
            # would clip whatever it was handed.
            peak = (float(np.abs(sum(d for _r, d, _s in mixes)).max())
                    if n else 0.0)
            if peak > 1.0:
                for _rev, dry, send in mixes:
                    dry /= peak
                    send /= peak
            done = [self._reverberate(dry, send, rev)
                    for rev, dry, send in mixes]
            # a reverb tail makes its group longer than the singing
            out = np.zeros((max(len(d) for d in done), 2), dtype=np.float32)
            for d in done:
                out[:len(d)] += d
            return out, peak
        out = np.zeros(n, dtype=np.float32)
        for y, gl, _gr, _v in parts:      # in the middle both gains are the
            out[:len(y)] += y * gl        # volume, so one channel says it all
        peak = float(np.abs(out).max()) if n else 0.0
        if peak > 1.0:
            # the caller is told the number, so that it can say so rather than
            # leaving someone wondering why the song got quieter
            out /= peak
        return out, peak

    def _reverberate(self, dry, send, reverb):
        """One group's mix with its reverb on it: `dry` panned, `send` centred.

        The reverberator is two mono reverbs side by side -- the left delay
        lines only ever hear the left channel, and nothing crosses between
        them -- so handing it the panned mix put a hard-panned part's reverb
        hard against the same wall, and panning a part swung the whole room
        with it. It is handed the centred sum instead, and what comes back is
        added to the panned dry. The part moves; the room stays where it is.

        Asking for all wet asks for no dry, the engine's dry gain being
        1 - wet, so the reverb returns its own signal on its own and the two
        are mixed here. That is what leaves the dry free to keep its panning,
        and it keeps the dry out of 16 bits on the way through.

        The reverberator works on the 16-bit samples the engine writes, 220
        frames at a time. That is what the application does with its own sound
        buffers; going through floats instead would be a different reverb.
        """
        room, wet = reverb
        if wet <= 0 or not len(dry):
            return dry
        heard = wet / 100.0
        eng = open_engine()
        try:
            if not eng.reverb(room / 100.0, 1.0):
                return dry
            # room enough for the tail to decay into, and half a second over
            room_tail = int(SAMPLE_RATE
                            * (REVERB_TAIL_SECONDS * room / 100.0 + 0.5))
            padded = np.concatenate(
                [send, np.zeros((room_tail, 2), dtype=np.float32)])
            block = 220
            frames = (len(padded) // block) * block
            pcm = np.clip(padded[:frames], -1.0, 1.0)
            pcm = (pcm * 32767).astype('<i2').reshape(-1)
            eng.reverberate(pcm)
            wet_mix = pcm.reshape(-1, 2).astype(np.float32) / 32767.0
        finally:
            eng.close()
        # keep whatever of the tail is not silence, so a big room is not cut
        # off and a small one does not pad the song with nothing
        loud = np.abs(wet_mix).max(axis=1) > (1.0 / 32767.0)
        last = int(np.nonzero(loud)[0][-1]) + 1 if loud.any() else len(dry)
        end = max(len(dry), min(last, len(wet_mix)))
        out = np.zeros((end, 2), dtype=np.float32)
        out[:len(dry)] = dry * (1.0 - heard)
        k = min(end, len(wet_mix))
        out[:k] += wet_mix[:k] * heard
        return out

    def _track(self, track, bpm, consonants, start=0.0, early=True,
               lead=0.0):
        """One track's audio, phrase by phrase, laid out on the beat.

        Rests are not sung. Each run of notes between them goes to its own
        engine and is placed at the time the score puts it, for two reasons.
        A continuous render has no way to hold a silence, because the engine
        only ever plays the next note it is handed, so the rests would close
        up and the song would drift off the beat. Cutting at a silence costs
        nothing: there is no sound there to cut.

        It was also, for a long time, how a render longer than twenty seconds
        was avoided -- that one turned out to be the engine's output buffer
        overrunning at 23.8 seconds rather than anything drifting, and it is
        fixed.

        `start` is where playing begins, in beats. A phrase that is over by
        then is not rendered at all; one already under way is rendered whole
        and has its beginning cut off, so a note the cursor lands in the middle
        of is heard from the middle rather than being retriggered or lost.
        """
        spb = 60.0 / max(bpm, 1e-6)
        vel = int(track.get('velocity', 64))
        program = int(track.get('program', 0))
        voice_id = track.get('voice_id')
        # The engine's own level, so that a part that would clip is turned
        # down before the clamp rather than after it. What is left for the
        # mixing below is only the panning.
        level = float(track.get('volume', 1.0))
        if voice_id is not None:
            level *= self.headroom(voice_id)
        voice = track.get('voice') or None
        # A part may set its own consonant length; without one it follows the
        # project's, which is what the setting in the song dialog is.
        own = track.get('consonants')
        if own is not None:
            try:
                consonants = float(own)
            except (TypeError, ValueError):
                pass
        # [(beat, semitones, slides into the next)], in the song's own time
        bends = sorted(_triples(track.get('bends') or []))
        # A part sung a shade sharp or flat, and a shade early or late. Two
        # voices in perfect tune and perfect time are one voice twice as loud;
        # a few cents and a few milliseconds apart is what two singers are.
        detune = float(track.get('detune', 0)) / 100.0        # cents
        if detune:                        # see PITCH_STEP: floor into round
            detune += PITCH_STEP / 2.0
        # `lead` has already made room for the earliest part in the song, so
        # this is never negative unless the part is early and alone in being
        # so, which cannot happen: it is what set the lead.
        shift = int(round((float(track.get('offset', 0)) / 1000.0 + lead)
                          * SAMPLE_RATE))                     # milliseconds
        runs, total = phrases(track.get('notes') or [])
        length = max(0.0, total - start) * spb + TAIL_SECONDS
        out = np.zeros(int(round(length * SAMPLE_RATE)) + max(0, shift),
                       dtype=np.float32)
        was_over = 0.0                   # where the phrase before this ended
        for at, run in runs:
            notes = []
            for e in run:
                ph = e.get('phonemes') or ['%']
                beats = float(e.get('beats', 0.5))
                notes.append(Note(int(e['pitch']), beats, ph,
                                  velocity=int(e.get('velocity', vel)),
                                  durations=syllable_lengths(
                                      ph, beats * 60000.0 / bpm, consonants)))
            lead = (anticipate(notes, bpm, consonants, at - was_over)
                    if early else 0.0)
            was_over = at + sum(n.beats for n in notes) - lead
            # a marked rest to scale the last syllable against, and to let it
            # decay rather than being cut off
            notes.append(Note(notes[-1].midi, 0.4, ['%'], velocity=1))
            at -= lead                   # the first note's consonants
            span = sum(n.beats for n in notes)
            if at + span <= start:
                continue                 # over and done with before the cursor
            ev = bend_events([(t * spb, v, sl) for t, v, sl in bends],
                             at * spb, (at + span) * spb, detune)
            # render_live is what applies the bends; with no events it produces
            # the same samples as render, checked against it
            renderer = Renderer(program=program, bpm=bpm, voice=voice,
                                voice_id=voice_id, level=level)
            y = renderer.render_live(notes, [0] * len(notes), ev,
                                     lambda _t: bpm)
            if renderer.stopped_short:
                self.stopped_short = True
            i = int(round((at - start) * spb * SAMPLE_RATE)) + shift
            if i < 0:                     # the phrase began before the cursor,
                y = y[-i:]                # or was moved back past the start
                i = 0
            if not len(y):
                continue
            if i + len(y) > len(out):
                out = np.concatenate(
                    [out, np.zeros(i + len(y) - len(out), dtype=np.float32)])
            out[i:i + len(y)] += y
        return out


if __name__ == '__main__':
    # a quick look at the engine from the command line
    eng = Engine()
    print(engine_name())
    print('voices:', ', '.join(eng.voices()[:6]), '...')
    print('daisy:', eng.phonemes(['daisy']))
