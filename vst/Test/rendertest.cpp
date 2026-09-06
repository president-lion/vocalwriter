/*  rendertest -- does the plugin's own wrapper sing?

    The engine underneath is proved exact elsewhere; what is unproved here is
    everything this plugin puts around it -- packing the phonemes into the
    block SetSeqAddr reads, feeding a note at each point the engine asks for
    one, and reading the output buffer the right way round. A wrong phoneme
    code or a mispacked duration still renders; it just sings the wrong thing,
    which no compiler catches.

    So: sing a scale on a known vowel, check the pitch that comes out is the
    pitch that went in, and write a WAV to listen to.

      rendertest <assets dir> [out.wav]
*/
#include <juce_audio_formats/juce_audio_formats.h>

#include "Part.h"
#include "VoiceEngine.h"

namespace
{
/*  The fundamental of a stretch of audio, by autocorrelation. Good to a few
    cents over a held vowel, which is all that is wanted: the question is
    whether a note comes out on its own pitch or an octave away from it. */
double fundamental (const juce::AudioBuffer<float>& buf, int from, int len)
{
    const float* x = buf.getReadPointer (0);
    len = juce::jmin (len, buf.getNumSamples() - from);
    if (len < 2048)
        return 0.0;

    double mean = 0.0;
    for (int i = 0; i < len; ++i)
        mean += x[from + i];
    mean /= len;

    const int lo = (int) (kEngineSampleRate / 1000.0);   // 1000 Hz
    const int hi = (int) (kEngineSampleRate / 60.0);     // 60 Hz
    double best = 0.0;
    int bestLag = 0;
    for (int lag = lo; lag < hi && lag < len / 2; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i < len - lag; ++i)
            sum += (x[from + i] - mean) * (x[from + i + lag] - mean);
        if (sum > best)
        {
            best = sum;
            bestLag = lag;
        }
    }
    return bestLag > 0 ? kEngineSampleRate / bestLag : 0.0;
}

int fail (const juce::String& why)
{
    std::fprintf (stderr, "FAIL: %s\n", why.toRawUTF8());
    return 1;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    juce::File assets = argc > 1 ? juce::File (juce::String (argv[1]))
                                 : VoiceEngine::guessAssetDir();
    if (assets == juce::File() || ! VoiceEngine::looksLikeAssetDir (assets))
        return fail ("give me the folder with VocalWriter.rsrc and GMSpeech.rsrc in it");

    VoiceEngine engine;
    if (! engine.open (assets))
        return fail (engine.lastError());
    std::printf ("engine open: %d voices\n", engine.voiceCount());

    // -- the dictionary ----------------------------------------------------

    auto syllables = engine.wordSyllables ("twinkle");
    std::printf ("\"twinkle\" ->");
    for (auto& syl : syllables)
    {
        std::printf (" [");
        for (auto& p : syl)
            std::printf ("%s", p.c_str());
        std::printf ("]");
    }
    std::printf ("\n");
    if (syllables.empty())
        return fail ("the dictionary gave nothing back for \"twinkle\"");

    // -- sharing a word out over notes -------------------------------------

    {
        struct Case { const char* word; int notes; const char* want; };
        /*  What regroup should do: syllables kept whole, shared out as evenly
            as they go, and never more groups than the word has syllables.
            These are ppc/phonology.py's own answers, checked against it --
            three syllables over two notes gives one then two, not two then
            one, and a line typed in the Studio's lyric writer therefore sings
            the same here. */
        const Case cases[] = {
            { "bicycle", 3, "bAY|sIH|kEL" },
            { "bicycle", 2, "bAY|sIHkEL" },
            { "bicycle", 1, "bAYsIHkEL" },
            { "thousand", 2, "THAW|zENd" },   /* the syllabic n of "thousand" */
            { "button",   2, "bUX|tEN" },
            { "star",     3, "stAR" },        /* one syllable stays one note */
        };
        int bad = 0;
        for (auto& c : cases)
        {
            auto groups = regroup (engine.wordSyllables (c.word), c.notes);
            juce::String got;
            for (size_t g = 0; g < groups.size(); ++g)
            {
                if (g > 0) got << "|";
                for (auto& ph : groups[g]) got << ph;
            }
            const bool ok = got == juce::String (c.want);
            std::printf ("  %-9s over %d: %-14s %s\n", c.word, c.notes,
                         got.toRawUTF8(), ok ? "" : ("<-- wanted " + juce::String (c.want)).toRawUTF8());
            if (! ok) ++bad;
        }
        if (bad > 0)
            return fail (juce::String (bad) + " words were not divided as expected");
    }

    // -- a scale, sung on AA ----------------------------------------------

    const int scale[] = { 57, 60, 64, 69, 72 };   // A3 C4 E4 A4 C5
    Part part;
    for (int i = 0; i < 5; ++i)
    {
        Note n;
        n.startBeats = i * 1.0;
        n.beats = 1.0;
        n.midi = scale[i];
        n.velocity = 100;
        n.phonemes = { "AA" };
        part.notes.push_back (n);
    }

    const double bpm = 120.0;
    auto audio = engine.render (part, 0, bpm, {});
    if (audio.getNumSamples() == 0)
        return fail ("nothing was rendered: " + engine.lastError());

    const double seconds = audio.getNumSamples() / kEngineSampleRate;
    std::printf ("rendered %.3f s (%d samples), expected about %.3f s\n",
                 seconds, audio.getNumSamples(), 5 * 60.0 / bpm);

    /*  Each note is one beat, so look at the settled middle of each. */
    const int perNote = (int) (kEngineSampleRate * 60.0 / bpm);
    int wrong = 0;
    for (int i = 0; i < 5; ++i)
    {
        const double want = 440.0 * std::pow (2.0, (scale[i] - 69) / 12.0);
        const double got = fundamental (audio, i * perNote + perNote / 3, perNote / 3);
        const double cents = got > 0.0 ? 1200.0 * std::log2 (got / want) : 9999.0;
        std::printf ("  note %d: MIDI %d wants %7.2f Hz, got %7.2f Hz  (%+.0f cents)%s\n",
                     i + 1, scale[i], want, got, cents,
                     std::abs (cents) < 60.0 ? "" : "   <-- wrong");
        if (! (std::abs (cents) < 60.0))
            ++wrong;
    }

    juce::File out (argc > 2 ? juce::String (argv[2]) : juce::String ("rendertest.wav"));
    out.deleteFile();
    juce::WavAudioFormat wav;
    if (auto* stream = out.createOutputStream().release())
    {
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (stream, kEngineSampleRate, 2, 16, {}, 0));
        if (writer != nullptr)
        {
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
            std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());
        }
    }

    if (wrong > 0)
        return fail (juce::String (wrong) + " of 5 notes came out on the wrong pitch");
    std::printf ("OK: every note on its own pitch\n");
    return 0;
}
