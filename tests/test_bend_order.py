"""Run with: python -m unittest discover -s tests."""
import unittest

from ppc import engine as ppc_engine
from app import project


class Note(object):
    """As much of a note as project.timeline reads."""

    def __init__(self, beats=1.0, bend=None):
        self.beats = beats
        self.bend = list(bend or ())


class TimeOrder(unittest.TestCase):
    """Bend points in the order they happen, ties in the order written."""

    def test_points_come_out_in_time(self):
        got = ppc_engine.in_time_order([(2.0, 1.0, False), (0.0, 3.0, False)])
        self.assertEqual([t for t, _v, _sl in got], [0.0, 2.0])

    def test_two_at_the_same_instant_keep_the_order_they_arrived_in(self):
        # the note's own last point, then the next note's reset. Sorting the
        # whole triple put the smaller *value* first, which is not an order.
        written = [(1.0, 12.0, False), (1.0, 0.0, False)]
        self.assertEqual(ppc_engine.in_time_order(written), written)

    def test_and_that_is_not_what_sorting_the_triple_does(self):
        written = [(1.0, 12.0, False), (1.0, 0.0, False)]
        self.assertNotEqual(sorted(written), written)

    def test_a_point_written_without_a_slide_flag_still_works(self):
        got = ppc_engine.in_time_order([(0.5, 2.0)])
        self.assertEqual(got, [(0.5, 2.0, False)])


class ANoteAddedAfterABentOne(unittest.TestCase):
    """The bug: a bend that inverted itself the moment a note followed it.

    A note whose bend runs to its end has its last point exactly where the
    next note starts, and the next note asks for a zero at that same instant
    so the bend does not carry on into it. Ordered by value rather than by
    time, the zero came first: the slide ran down to nothing instead of up to
    where it was written, and the value it should have finished on was left
    in force with nothing after it to clear it.
    """

    def line(self, notes):
        return ppc_engine.in_time_order(project.timeline(notes))

    def bent(self):
        return Note(2.0, [(0.0, 0.0), (1.0, 12.0)])

    def test_alone_it_climbs(self):
        curve = ppc_engine.glide(self.line([self.bent()]))
        self.assertAlmostEqual(curve[0][1], 0.0)
        self.assertAlmostEqual(curve[-1][1], 12.0)

    def test_the_climb_survives_a_note_being_added_after_it(self):
        curve = ppc_engine.glide(self.line([self.bent(), Note(2.0)]))
        rise = [v for t, v in curve if t < 2.0]
        self.assertAlmostEqual(rise[0], 0.0)
        self.assertGreater(max(rise), 11.0)
        # and it climbs the whole way rather than turning back
        self.assertEqual(rise, sorted(rise))

    def test_and_the_note_after_it_is_left_on_its_own_pitch(self):
        curve = ppc_engine.glide(self.line([self.bent(), Note(2.0)]))
        self.assertAlmostEqual(curve[-1][1], 0.0)
        after = [v for t, v in curve if t >= 2.0]
        self.assertTrue(all(abs(v) < 1e-9 for v in after[1:]),
                        'the bend went on being held after the note ended')

    def test_the_reset_is_only_written_when_there_is_one_to_undo(self):
        # a note whose bend finishes where it started needs no zero after it
        even = Note(2.0, [(0.0, 0.0), (1.0, 0.0)])
        line = self.line([even, Note(2.0)])
        self.assertEqual([t for t, _v, _sl in line], [0.0, 2.0])

    def test_a_bend_that_ends_flat_was_never_reordered(self):
        # -2 sorts before the 0 that follows it, which is why this only ever
        # went wrong on a note bent upwards
        line = self.line([Note(2.0, [(0.0, 0.0), (1.0, -2.0)]), Note(2.0)])
        at_two = [(t, v) for t, v, _sl in line if t == 2.0]
        self.assertEqual([v for _t, v in at_two], [-2.0, 0.0])


if __name__ == '__main__':
    unittest.main()
