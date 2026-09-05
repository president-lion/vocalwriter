"""Run with: python -m unittest discover -s tests."""
import unittest
from types import SimpleNamespace
from unittest.mock import Mock

from app import project
from app import studio


def tracks(*flags):
    """A song of parts, each given as (mute, solo)."""
    return [project.Track(name=str(i + 1), mute=m, solo=s)
            for i, (m, s) in enumerate(flags)]


class TrackState(unittest.TestCase):
    """A row says what has become of the track, and says nothing otherwise."""

    def state(self, parts, i=0):
        frame = SimpleNamespace(tracks=parts)
        return studio.Frame.track_state(frame, parts[i])

    def test_a_track_that_can_be_heard_says_nothing(self):
        self.assertEqual(self.state(tracks((False, False))), '')

    def test_a_muted_track_says_muted(self):
        self.assertEqual(self.state(tracks((True, False)), 0), 'muted')

    def test_a_soloed_track_says_soloed(self):
        self.assertEqual(self.state(tracks((False, True)), 0), 'soloed')

    def test_soloing_one_leaves_the_others_reading_muted(self):
        parts = tracks((False, True), (False, False), (False, False))
        self.assertEqual([self.state(parts, i) for i in range(3)],
                         ['soloed', 'muted', 'muted'])

    def test_mute_wins_over_solo_because_audible_says_so(self):
        # project.audible drops a track that is muted even when it is soloed,
        # and the row has to agree with what will actually be heard.
        self.assertEqual(self.state(tracks((True, True)), 0), 'muted')


class ToggleAnnouncements(unittest.TestCase):
    """S and M say one word: the state the track is now in."""

    def frame(self, parts, at=0):
        return SimpleNamespace(
            tracks=parts, track_at=lambda: at, touch=Mock(),
            refresh_track=Mock(), announce_state=Mock(),
            tracks_list=object())

    def spoken(self, frame, field):
        studio.Frame.toggle_track(frame, field)
        frame.announce_state.assert_called_once()
        return frame.announce_state.call_args[0][0]

    def test_muting_says_muted(self):
        frame = self.frame(tracks((False, False)))
        self.assertEqual(self.spoken(frame, 'mute'), 'muted')

    def test_unmuting_says_unmuted(self):
        frame = self.frame(tracks((True, False)))
        self.assertEqual(self.spoken(frame, 'mute'), 'unmuted')

    def test_soloing_says_soloed(self):
        frame = self.frame(tracks((False, False)))
        self.assertEqual(self.spoken(frame, 'solo'), 'soloed')

    def test_unsoloing_says_unsoloed(self):
        frame = self.frame(tracks((False, True)))
        self.assertEqual(self.spoken(frame, 'solo'), 'unsoloed')

    def test_every_row_is_redrawn_because_solo_changes_them_all(self):
        frame = self.frame(tracks((False, False), (False, False)))
        studio.Frame.toggle_track(frame, 'solo')
        self.assertEqual(frame.refresh_track.call_count, 2)

    def test_the_row_is_offered_as_the_fallback(self):
        frame = self.frame(tracks((False, False)), at=0)
        studio.Frame.toggle_track(frame, 'mute')
        self.assertEqual(frame.announce_state.call_args[0][1:],
                         (frame.tracks_list, 0))


class TrackNames(unittest.TestCase):
    """A track is called a number, and never the same number twice."""

    def name(self, parts):
        return studio.Frame.track_name(SimpleNamespace(tracks=parts))

    def test_a_new_track_is_the_next_number(self):
        self.assertEqual(self.name(tracks((False, False))), '2')

    def test_a_number_already_taken_is_skipped(self):
        parts = tracks((False, False), (False, False))
        parts[1].name = '3'
        self.assertEqual(self.name(parts), '4')


if __name__ == '__main__':
    unittest.main()
