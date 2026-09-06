"""Typing a line of lyrics onto notes that are already written.

The dialog is driven through its own key handlers rather than through wx, so
these run without a display and without the engine: what is being checked is
where each syllable lands, not what it sounds like. The lookup is stubbed, so
the phonemes here are whatever the stub says they are.
"""
import unittest
from unittest.mock import patch

import wx

from app import studio


DICTIONARY = {
    'day': ['d', 'EY'],
    'daisy': ['d', 'EY', 'z', 'IY'],
    'bicycle': ['b', 'AY', 's', 'IH', 'k', 'EL'],
    'zee': ['z', 'IY'],
}


class Key(object):
    """Enough of a key event for the handlers under test."""

    def __init__(self, code):
        self.code = code
        self.skipped = False

    def GetKeyCode(self):
        return self.code

    def Skip(self):
        self.skipped = True


class Box(object):
    """Enough of a text control to be typed into."""

    def __init__(self):
        self.text = ''

    def GetValue(self):
        return self.text

    def ChangeValue(self, text):
        self.text = text

    def SetInsertionPointEnd(self):
        pass


class Label(object):
    """The line under the box that says which note is next."""

    def __init__(self):
        self.text = ''

    def SetLabel(self, text):
        self.text = text


class Engine(object):
    """The dictionary and the renderer, answered at once rather than on a
    worker thread. The render is only what a preview plays, so it answers
    with enough of a result to be handed on and nothing more."""

    def __init__(self):
        self.rendered = []

    def phonemes(self, words, callback):
        callback({w: DICTIONARY[w] for w in words if w in DICTIONARY})

    def render(self, song, callback, out=None):
        self.rendered.append(song)
        callback({'audio': [0.0], 'rate': 44100, 'seconds': 0.0})


class Studio(object):
    """What the dialog asks of the window, and nothing else."""

    def __init__(self):
        self.engine = Engine()
        self.said = []
        self.played = 0

    def say(self, text):
        self.said.append(text)

    def announce_state(self, text, control=None, row=-1):
        self.said.append(text)

    def song(self, notes=None, start=0.0, tracks=None):
        return {'notes': notes}

    def play_audio(self, res):
        self.played += 1


def lyrics(notes, start=0):
    """A LyricDialog with no window behind it."""
    dlg = studio.LyricDialog.__new__(studio.LyricDialog)
    dlg.studio = Studio()
    dlg.notes = notes
    dlg.places = [i for i, n in enumerate(notes)
                  if not (n.phonemes and n.is_rest())]
    dlg.at = next((k for k, i in enumerate(dlg.places) if i >= start), 0)
    dlg.written = [None] * len(dlg.places)
    dlg.before = [(n.word, list(n.phonemes)) for n in notes]
    dlg.missing = []
    dlg.waiting = 0
    dlg.alive = True
    dlg.box = Box()
    dlg.where = Label()
    dlg.show_place = lambda speak=True: None
    return dlg


def type_in(dlg, text):
    """Type `text`, with space and hyphen going to the handlers."""
    for ch in text:
        if ch == ' ':
            dlg.on_key(Key(wx.WXK_SPACE))
        elif ch == '-':
            dlg.on_char(Key(studio.HYPHEN))
        else:
            dlg.box.ChangeValue(dlg.box.GetValue() + ch)


def backspace(dlg):
    """One backspace: a letter if there is one, else back a note."""
    evt = Key(wx.WXK_BACK)
    dlg.on_key(evt)
    if evt.skipped:
        dlg.box.ChangeValue(dlg.box.GetValue()[:-1])


def note(pitch=62, phonemes=None, beats=1.0):
    return studio.Note(list(phonemes or ()), pitch, beats)


class LyricLineTests(unittest.TestCase):
    """The lookup answers on a worker thread and comes back through
    `wx.CallAfter`, which needs an application. There is none here and the
    stub answers at once anyway, so the hop is taken out."""

    def setUp(self):
        patcher = patch.object(studio.wx, 'CallAfter',
                               lambda fn, *a, **kw: fn(*a, **kw))
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_space_writes_a_word_and_moves_on(self):
        notes = [note(), note()]
        dlg = lyrics(notes)
        type_in(dlg, 'day ')
        self.assertEqual(notes[0].word, 'day')
        self.assertEqual(notes[0].phonemes, ['d', 'EY'])
        self.assertEqual(dlg.at, 1)

    def test_a_finished_word_is_played(self):
        notes = [note()]
        dlg = lyrics(notes)
        type_in(dlg, 'day ')
        self.assertEqual(dlg.studio.played, 1)

    def test_a_hyphen_splits_a_word_over_two_notes(self):
        notes = [note(), note(64)]
        dlg = lyrics(notes)
        type_in(dlg, 'dai-sy ')
        self.assertEqual([n.word for n in notes], ['dai-', 'sy'])
        self.assertEqual(notes[0].phonemes, ['d', 'EY'])
        self.assertEqual(notes[1].phonemes, ['z', 'IY'])

    def test_a_hyphen_alone_does_not_look_the_word_up_yet(self):
        notes = [note(), note(64)]
        dlg = lyrics(notes)
        type_in(dlg, 'dai-')
        self.assertEqual(notes[0].word, 'dai-')
        self.assertEqual(notes[0].phonemes, [])   # not a word yet
        self.assertEqual(dlg.at, 1)

    def test_three_syllables_over_three_notes(self):
        notes = [note(), note(64), note(65)]
        dlg = lyrics(notes)
        type_in(dlg, 'bi-cy-cle ')
        self.assertEqual([n.word for n in notes], ['bi-', 'cy-', 'cle'])
        self.assertEqual([len(n.phonemes) > 0 for n in notes],
                         [True, True, True])
        joined = [p for n in notes for p in n.phonemes]
        self.assertEqual(joined, DICTIONARY['bicycle'])

    def test_more_hyphens_than_syllables_leaves_the_spare_notes(self):
        notes = [note(), note(64), note(65)]
        dlg = lyrics(notes)
        type_in(dlg, 'dai-s-y ')      # "daisy" cut into three, and it has two
        self.assertEqual(notes[0].phonemes, ['d', 'EY'])
        self.assertEqual(notes[1].phonemes, ['z', 'IY'])
        self.assertEqual(notes[2].phonemes, [])      # not turned into a rest
        self.assertTrue(any('2 syllables' in m for m in dlg.studio.said))

    def test_a_rest_is_stepped_over(self):
        notes = [note(), note(64, ['%']), note(65)]
        dlg = lyrics(notes)
        type_in(dlg, 'day zee ')
        self.assertEqual(notes[0].word, 'day')
        self.assertEqual(notes[1].word, '')          # the rest, untouched
        self.assertEqual(notes[1].phonemes, ['%'])
        self.assertEqual(notes[2].word, 'zee')

    def test_a_note_with_no_phonemes_is_not_a_rest_here(self):
        # is_rest() is true of an empty note, and an empty note is exactly
        # what lyrics are written onto
        notes = [note(), note(64)]
        dlg = lyrics(notes)
        self.assertEqual(dlg.places, [0, 1])

    def test_backspace_on_an_empty_box_goes_back_a_note(self):
        notes = [note(), note(64)]
        dlg = lyrics(notes)
        type_in(dlg, 'day ')
        backspace(dlg)
        self.assertEqual(dlg.at, 0)
        self.assertEqual(dlg.box.GetValue(), 'day')
        self.assertEqual(notes[0].word, '')          # put back as it was
        self.assertEqual(notes[0].phonemes, [])

    def test_backspace_then_deletes_letters(self):
        notes = [note(), note(64)]
        dlg = lyrics(notes)
        type_in(dlg, 'day ')
        backspace(dlg)
        backspace(dlg)
        self.assertEqual(dlg.box.GetValue(), 'da')
        self.assertEqual(dlg.at, 0)

    def test_backspace_walks_back_note_after_note(self):
        notes = [note(), note(64), note(65)]
        dlg = lyrics(notes)
        type_in(dlg, 'day zee ')
        self.assertEqual(dlg.at, 2)
        backspace(dlg)                      # back to "zee"
        for _ in range(3):
            backspace(dlg)                  # empty it
        self.assertEqual(dlg.box.GetValue(), '')
        backspace(dlg)                      # and back again
        self.assertEqual(dlg.at, 0)
        self.assertEqual(dlg.box.GetValue(), 'day')

    def test_backspace_at_the_first_note_does_nothing(self):
        notes = [note()]
        dlg = lyrics(notes)
        backspace(dlg)
        self.assertEqual(dlg.at, 0)
        self.assertEqual(dlg.box.GetValue(), '')
        self.assertIn('this is the first note', dlg.studio.said)

    def test_a_word_typed_after_walking_back_replaces_the_old_one(self):
        notes = [note(), note(64)]
        dlg = lyrics(notes)
        type_in(dlg, 'day ')
        backspace(dlg)
        for _ in range(3):
            backspace(dlg)
        type_in(dlg, 'zee ')
        self.assertEqual(notes[0].word, 'zee')
        self.assertEqual(notes[0].phonemes, ['z', 'IY'])

    def test_space_on_an_empty_box_writes_nothing(self):
        notes = [note()]
        dlg = lyrics(notes)
        type_in(dlg, ' ')
        self.assertEqual(dlg.at, 0)
        self.assertEqual(notes[0].word, '')

    def test_typing_past_the_last_note_writes_nothing(self):
        notes = [note()]
        dlg = lyrics(notes)
        type_in(dlg, 'day ')
        type_in(dlg, 'zee ')
        self.assertEqual(notes[0].word, 'day')
        self.assertIn('no more notes to write on', dlg.studio.said)

    def test_a_word_with_no_pronunciation_is_named_and_still_written(self):
        notes = [note()]
        dlg = lyrics(notes)
        type_in(dlg, 'xyzzy ')
        self.assertEqual(notes[0].word, 'xyzzy')
        self.assertEqual(notes[0].phonemes, [])
        self.assertEqual(dlg.missing, ['xyzzy'])

    def test_restore_puts_every_note_back(self):
        notes = [note(62, ['k', 'IY', 'p']), note(64)]
        notes[0].word = 'keep'
        dlg = lyrics(notes)
        type_in(dlg, 'day zee ')
        dlg.restore()
        self.assertEqual(notes[0].word, 'keep')
        self.assertEqual(notes[0].phonemes, ['k', 'IY', 'p'])
        self.assertEqual(notes[1].word, '')
        self.assertEqual(notes[1].phonemes, [])

    def test_count_is_the_notes_written_on(self):
        notes = [note(), note(64), note(65)]
        dlg = lyrics(notes)
        type_in(dlg, 'dai-sy ')
        self.assertEqual(dlg.count(), 2)

    def test_starting_part_way_along(self):
        notes = [note(), note(64), note(65)]
        dlg = lyrics(notes, start=2)
        type_in(dlg, 'day ')
        self.assertEqual(notes[2].word, 'day')
        self.assertEqual([n.word for n in notes[:2]], ['', ''])


if __name__ == '__main__':
    unittest.main()
