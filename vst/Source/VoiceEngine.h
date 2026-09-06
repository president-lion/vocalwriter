/*  VoiceEngine.h -- VocalWriter's synthesiser, as one voice of a plugin.

    One VoiceEngine is one vw_editor, which is one voice with its own context:
    the granularity the C API already has, and the reason "one voice per
    plugin" costs nothing to arrange.

    Rendering is done ahead of time, not in the audio callback, and that is
    not a shortcut. The engine wants a whole phrase's phonemes packed into one
    block before it will start (`vw_ed_sequence`, then `vw_ed_start`), it is
    told each note's length in beats at the moment the note begins, and it
    writes into a buffer of its own that it grows as it goes. None of those
    things can happen on the audio thread, and none of them can be fed by MIDI
    arriving a block at a time -- a note's length is not known until its
    note-off, which is after the note has to start sounding. So the part is
    rendered in full on a background thread whenever it changes, and the audio
    callback only reads out of the finished buffer at the host's playhead.
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Part.h"

struct vw_editor;

/*  The engine's own sample rate. Not a choice: a control frame is 220 stereo
    frames (FRAME_HALFWORDS 440 in editor.c) and the reverb runs in blocks of
    220, both of which are 5 ms only at this rate. Host rates other than this
    are resampled on the way out.
*/
constexpr double kEngineSampleRate = 44100.0;

/*  How the engine's output buffer is read here, and how that was checked.

    The header describes it as groups of four halfwords, "L0 L1 R0 R1", and
    ppc/render.py unpacks it a third way again. Rather than argue from the
    comments, the reading below was measured: a five-note scale rendered
    through it comes out with every note's fundamental within six cents of the
    pitch it was given, and 2.494 seconds of audio where 2.500 were asked for.
    Test/rendertest.cpp is that check, and it runs on every build -- a wrong
    reading here still makes a sound, so nothing else would catch it.

    The two halfwords of a frame are equal over the whole buffer: the voice is
    mono, written to both channels.
*/
struct VoiceControls
{
    int colour     = 95;      // Speech_Color
    int vibrato    = 31;      // Speech_VibDepth
    int vibratoRate= 47;      // Speech_VibFreq
    int breath     = 0;       // Speech_Breath
    int portamento = 0;       // Speech_Portamento
    int detune     = 0;       // Speech_Detune

    bool operator== (const VoiceControls& o) const noexcept
    {
        return colour == o.colour && vibrato == o.vibrato
            && vibratoRate == o.vibratoRate && breath == o.breath
            && portamento == o.portamento && detune == o.detune;
    }
    bool operator!= (const VoiceControls& o) const noexcept { return ! (*this == o); }
};

class VoiceEngine
{
public:
    VoiceEngine();
    ~VoiceEngine();

    /*  Reads VocalWriter's four data files out of `assetDir`. They are KAE
        Labs' 2005 software and are not ours to ship, so the plugin is pointed
        at the copy the user already has. Returns true, or false with the
        reason in lastError(). */
    bool open (const juce::File& assetDir);
    bool isOpen() const noexcept    { return editor != nullptr; }
    juce::String lastError() const  { return error; }

    /*  Where the assets are, if a usable set can be found without asking:
        beside the plugin, then the places a VocalWriter checkout keeps them. */
    static juce::File guessAssetDir();
    static bool looksLikeAssetDir (const juce::File& dir);

    /*  The voices worth offering, which is not every slot the bank has.
        `vw_ed_voice_count` says 88 and the last of them has no name and
        wedges the engine when it is selected, so the ones with names are the
        ones counted here -- which is the same filter ppc/engine.py's
        `voices()` applies, for the same reason. An index anywhere in this
        class is a place in that list, not a place in the bank. */
    int  voiceCount() const                   { return (int) usable.size(); }
    juce::String voiceName (int index) const;
    bool voiceNeedsBank (int index) const;

    /*  The dictionary's pronunciation of one word, as phoneme names, with the
        syllables kept apart. Empty if the word is not known. */
    std::vector<std::vector<std::string>> wordSyllables (const juce::String& word) const;

    /*  Every phoneme the engine has, in its own order -- the names that may
        appear in a note. */
    const std::vector<std::string>& phonemeNames() const { return phonemes; }
    bool isPhoneme (const std::string& name) const;

    /*  Render the whole part at `bpm`, as stereo at kEngineSampleRate.
        Rests are sung as the silence phoneme, so what comes back is one
        continuous stretch beginning at the part's first beat. */
    juce::AudioBuffer<float> render (const Part& part, int voiceIndex,
                                     double bpm, const VoiceControls& controls);

    /*  Set when a render stopped because it ran out of room rather than
        because the singing finished. */
    bool stoppedShort() const noexcept { return ranOut; }

    /*  How far a voice has to be turned down inside the engine to stop it
        clamping. See the definition: most of the bank needs this badly. */
    float headroom (int voiceIndex);

private:
    /*  The block SetSeqAddr reads: a count, then the phoneme codes, the
        control words, a spare word each and the nominal durations, all
        big-endian. Bit 0 of a control word marks the phoneme that begins a
        note, and the engine consumes one note per marker. */
    std::vector<unsigned char> packSequence (const Part& part) const;

    /*  One render, at a given internal level. `render` is this with the
        voice's headroom worked out first; the probe is this at 0.01.

        Each one runs on an engine of its own. A render leaves the resonator
        delay lines holding the state of whatever was last sung through them,
        so a second render down the same engine begins in the middle of the
        first -- the probe made the note after it come out an octave and a half
        flat. ppc/render.py sidesteps this by opening a fresh engine for every
        render, and at 25 ms to open one there is no reason not to do the same.
    */
    juce::AudioBuffer<float> renderAt (const Part& part, int voiceIndex, double bpm,
                                       const VoiceControls& controls, float level);

    /*  An engine set up the way a render wants one, or nullptr. The caller
        closes it. */
    vw_editor* openEditor() const;

    /*  Which slot of the bank each offered voice really is. */
    int inBank (int index) const;

    vw_editor* editor = nullptr;
    std::vector<int> usable;
    std::map<int, float> headrooms;
    juce::MemoryBlock rsrc, gmspeech, gmbank, lexicon;
    std::vector<std::string> phonemes;      // code -> name
    juce::String error;
    bool ranOut = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceEngine)
};
