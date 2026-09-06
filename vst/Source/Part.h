/*  Part.h -- the notes this voice sings.

    The plugin owns its notes rather than singing the MIDI as it arrives, for
    a reason that is the engine's and not a preference: a note's length in
    beats has to be handed over at the moment the note begins, and MIDI does
    not say how long a note is until its note-off, which is after that moment
    has passed. A phrase's phonemes have to be packed and handed over before
    any of it sounds, too. So the notes are collected into a part first and
    sung from there.

    The MIDI on the track fills the part in: while the transport runs, note-ons
    and note-offs are written down against their position on the host's
    timeline, and when they stop changing the part is re-rendered. Play the
    section once and the plugin has it.
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <string>
#include <vector>

struct Note
{
    double startBeats = 0.0;    // from the start of the part
    double beats      = 1.0;
    int    midi       = 60;
    int    velocity   = 100;
    /*  What this note sings. Empty means a rest. A note with a word on it but
        no phonemes yet is one the dictionary has still to be asked about. */
    std::vector<std::string> phonemes;
    juce::String word;

    bool isRest() const noexcept { return phonemes.empty() && word.isEmpty(); }
};

/*  Divide a word's syllables over exactly `count` notes, as evenly as they
    go, keeping every syllable whole -- so a three-syllable word asked for two
    notes gives two syllables then one, rather than breaking one in half.

    This is ppc/phonology.py's `regroup`, which the Studio's lyric writer uses,
    ported so that a line typed in one sings the same in the other. Asking for
    more notes than the word has syllables gives one per syllable and no more:
    the notes it does not reach are left alone rather than being filled with a
    repeat of the last one, which would re-articulate its consonant on every
    one of them.
*/
std::vector<std::vector<std::string>>
regroup (const std::vector<std::vector<std::string>>& syllables, int count);

struct Part
{
    std::vector<Note> notes;    // in time order, no two overlapping

    bool  isEmpty() const noexcept { return notes.empty(); }
    double lengthBeats() const noexcept
    {
        return notes.empty() ? 0.0
             : notes.back().startBeats + notes.back().beats;
    }

    /*  Put the notes in time order and cut short any that overlap the next.
        A voice sings one note at a time; where two sound together the higher
        one is the one that carries the tune, so it is the one kept. */
    void makeMonophonic();

    /*  Lay `text` over the notes: one syllable per note, in order. Words are
        split on spaces, and a syllable ending in a hyphen is joined to the
        next note the way "twin-" and "kle" are. Notes past the end of the
        text keep what they had; a note whose word is unchanged keeps the
        phonemes already worked out for it, so retyping one word does not send
        every other word back to the dictionary. */
    void setLyrics (const juce::String& text);

    /*  The words still needing a pronunciation, in the order they appear. */
    std::vector<juce::String> wordsToLookUp() const;

    juce::ValueTree toValueTree() const;
    static Part fromValueTree (const juce::ValueTree& tree);
};
