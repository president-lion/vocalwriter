"""Run with: python -m unittest discover -s tests."""
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

import wx

from app import studio


class FakeDialog(object):
    """AddWordDialog as far as `on_word_over` uses it: a word, and Ok."""

    seen = {}

    def __init__(self, rows, ok=True):
        self.rows = rows
        self.ok = ok

    def __call__(self, parent, studio_, pitch, beats, over=None):
        FakeDialog.seen = {'pitch': pitch, 'beats': beats, 'over': list(over)}
        return self

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def ShowModal(self):
        return wx.ID_OK if self.ok else wx.ID_CANCEL

    def result(self):
        return self.rows


class WordOverNotes(unittest.TestCase):
    def editor(self, notes, rows=None):
        chosen = list(range(len(notes))) if rows is None else rows
        return SimpleNamespace(
            notes=notes, selected=lambda: chosen, list=object(),
            say=Mock(), touch=Mock(), sync_lengths=Mock(),
            announce_note=Mock())

    def written(self, *groups):
        return [studio.Note(g, 0, 0, w) for g, w in groups]

    def run_it(self, frame, rows, ok=True):
        with patch.object(studio, 'AddWordDialog', FakeDialog(rows, ok)):
            studio.Frame.on_word_over(frame, None)

    def test_the_notes_keep_their_pitch_and_their_length(self):
        notes = [studio.Note(['%'], 60, 0.25), studio.Note(['%'], 67, 1.5)]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['d', 'EY'], 'daisy'),
                                        (['z', 'IY'], '')))
        self.assertEqual([(n.pitch, n.beats) for n in notes],
                         [(60, 0.25), (67, 1.5)])

    def test_only_what_they_sing_changes(self):
        notes = [studio.Note(['%'], 60, 0.25), studio.Note(['%'], 67, 1.5)]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['d', 'EY'], 'daisy'),
                                        (['z', 'IY'], '')))
        self.assertEqual([n.phonemes for n in notes],
                         [['d', 'EY'], ['z', 'IY']])
        self.assertEqual([n.word for n in notes], ['daisy', ''])

    def test_the_dialog_is_handed_the_notes_it_is_writing_over(self):
        notes = [studio.Note(['%'], 62, 0.5), studio.Note(['%'], 64, 0.25)]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['l', 'AA'], 'la')))
        self.assertEqual(FakeDialog.seen['over'], notes)
        self.assertEqual((FakeDialog.seen['pitch'], FakeDialog.seen['beats']),
                         (62, 0.5))

    def test_a_note_the_word_does_not_reach_is_left_alone(self):
        notes = [studio.Note(['%'], 60, 0.25),
                 studio.Note(['m', 'IY'], 62, 0.5, 'me')]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['k', 'AE', 't'], 'cat')))
        self.assertEqual(notes[1].phonemes, ['m', 'IY'])
        self.assertEqual(notes[1].word, 'me')

    def test_and_says_so(self):
        notes = [studio.Note(['%'], 60, 0.25), studio.Note(['%'], 62, 0.5)]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['k', 'AE', 't'], 'cat')))
        self.assertEqual(frame.announce_note.call_args[0][0],
                         'cat over 1 note, and 1 left as it was')

    def test_a_word_that_fits_says_only_what_it_did(self):
        notes = [studio.Note(['%'], 60, 0.25), studio.Note(['%'], 62, 0.5)]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['d', 'EY'], 'daisy'),
                                        (['z', 'IY'], '')))
        self.assertEqual(frame.announce_note.call_args[0][0],
                         'daisy over 2 notes')

    def test_nothing_selected_asks_for_a_note(self):
        frame = self.editor([studio.Note(['%'], 60, 0.25)], rows=[])
        self.run_it(frame, self.written((['x'], 'x')))
        frame.touch.assert_not_called()
        self.assertIn('select a note', frame.say.call_args[0][0])

    def test_cancelling_changes_nothing(self):
        notes = [studio.Note(['%'], 60, 0.25)]
        frame = self.editor(notes)
        self.run_it(frame, self.written((['d', 'EY'], 'daisy')), ok=False)
        self.assertEqual(notes[0].phonemes, ['%'])
        frame.touch.assert_not_called()


if __name__ == '__main__':
    unittest.main()
