"""Run with: python -m unittest discover -s tests."""
import unittest
from types import SimpleNamespace
from unittest.mock import Mock

import wx

from app import studio


class Brushes(unittest.TestCase):
    """Nine lengths on nine number keys, and what each one is worth."""

    def test_every_key_from_one_to_nine_paints_something(self):
        self.assertEqual(sorted(studio.BRUSHES), list('123456789'))

    def test_the_lengths_are_what_they_are_called(self):
        said = {k: studio.brush_name(b) for k, b in studio.BRUSHES.items()}
        self.assertEqual(said, {
            '1': 'whole', '2': 'half', '3': 'thirty-second', '4': 'quarter',
            '5': 'quarter triplet', '6': 'sixteenth',
            '7': 'sixteenth triplet', '8': 'eighth', '9': 'eighth triplet'})

    def test_three_triplets_fill_the_note_above_them(self):
        for triplet, whole in (('5', '2'), ('9', '4'), ('7', '8')):
            with self.subTest(triplet=triplet):
                self.assertAlmostEqual(3 * studio.BRUSHES[triplet],
                                       studio.BRUSHES[whole])

    def test_six_is_the_sixteenth_it_always_was(self):
        self.assertEqual(studio.BRUSHES['6'], studio.SIXTEENTH)


class BrushKeys(unittest.TestCase):
    """The number row and the keypad both."""

    def test_the_number_row(self):
        self.assertEqual(studio.brush_key(ord('7')), '7')

    def test_the_keypad(self):
        self.assertEqual(studio.brush_key(wx.WXK_NUMPAD7), '7')

    def test_zero_is_not_a_brush(self):
        self.assertEqual(studio.brush_key(ord('0')), '')

    def test_a_letter_is_not_a_brush(self):
        self.assertEqual(studio.brush_key(ord('J')), '')


class Remembered(unittest.TestCase):
    """A brush kept in the settings file, read back as one of the nine."""

    def test_a_known_length_comes_back(self):
        self.assertEqual(studio.brush_beats(2.0), 2.0)

    def test_a_triplet_survives_the_rounding_of_a_json_file(self):
        self.assertEqual(studio.brush_beats(0.666667),
                         studio.BRUSHES['5'])

    def test_a_length_that_is_no_longer_a_brush_falls_back(self):
        self.assertEqual(studio.brush_beats(0.9), studio.SIXTEENTH)

    def test_nothing_at_all_falls_back(self):
        self.assertEqual(studio.brush_beats(None), studio.SIXTEENTH)
        self.assertEqual(studio.brush_beats('half'), studio.SIXTEENTH)


class SteppedLength(unittest.TestCase):
    """A step of whatever is in hand, refused when it would vanish."""

    def test_a_step_of_the_brush(self):
        self.assertEqual(studio.stepped_length(1.0, True, 2.0), 3.0)
        self.assertEqual(studio.stepped_length(3.0, False, 2.0), 1.0)

    def test_the_default_is_still_a_sixteenth(self):
        self.assertEqual(studio.stepped_length(1.0, True), 1.25)

    def test_too_short_is_refused_rather_than_clamped(self):
        self.assertIsNone(studio.stepped_length(0.25, False, 4.0))

    def test_the_floor_itself_is_allowed(self):
        self.assertEqual(studio.stepped_length(0.25, False, 0.125),
                         studio.MIN_BEATS)


class Snapping(unittest.TestCase):
    """Land on the brush's own grid rather than adding to what is there."""

    def walk(self, start, step, presses, up=True, snap=True):
        """The lengths a run of presses gives, in beats."""
        out, beats = [], start
        for _ in range(presses):
            beats = studio.stepped_length(beats, up, step, snap)
            if beats is None:
                out.append(None)
                break
            out.append(beats)
        return out

    def test_an_eighth_brush_draws_eighths_from_wherever_it_starts(self):
        # a sixteenth, then the eighth brush: without snapping this walks
        # 0.75, 1.25, 1.75 -- an eighth grid a sixteenth out, for ever
        self.assertEqual(self.walk(0.25, 0.5, 3), [0.5, 1.0, 1.5])

    def test_and_without_snapping_it_does_not(self):
        self.assertEqual(self.walk(0.25, 0.5, 3, snap=False),
                         [0.75, 1.25, 1.75])

    def test_a_note_already_on_the_grid_moves_one_line(self):
        self.assertEqual(self.walk(0.5, 0.5, 3), [1.0, 1.5, 2.0])

    def test_going_back_down_lands_on_the_lines_too(self):
        self.assertEqual(self.walk(1.75, 0.5, 3, up=False), [1.5, 1.0, 0.5])

    def test_the_line_below_the_floor_is_refused_not_clamped(self):
        self.assertEqual(self.walk(0.5, 0.5, 2, up=False), [None])

    def test_a_triplet_brush_snaps_to_thirds(self):
        got = self.walk(0.25, studio.BRUSHES['9'], 3)
        self.assertEqual([studio.spoken_length(b, (4, 4)) for b in got],
                         ['eighth triplet note', 'quarter triplet note',
                          'quarter note'])

    def test_a_note_written_as_a_triplet_is_treated_as_on_the_line(self):
        # 1/3 kept to six places is 0.999999 of a step when the sum is done
        # again, and must move on rather than back on to where it already is
        step = studio.BRUSHES['9']
        self.assertAlmostEqual(
            studio.stepped_length(round(step, 6), True, step, True), 2 * step,
            places=5)

    def test_the_whole_note_brush_from_a_sixteenth(self):
        self.assertEqual(self.walk(0.25, 4.0, 2), [4.0, 8.0])


class NudgingUsesTheBrush(unittest.TestCase):
    def editor(self, brush, beats=0.25, notes=1, snap=False):
        return SimpleNamespace(
            notes=[studio.Note(pitch=60, beats=beats) for _ in range(notes)],
            selected=lambda: list(range(notes)), signature=lambda: (4, 4),
            brush=brush, snap=snap, list=object(), touch=Mock(),
            refresh_row=Mock(),
            sync_lengths=Mock(), preview_note=Mock(), announce_note=Mock())

    def test_a_whole_note_brush_adds_a_whole_note(self):
        frame = self.editor(studio.BRUSHES['1'])
        studio.Frame.nudge_length(frame, True)
        self.assertEqual(frame.notes[0].beats, 4.25)

    def test_a_triplet_brush_adds_a_triplet(self):
        frame = self.editor(studio.BRUSHES['9'], beats=1.0 / 3.0)
        studio.Frame.nudge_length(frame, True)
        self.assertEqual(studio.spoken_length(frame.notes[0].beats, (4, 4)),
                         'quarter triplet note')

    def test_the_article_suits_the_name(self):
        self.assertEqual(studio.a_brush(studio.BRUSHES['8']), 'an eighth')
        self.assertEqual(studio.a_brush(studio.BRUSHES['4']), 'a quarter')

    def test_several_notes_are_told_which_brush_moved_them(self):
        frame = self.editor(studio.BRUSHES['2'], notes=3)
        studio.Frame.nudge_length(frame, True)
        self.assertEqual(frame.announce_note.call_args[0][0],
                         '3 notes a half longer')


if __name__ == '__main__':
    unittest.main()
