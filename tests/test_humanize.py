"""Run with: python -m unittest discover -s tests."""
import json
import os
import tempfile
import unittest

from ppc import engine as ppc_engine
from app import project


def part(**kw):
    """One track as the engine is handed it."""
    doc = {'program': 0, 'volume': 1.0, 'pan': 0,
           'notes': [{'pitch': 60, 'beats': 1.0, 'phonemes': ['AA']}]}
    doc.update(kw)
    return doc


class TrackHolds(unittest.TestCase):
    """A part keeps how far out of tune and how far off the beat it is."""

    def test_nothing_by_default(self):
        t = project.Track(name='1')
        self.assertEqual((t.detune, t.offset), (0, 0))

    def test_kept_as_given(self):
        t = project.Track(name='1', detune=-7, offset=25)
        self.assertEqual((t.detune, t.offset), (-7, 25))

    def test_clamped_to_something_a_singer_could_be(self):
        t = project.Track(name='1', detune=5000, offset=-9000)
        self.assertEqual((t.detune, t.offset),
                         (project.DETUNE_RANGE, -project.OFFSET_RANGE))

    def test_nonsense_is_nothing_rather_than_a_crash(self):
        t = project.Track(name='1', detune='sharp', offset=None)
        self.assertEqual((t.detune, t.offset), (0, 0))


class SavedAndRead(unittest.TestCase):
    """They survive a trip through a project file, and are absent until set."""

    def save(self, tracks):
        path = os.path.join(tempfile.mkdtemp(), 'song' + project.SUFFIX)
        project.save(path, 120, tracks)
        with open(path, encoding='utf-8') as fh:
            doc = json.load(fh)
        return doc, project.load(path)[1]

    def test_a_song_that_sets_neither_writes_neither(self):
        doc, _read = self.save([project.Track(name='1')])
        self.assertNotIn('detune', doc['tracks'][0])
        self.assertNotIn('offset', doc['tracks'][0])

    def test_what_was_set_comes_back(self):
        doc, read = self.save([project.Track(name='1', detune=-7, offset=25)])
        self.assertEqual((doc['tracks'][0]['detune'],
                          doc['tracks'][0]['offset']), (-7, 25))
        self.assertEqual((read[0]['detune'], read[0]['offset']), (-7, 25))

    def test_a_file_that_says_nothing_reads_as_nothing(self):
        _doc, read = self.save([project.Track(name='1')])
        self.assertEqual((read[0]['detune'], read[0]['offset']), (0, 0))


class Lead(unittest.TestCase):
    """The earliest part decides how much silence goes in front of the song."""

    def lead(self, *offsets):
        return ppc_engine.lead_seconds(
            {'tracks': [part(offset=ms) for ms in offsets]})

    def test_nothing_early_needs_no_room(self):
        self.assertEqual(self.lead(0, 20, 200), 0.0)

    def test_the_earliest_part_sets_it(self):
        self.assertAlmostEqual(self.lead(0, -40, -120), 0.120)

    def test_a_song_with_no_offsets_at_all_gets_none(self):
        self.assertEqual(ppc_engine.lead_seconds({'tracks': [part()]}), 0.0)


class DetuneRidesOnTheBend(unittest.TestCase):
    """Detune is a bend, because the bend is what moves the pitch."""

    def bends(self, points, detune=0.0):
        return ppc_engine.bend_events(points, 0.0, 10.0, detune)

    def test_no_bends_and_no_detune_sends_nothing(self):
        self.assertEqual(self.bends([]), [])

    def test_detune_alone_still_sets_the_pitch(self):
        ev = self.bends([], 0.05)
        self.assertEqual([kind for _t, kind, _v in ev], ['sens', 'bend'])
        self.assertGreater(ev[1][2], 0)         # sharp, and before the singing
        self.assertEqual(ev[1][0], -1.0)

    def test_flat_is_the_other_way(self):
        self.assertLess(self.bends([], -0.05)[1][2], 0)

    def test_a_bend_carries_the_detune_with_it(self):
        plain = self.bends([(0.5, 2.0, False)])
        moved = self.bends([(0.5, 2.0, False)], 0.05)
        self.assertGreater(max(v for _t, k, v in moved if k == 'bend'),
                           max(v for _t, k, v in plain if k == 'bend'))

    def test_zero_detune_leaves_a_bent_phrase_exactly_as_it_was(self):
        points = [(0.2, -1.0, True), (0.9, 1.5, False)]
        self.assertEqual(self.bends(points, 0.0), self.bends(points))


if __name__ == '__main__':
    unittest.main()
