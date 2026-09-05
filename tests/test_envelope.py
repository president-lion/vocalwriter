"""Run with: python -m unittest discover -s tests."""
import unittest

from app import studio


class Envelope(object):
    """The dialog's own arithmetic, without a window around it."""

    value_at = studio.EnvelopeDialog.value_at
    row_text = studio.EnvelopeDialog.row_text
    result = studio.EnvelopeDialog.result

    def __init__(self, markers):
        self.markers = dict(markers)


class ValueAt(unittest.TestCase):
    """What the pitch is doing at a position between the markers."""

    def test_nothing_written_says_nothing_anywhere(self):
        self.assertIsNone(Envelope({}).value_at(50))

    def test_before_the_first_marker_is_not_this_note_to_say(self):
        # the note starts on whatever the note before it left behind
        self.assertIsNone(Envelope({20: 1.0}).value_at(19))

    def test_a_marker_is_its_own_value(self):
        self.assertEqual(Envelope({20: 1.0}).value_at(20), 1.0)

    def test_between_two_markers_it_is_the_line_between_them(self):
        env = Envelope({0: 0.0, 50: 2.0})
        self.assertAlmostEqual(env.value_at(25), 1.0)
        self.assertAlmostEqual(env.value_at(10), 0.4)

    def test_the_line_goes_down_as_readily_as_up(self):
        env = Envelope({50: 2.0, 100: -1.0})
        self.assertAlmostEqual(env.value_at(75), 0.5)

    def test_after_the_last_marker_it_holds(self):
        env = Envelope({0: 0.0, 40: -2.0})
        self.assertEqual(env.value_at(41), -2.0)
        self.assertEqual(env.value_at(100), -2.0)


class RowText(unittest.TestCase):
    """What is read out as the cursor lands on a position."""

    def test_a_bare_position(self):
        self.assertEqual(Envelope({}).row_text(37), '37 per cent')

    def test_a_marker_says_so(self):
        self.assertEqual(Envelope({50: 2.0}).row_text(50),
                         '50 per cent, plus 2, marker')

    def test_between_markers_says_where_the_pitch_is(self):
        self.assertEqual(Envelope({0: 0.0, 50: 2.0}).row_text(25),
                         '25 per cent, plus 1')

    def test_a_marker_on_the_written_pitch_says_that(self):
        self.assertEqual(Envelope({0: 0.0}).row_text(0),
                         '0 per cent, on the note, marker')


class Semitones(unittest.TestCase):
    def test_sharp_flat_and_neither(self):
        self.assertEqual(studio.semitone_text(2), 'plus 2')
        self.assertEqual(studio.semitone_text(-0.5), 'minus 0.5')
        self.assertEqual(studio.semitone_text(0), 'on the note')

    def test_a_rounding_error_is_still_on_the_note(self):
        self.assertEqual(studio.semitone_text(1e-9), 'on the note')


class Jumps(unittest.TestCase):
    """The number row walks the note in tens, nought being the far end."""

    def test_every_key_lands_on_its_ten(self):
        self.assertEqual(studio.EnvelopeDialog.JUMPS,
                         {'1': 10, '2': 20, '3': 30, '4': 40, '5': 50,
                          '6': 60, '7': 70, '8': 80, '9': 90, '0': 100})


class Result(unittest.TestCase):
    """Per cents on the way in, fractions of the note on the way out."""

    def test_sorted_and_scaled(self):
        self.assertEqual(Envelope({100: -1.0, 0: 0.0, 50: 2.0}).result(),
                         [(0.0, 0.0), (0.5, 2.0), (1.0, -1.0)])

    def test_nothing_drawn_is_no_bend_at_all(self):
        self.assertEqual(Envelope({}).result(), [])


if __name__ == '__main__':
    unittest.main()
