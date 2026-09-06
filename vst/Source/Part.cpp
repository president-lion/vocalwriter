#include "Part.h"

#include <algorithm>

std::vector<std::vector<std::string>>
regroup (const std::vector<std::vector<std::string>>& syllables, int count)
{
    const int parts = (int) syllables.size();
    count = juce::jlimit (1, juce::jmax (1, parts), count);
    if (parts == 0 || count == parts)
        return syllables;

    std::vector<std::vector<std::string>> out;
    int at = 0;
    for (int k = 0; k < count; ++k)
    {
        const int take = (parts - at) / (count - k);
        std::vector<std::string> group;
        for (int p = at; p < at + take; ++p)
            group.insert (group.end(), syllables[(size_t) p].begin(),
                          syllables[(size_t) p].end());
        out.push_back (std::move (group));
        at += take;
    }
    return out;
}

void Part::makeMonophonic()
{
    std::stable_sort (notes.begin(), notes.end(),
                      [] (const Note& a, const Note& b)
                      {
                          if (a.startBeats != b.startBeats)
                              return a.startBeats < b.startBeats;
                          return a.midi > b.midi;   // the top note first
                      });

    std::vector<Note> kept;
    for (auto& n : notes)
    {
        if (! kept.empty() && std::abs (n.startBeats - kept.back().startBeats) < 1.0e-6)
            continue;                    // a lower note of the same chord
        kept.push_back (n);
    }
    for (size_t i = 0; i + 1 < kept.size(); ++i)
        kept[i].beats = juce::jmin (kept[i].beats,
                                    kept[i + 1].startBeats - kept[i].startBeats);
    kept.erase (std::remove_if (kept.begin(), kept.end(),
                                [] (const Note& n) { return n.beats <= 1.0e-6; }),
                kept.end());
    notes = std::move (kept);
}

void Part::setLyrics (const juce::String& text)
{
    /*  One syllable per note, in order. A syllable ending in a hyphen is half
        of a word -- "twin-" and "kle" -- and stays that way here; joining
        them back together to look the word up is the caller's business, since
        that needs the dictionary. */
    juce::StringArray syllables;
    for (auto& word : juce::StringArray::fromTokens (text, " \n\t\r", "\""))
    {
        auto w = word.trim();
        if (w.isEmpty())
            continue;
        int from = 0;
        for (;;)
        {
            int hyphen = w.indexOfChar (from, '-');
            if (hyphen < 0)
            {
                syllables.add (w.substring (from));
                break;
            }
            syllables.add (w.substring (from, hyphen + 1));   // keeps the hyphen
            from = hyphen + 1;
        }
    }

    int s = 0;
    for (auto& note : notes)
    {
        if (s >= syllables.size())
            break;
        auto next = syllables[s++];
        if (note.word != next)
        {
            note.word = next;
            note.phonemes.clear();       // it means something else now
        }
    }
}

std::vector<juce::String> Part::wordsToLookUp() const
{
    std::vector<juce::String> out;
    juce::String building;
    for (auto& note : notes)
    {
        if (note.word.isEmpty())
            continue;
        building += note.word.endsWithChar ('-')
                      ? note.word.dropLastCharacters (1) : note.word;
        if (note.word.endsWithChar ('-'))
            continue;                    // the rest of it is on the next note
        if (note.phonemes.empty())
            out.push_back (building);
        building.clear();
    }
    if (building.isNotEmpty())
        out.push_back (building);
    return out;
}

juce::ValueTree Part::toValueTree() const
{
    juce::ValueTree tree ("part");
    for (auto& n : notes)
    {
        juce::ValueTree v ("note");
        v.setProperty ("start", n.startBeats, nullptr);
        v.setProperty ("beats", n.beats, nullptr);
        v.setProperty ("midi", n.midi, nullptr);
        v.setProperty ("velocity", n.velocity, nullptr);
        v.setProperty ("word", n.word, nullptr);
        juce::StringArray ph;
        for (auto& p : n.phonemes)
            ph.add (juce::String (p));
        v.setProperty ("phonemes", ph.joinIntoString (" "), nullptr);
        tree.appendChild (v, nullptr);
    }
    return tree;
}

Part Part::fromValueTree (const juce::ValueTree& tree)
{
    Part part;
    if (! tree.hasType ("part"))
        return part;
    for (auto v : tree)
    {
        Note n;
        n.startBeats = v.getProperty ("start", 0.0);
        n.beats      = v.getProperty ("beats", 1.0);
        n.midi       = v.getProperty ("midi", 60);
        n.velocity   = v.getProperty ("velocity", 100);
        n.word       = v.getProperty ("word", juce::String());
        auto ph = juce::StringArray::fromTokens (
            v.getProperty ("phonemes", juce::String()).toString(), " ", "");
        for (auto& p : ph)
            if (p.isNotEmpty())
                n.phonemes.push_back (p.toStdString());
        part.notes.push_back (std::move (n));
    }
    return part;
}
