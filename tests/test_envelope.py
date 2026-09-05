"""Run with: python -m unittest discover -s tests."""
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

from app import studio


class Envelope(object):
    """The dialog's own arithmetic, without a window around it."""

    value_at = studio.EnvelopeDialog.value_at
    row_text = studio.EnvelopeDialog.row_text
    result = studio.EnvelopeDialog.result
    bent = studio.EnvelopeDialog.bent

    def __init__(self, markers):
        self.markers = dict(markers)


class Editor(Envelope):
    """The same, plus the parts that draw and speak, as mocks."""

    shift_pitch = studio.EnvelopeDialog.shift_pitch
    drop_marker = studio.EnvelopeDialog.drop_marker
    clear = studio.EnvelopeDialog.clear

    def __init__(self, markers):
        Envelope.__init__(self, markers)
        self.list = object()
        self.refresh_from = Mock()
        self.sound = Mock()
        self.studio = SimpleNamespace(announce_state=Mock())

    def said(self):
        calls = self.studio.announce_state.call_args_list
        return calls[-1][0][0] if calls else None


class SettingThePitch(unittest.TestCase):
    """Up and down, which make a marker where the cursor is."""

    def setUp(self):
        patcher = patch.object(studio, 'reannounce')
        self.reannounce = patcher.start()
        self.addCleanup(patcher.stop)

    def test_a_position_with_no_marker_gets_one(self):
        env = Editor({0: 0.0})
        env.shift_pitch(35, 1.0)
        self.assertEqual(env.markers[35], 1.0)
        self.assertEqual(env.said(), 'marker added, 35 per cent, plus 1')

    def test_it_starts_from_what_the_bend_already_does_there(self):
        # after a marker of +1 the line holds at +1, so up is +2
        env = Editor({0: 0.0, 35: 1.0})
        env.shift_pitch(60, 1.0)
        self.assertEqual(env.markers[60], 2.0)

    def test_and_between_two_markers_from_the_line_between_them(self):
        env = Editor({0: 0.0, 100: 4.0})
        env.shift_pitch(50, 1.0)
        self.assertEqual(env.markers[50], 3.0)

    def test_moving_one_that_exists_re_reads_the_row_rather_than_announcing(self):
        env = Editor({0: 0.0, 60: 2.0})
        env.shift_pitch(60, 1.0)
        self.assertEqual(env.markers[60], 3.0)
        env.studio.announce_state.assert_not_called()
        self.reannounce.assert_called_once()

    def test_down_goes_down(self):
        env = Editor({0: 0.0})
        env.shift_pitch(20, -2.0)
        self.assertEqual(env.markers[20], -2.0)

    def test_it_stops_where_the_engine_stops(self):
        env = Editor({0: studio.BEND_LIMIT})
        env.shift_pitch(50, 1.0)
        self.assertEqual(env.markers[50], studio.BEND_LIMIT)

    def test_and_says_so_when_a_press_can_do_nothing_more(self):
        env = Editor({0: studio.BEND_LIMIT, 50: studio.BEND_LIMIT})
        env.shift_pitch(50, 1.0)
        self.assertIn('as far as the engine will bend', env.said())
        env.refresh_from.assert_not_called()

    def test_every_press_asks_for_the_note_to_be_played(self):
        env = Editor({0: 0.0})
        env.shift_pitch(35, 1.0)
        env.sound.assert_called_once()


class TheStartingMarker(unittest.TestCase):
    """The one at nought per cent, which needs asking twice to remove."""

    def test_plain_delete_will_not_have_it(self):
        env = Editor({0: 0.0, 50: 2.0})
        env.drop_marker(0)
        self.assertIn(0, env.markers)
        self.assertIn('where the note starts', env.said())

    def test_control_delete_will(self):
        env = Editor({0: 0.0, 50: 2.0})
        env.drop_marker(0, forced=True)
        self.assertNotIn(0, env.markers)

    def test_any_other_marker_goes_on_a_plain_delete(self):
        env = Editor({0: 0.0, 50: 2.0})
        env.drop_marker(50)
        self.assertNotIn(50, env.markers)

    def test_clearing_leaves_the_start_and_means_no_bend(self):
        env = Editor({0: 0.0, 50: 2.0, 100: -1.0})
        env.clear()
        self.assertEqual(env.markers, {0: 0.0})
        self.assertEqual(env.result(), [])

    def test_and_clearing_nothing_says_so(self):
        env = Editor({0: 0.0})
        env.clear()
        self.assertEqual(env.said(), 'there is no bend to clear')
        env.refresh_from.assert_not_called()


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

    def test_the_starting_marker_on_its_own_is_no_bend_either(self):
        # it is put there when the dialog opens, so a note this was merely
        # looked at must come out of it with the bend it went in with
        self.assertEqual(Envelope({0: 0.0}).result(), [])

    def test_but_a_starting_marker_that_was_moved_is_a_bend(self):
        self.assertEqual(Envelope({0: -2.0}).result(), [(0.0, -2.0)])

    def test_and_so_is_a_start_with_anything_after_it(self):
        self.assertEqual(Envelope({0: 0.0, 100: 3.0}).result(),
                         [(0.0, 0.0), (1.0, 3.0)])


if __name__ == '__main__':
    unittest.main()
