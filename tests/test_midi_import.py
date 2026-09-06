"""Run with: python -m unittest discover -s tests."""
import os
import struct
import tempfile
import unittest

from app import project
from tools.smf import MidiFile


def _vlq(n):
    out = bytearray([n & 0x7F])
    n >>= 7
    while n:
        out.insert(0, 0x80 | (n & 0x7F))
        n >>= 7
    return bytes(out)


def _track(events, name=None):
    """`events` are (tick, bytes); the ticks are absolute."""
    body = b''
    at = 0
    if name is not None:
        body += _vlq(0) + b'\xFF\x03' + _vlq(len(name)) + name.encode()
    for tick, data in sorted(events, key=lambda e: e[0]):
        body += _vlq(tick - at) + data
        at = tick
    body += _vlq(0) + b'\xFF\x2F\x00'
    return b'MTrk' + struct.pack('>I', len(body)) + body


def _notes(pairs, chan=0):
    """(tick, pitch, length) each way round, on one channel."""
    out = []
    for tick, pitch, length in pairs:
        out.append((tick, bytes([0x90 | chan, pitch, 100])))
        out.append((tick + length, bytes([0x80 | chan, pitch, 0])))
    return out


def _file(tracks, fmt=1, division=480):
    return (b'MThd' + struct.pack('>IHHH', 6, fmt, len(tracks), division)
            + b''.join(tracks))


def written(blob):
    fh, path = tempfile.mkstemp(suffix='.mid')
    os.write(fh, blob)
    os.close(fh)
    return path


class OneFileWithFourAlikeTracks(unittest.TestCase):
    """The bug: a Reaper export whose parts are all named after the plugin.

    Four tracks, four different lines, one name between them. Picking a part
    by its name gave whichever came first, so importing every part gave four
    editor tracks with the same notes on them.
    """

    def setUp(self):
        lines = [[(0, 60, 480)], [(0, 55, 480)], [(0, 52, 480)],
                 [(0, 48, 480)]]
        self.path = written(_file(
            [_track([], 'song')]
            + [_track(_notes(line), 'ReaSynth') for line in lines]))

    def tearDown(self):
        os.remove(self.path)

    def test_every_part_is_offered(self):
        self.assertEqual(len(project.midi_parts(self.path)), 4)

    def test_and_they_are_told_apart_by_name(self):
        names = [p['name'] for p in project.midi_parts(self.path)]
        self.assertEqual(len(set(names)), 4)

    def test_and_each_one_brings_its_own_notes(self):
        _bpm, _sig, got = project.from_midi_parts(self.path, [0, 1, 2, 3])
        self.assertEqual([rows[0][1] for _n, rows, _p in got],
                         [60, 55, 52, 48])

    def test_one_part_on_its_own_is_the_part_that_was_asked_for(self):
        _bpm, rows, _pend, _sig = project.from_midi(self.path, 2)
        self.assertEqual(rows[0][1], 52)

    def test_asking_for_a_part_that_is_not_there_says_so(self):
        with self.assertRaises(ValueError):
            project.from_midi(self.path, 9)


class OneTrackWithSeveralChannels(unittest.TestCase):
    """A format 0 file is one track carrying every part at once."""

    def setUp(self):
        self.path = written(_file([_track(
            _notes([(0, 60, 480)], chan=0)
            + _notes([(0, 48, 480)], chan=3), 'everything')], fmt=0))

    def tearDown(self):
        os.remove(self.path)

    def test_each_channel_is_its_own_part(self):
        parts = project.midi_parts(self.path)
        self.assertEqual([p['channel'] for p in parts], [0, 3])

    def test_and_they_do_not_arrive_piled_into_one_line(self):
        _bpm, _sig, got = project.from_midi_parts(self.path, [0, 1])
        self.assertEqual([rows[0][1] for _n, rows, _p in got], [60, 48])


class APartThatPlaysChords(unittest.TestCase):
    """A voice sings one note at a time, so a chord is its top note."""

    def setUp(self):
        self.path = written(_file([_track(
            _notes([(0, 60, 480), (0, 64, 480), (0, 67, 480),
                    (480, 62, 480)]), 'chords')]))

    def tearDown(self):
        os.remove(self.path)

    def test_a_chord_is_one_note_and_not_three_in_a_row(self):
        _bpm, rows, _pend, _sig = project.from_midi(self.path)
        self.assertEqual([(r[1], r[2]) for r in rows], [(67, 1.0), (62, 1.0)])

    def test_a_note_held_under_the_next_one_is_cut_short_there(self):
        path = written(_file([_track(
            _notes([(0, 60, 1920), (480, 72, 480)]), 'over')]))
        try:
            _bpm, rows, _pend, _sig = project.from_midi(path)
        finally:
            os.remove(path)
        self.assertEqual([(r[1], r[2]) for r in rows], [(60, 1.0), (72, 1.0)])


class WhereThePartsStart(unittest.TestCase):
    """The bug: every part began at beat one, whenever it really began.

    A melody that comes in on the third beat over a bass that comes in on the
    first is a melody a bar late, and importing both put them on top of each
    other -- the one thing an accompaniment must not do.
    """

    def setUp(self):
        self.path = written(_file([
            _track(_notes([(960, 60, 480)]), 'late'),
            _track(_notes([(0, 48, 480)]), 'early')]))

    def tearDown(self):
        os.remove(self.path)

    def test_silence_before_the_first_note_is_kept_as_a_rest(self):
        _bpm, rows, _pend, _sig = project.from_midi(self.path, 0)
        self.assertEqual(rows[0][0], ['%'])
        self.assertEqual(rows[0][2], 2.0)

    def test_a_part_that_starts_on_the_beat_gets_no_rest(self):
        _bpm, rows, _pend, _sig = project.from_midi(self.path, 1)
        self.assertEqual(rows[0][1], 48)


class RoundingDoesNotAccumulate(unittest.TestCase):
    """Where a note falls is its own position, not the sum of what came first.

    Sixteen notes each a hair short of a beat used to be rounded one at a
    time and added up, so the part finished a beat and a half early. Each
    note's own moment is rounded instead, which cannot drift.
    """

    def setUp(self):
        pairs = [(k * 480, 60, 460) for k in range(16)]
        self.path = written(_file([_track(_notes(pairs), 'drifty')]))

    def tearDown(self):
        os.remove(self.path)

    def test_the_part_is_as_long_as_it_was_played(self):
        _bpm, rows, _pend, _sig = project.from_midi(self.path)
        self.assertEqual(sum(r[2] for r in rows), 16.0)
        self.assertEqual(len(rows), 16)


class WhatTheReaderKeeps(unittest.TestCase):

    def test_a_note_remembers_the_channel_it_was_on(self):
        path = written(_file([_track(_notes([(0, 60, 480)], chan=5), 'five')]))
        try:
            track = MidiFile.from_file(path).tracks[0]
        finally:
            os.remove(path)
        self.assertEqual(track.channels(), [5])
        self.assertEqual(track.on_channel(5).notes[0].pitch, 60)
        self.assertEqual(track.on_channel(0).notes, [])


if __name__ == '__main__':
    unittest.main()
