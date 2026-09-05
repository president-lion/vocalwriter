"""Run with: python -m unittest discover -s tests."""
import unittest
from types import SimpleNamespace
from unittest.mock import Mock

from ppc import engine as ppc_engine
from app import studio


class RowText(unittest.TestCase):
    """What a note's row says, which is read out in full on every arrow key."""

    def test_a_length_is_its_name(self):
        self.assertEqual(studio.length_text(0.5), 'eighth note')
        self.assertEqual(studio.length_text(1.0), 'quarter note')

    def test_a_length_with_no_name_is_a_number_of_beats(self):
        self.assertEqual(studio.length_text(1.25), '1.25 beats')

    def test_a_triplet_has_a_name_too(self):
        self.assertEqual(studio.length_text(studio.BRUSHES['9']),
                         'eighth triplet note')

    def test_only_bend_and_bar_keep_a_heading(self):
        # a heading is read before the value under it, so the ones that would
        # only repeat what the value already says are blank
        self.assertEqual([head for head, _width in studio.COLUMNS],
                         ['Phonemes', '', '', '', 'Bend', 'Bar'])


class WhatWasDone(unittest.TestCase):
    """"12 notes copied", and the note itself when there is only one."""

    def notes(self, n):
        return [studio.Note(['AA'], 60, 0.5) for _ in range(n)]

    def test_several(self):
        self.assertEqual(studio.note_count(self.notes(12), 'copied'),
                         '12 notes copied')

    def test_one_says_which(self):
        self.assertEqual(
            studio.note_count([studio.Note(['t', 'EH'], 60, 0.5)], 'pasted'),
            't EH pasted')

    def test_a_rest_is_named_as_one(self):
        self.assertEqual(studio.note_count([studio.Note([], 60, 0.5)], 'cut'),
                         '(rest) cut')


class SelectionCount(unittest.TestCase):
    """Said once the selecting has stopped, and only when there is a count."""

    def frame(self, rows, counted=0):
        return SimpleNamespace(selected=lambda: rows, list=object(),
                               _counted=counted, announce_state=Mock())

    def test_several_notes_are_counted(self):
        frame = self.frame([0, 1, 2])
        studio.Frame.say_selection(frame)
        self.assertEqual(frame.announce_state.call_args[0][0],
                         '3 notes selected')

    def test_one_note_is_the_row_the_cursor_is_on_and_says_nothing(self):
        frame = self.frame([4])
        studio.Frame.say_selection(frame)
        frame.announce_state.assert_not_called()

    def test_the_same_count_twice_is_not_said_twice(self):
        frame = self.frame([0, 1, 2], counted=3)
        studio.Frame.say_selection(frame)
        frame.announce_state.assert_not_called()

    def test_and_the_count_is_remembered(self):
        frame = self.frame([0, 1])
        studio.Frame.say_selection(frame)
        self.assertEqual(frame._counted, 2)


class StepTrack(unittest.TestCase):
    """Left and right move between parts without leaving the notes."""

    def frame(self, names, current=0):
        tracks = [SimpleNamespace(name=n, notes=[]) for n in names]
        return SimpleNamespace(
            tracks=tracks, current=current, list=object(),
            tracks_list=SimpleNamespace(Select=Mock(), Focus=Mock()),
            announce_state=Mock())

    def test_forward(self):
        frame = self.frame(['1', '2', '3'])
        studio.Frame.step_track(frame, 1)
        frame.tracks_list.Select.assert_called_once_with(1)
        self.assertEqual(frame.announce_state.call_args[0][0], '2, 0 notes')

    def test_back(self):
        frame = self.frame(['1', '2', '3'], current=2)
        studio.Frame.step_track(frame, -1)
        frame.tracks_list.Select.assert_called_once_with(1)

    def test_the_ends_do_not_wrap(self):
        frame = self.frame(['1', '2'], current=0)
        studio.Frame.step_track(frame, -1)
        frame.tracks_list.Select.assert_not_called()
        self.assertEqual(frame.announce_state.call_args[0][0],
                         'the first track')

    def test_the_far_end_says_which_end(self):
        frame = self.frame(['1', '2'], current=1)
        studio.Frame.step_track(frame, 1)
        self.assertEqual(frame.announce_state.call_args[0][0],
                         'the last track')

    def test_one_track_says_so(self):
        frame = self.frame(['1'])
        studio.Frame.step_track(frame, 1)
        self.assertEqual(frame.announce_state.call_args[0][0],
                         'this song has one track')


class BendReach(unittest.TestCase):
    """How far a marker may go, and what the engine will actually bend."""

    def test_the_editor_offers_exactly_what_the_engine_will_play(self):
        # anything past the engine's range is clamped down to it rather than
        # sung, so a marker that could be set past it would be a lie
        self.assertEqual(studio.BEND_LIMIT, ppc_engine.BEND_RANGE)

    def test_and_it_is_more_than_the_octave_it_was(self):
        self.assertGreater(studio.BEND_LIMIT, 12.0)


class Jumps(unittest.TestCase):
    def test_the_number_row_walks_the_note_in_tens(self):
        self.assertEqual(studio.EnvelopeDialog.JUMPS['1'], 10)
        self.assertEqual(studio.EnvelopeDialog.JUMPS['0'], 100)


if __name__ == '__main__':
    unittest.main()
