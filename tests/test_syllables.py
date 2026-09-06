"""Where one syllable ends and the next begins, against the real tables.

This reads VocalWriter's own phoneme table rather than a stub, because what is
being checked is the rule that reads it. No engine and no window is needed.
"""
import unittest

from ppc import phonology


class NucleusTests(unittest.TestCase):
    """A syllable needs a nucleus, and two of them have no vowel in them."""

    def test_vowels_are_nuclei(self):
        for sym in ('AA', 'IY', 'OW', 'EY', 'AW', 'UX'):
            self.assertTrue(phonology.is_nucleus(sym), sym)

    def test_ordinary_consonants_are_not(self):
        for sym in ('n', 'm', 'l', 'r', 'w', 'y', 'k', 'b', 'LX'):
            self.assertFalse(phonology.is_nucleus(sym), sym)

    def test_the_syllabic_consonants_are(self):
        # the -le of "little" and the -en of "thousand"
        self.assertTrue(phonology.is_nucleus('EL'))
        self.assertTrue(phonology.is_nucleus('EN'))

    def test_long_consonants_are_still_not_nuclei(self):
        # EN is 170 ms, but so are these, and none of them is a syllable --
        # which is why EN is named rather than let through on its length
        for sym in ('CH', 's', 'SH', '%'):
            self.assertFalse(phonology.is_nucleus(sym), sym)


class SyllableTests(unittest.TestCase):

    def divide(self, phonemes):
        return ['-'.join(part) for part in phonology.syllabify(phonemes)]

    def test_a_syllabic_n_ends_a_syllable(self):
        # "thousand": it counted as one syllable and would not divide over
        # two notes, so a lyric written "thou-sand" refused to lay itself out
        self.assertEqual(self.divide(['TH', 'AW', 'z', 'EN', 'd']),
                         ['TH-AW', 'z-EN-d'])

    def test_button(self):
        self.assertEqual(self.divide(['b', 'UX', 't', 'EN']),
                         ['b-UX', 't-EN'])

    def test_a_syllabic_l_ends_one_too(self):
        self.assertEqual(self.divide(['l', 'IH', 'Q', 'EL']),
                         ['l-IH', 'Q-EL'])

    def test_the_divisions_the_scores_use(self):
        # the words phonology.py says it was checked against
        self.assertEqual(self.divide(['b', 'AY', 's', 'IH', 'k', 'EL']),
                         ['b-AY', 's-IH', 'k-EL'])          # bi-cy-cle
        self.assertEqual(self.divide(['d', 'EY', 'z', 'IY']),
                         ['d-EY', 'z-IY'])                  # dai-sy

    def test_one_syllable_stays_one(self):
        self.assertEqual(self.divide(['d', 'EY']), ['d-EY'])

    def test_regroup_never_promises_more_notes_than_syllables(self):
        # what the lyric writer relies on when it reports a word as too short
        # for the hyphens it was given
        self.assertEqual(len(phonology.regroup(['d', 'EY'], 3)), 1)
        self.assertEqual(len(phonology.regroup(['d', 'EY', 'z', 'IY'], 3)), 2)


if __name__ == '__main__':
    unittest.main()
