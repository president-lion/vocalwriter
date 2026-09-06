#include "VoiceEngine.h"

extern "C" {
#include "vw_editor.h"
}

namespace
{
/*  The engine's nominal length for each phoneme, in the order the phoneme
    table has them, read out of VocalWriter.rsrc's `ttvi` resource (the
    DURATION_1 table -- see tools/ttvi.py in the VocalWriter repository).
    Syllable_Duration only uses their ratios, so the unit does not matter as
    long as it is consistent.

    They are here rather than read from the resource because the engine gives
    no way to ask for them and they are constants of a data file from 2005.
    The order is the engine's own, which vw_ed_phoneme_name confirms at
    startup; a mismatch is caught there rather than sung. */
constexpr int kNominal[] = {
    170, 160, 160, 230, 240, 160, 240, 170, 120, 180,   /* IY IH EH AE AA UX AO UH AX ER */
    200, 250, 260, 260, 220, 210, 230, 230, 250, 250,   /* EY AY OY AW OW UW YU IR XR AR */
    250, 230, 120, 305, 120, 100, 160, 170,  60,  75,   /* OR UR IX %  RX LX EL EN w  y  */
     65,  75,  70,  70,  65,  80, 100,  70, 100,  60,   /* r  l  h  m  n  NG f  v  TH DH */
    115,  75, 115,  70,  85,  80,  85,  80,  90,  90,   /* s  z  SH ZH p  b  t  d  k  g  */
    160, 100,  70,  20,  50,  60, 160                   /* CH JH TX Q  QX DD O           */
};
constexpr int kPhonemeCount = (int) (sizeof (kNominal) / sizeof (kNominal[0]));

/*  The silence phoneme, which is what a rest sings. */
const std::string kRest = "%";

void putBE16 (std::vector<unsigned char>& out, int v)
{
    out.push_back ((unsigned char) ((v >> 8) & 0xFF));
    out.push_back ((unsigned char) (v & 0xFF));
}

/*  VocalWriter's data files, by the names and places the application keeps
    them in. The engine's tables live inside the Mac application bundle. */
juce::File rsrcIn (const juce::File& dir)
{
    auto inBundle = dir.getChildFile ("VocalWriter.app/Contents/Resources/VocalWriter.rsrc");
    return inBundle.existsAsFile() ? inBundle : dir.getChildFile ("VocalWriter.rsrc");
}
} // namespace

VoiceEngine::VoiceEngine() = default;

VoiceEngine::~VoiceEngine()
{
    if (editor != nullptr)
        vw_ed_close (editor);
}

bool VoiceEngine::looksLikeAssetDir (const juce::File& dir)
{
    return dir.isDirectory()
        && rsrcIn (dir).existsAsFile()
        && dir.getChildFile ("GMSpeech.rsrc").existsAsFile();
}

juce::File VoiceEngine::guessAssetDir()
{
    juce::Array<juce::File> tries;
    auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    for (auto d = exe.getParentDirectory(); d.exists(); d = d.getParentDirectory())
    {
        tries.add (d.getChildFile ("assets"));
        if (d.getParentDirectory() == d)
            break;
    }
    tries.add (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("GitHub/vocalwriter/assets"));
    for (auto& d : tries)
        if (looksLikeAssetDir (d))
            return d;
    return {};
}

bool VoiceEngine::open (const juce::File& assetDir)
{
    if (editor != nullptr)
    {
        vw_ed_close (editor);
        editor = nullptr;
    }
    error = {};

    auto rsrcFile = rsrcIn (assetDir);
    auto speechFile = assetDir.getChildFile ("GMSpeech.rsrc");
    if (! rsrcFile.existsAsFile() || ! speechFile.existsAsFile())
    {
        error = "VocalWriter.rsrc and GMSpeech.rsrc were not in " + assetDir.getFullPathName();
        return false;
    }
    if (! rsrcFile.loadFileAsData (rsrc) || ! speechFile.loadFileAsData (gmspeech))
    {
        error = "could not read VocalWriter's data files";
        return false;
    }

    editor = vw_ed_open ((const unsigned char*) rsrc.getData(), rsrc.getSize(),
                         (const unsigned char*) gmspeech.getData(), gmspeech.getSize());
    if (editor == nullptr)
    {
        error = "VocalWriter's data files are there but the engine refused them";
        return false;
    }

    /*  The instrument bank is what the voices with instrument names are built
        on; without it only the rest of the bank can be selected. The
        dictionary is what turns a typed word into phonemes. Both are
        optional, and both are usually there. */
    auto bankFile = assetDir.getChildFile ("GMBank.rsrc");
    if (bankFile.existsAsFile() && bankFile.loadFileAsData (gmbank))
        vw_ed_bank (editor, (const unsigned char*) gmbank.getData(), gmbank.getSize());

    auto lexFile = assetDir.getChildFile ("EnglishLex");
    if (lexFile.existsAsFile() && lexFile.loadFileAsData (lexicon))
        vw_ed_lexicon (editor, (const unsigned char*) lexicon.getData(), lexicon.getSize());

    /*  A slot with no name is not a voice. There is one at the end of the
        bank, and selecting it does not come back -- so it is left out here
        rather than guarded against everywhere it could be reached. */
    usable.clear();
    for (int i = 0, n = vw_ed_voice_count (editor); i < n; ++i)
    {
        const char* name = vw_ed_voice_name_at (editor, i);
        if (name != nullptr && juce::String (name).trim().isNotEmpty())
            usable.push_back (i);
    }
    if (usable.empty())
    {
        error = "the voice bank in GMSpeech.rsrc has no voices in it";
        vw_ed_close (editor);
        editor = nullptr;
        return false;
    }

    phonemes.clear();
    for (int code = 0; code < kPhonemeCount; ++code)
    {
        const char* name = vw_ed_phoneme_name (code);
        phonemes.push_back (name != nullptr ? std::string (name) : std::string());
    }
    /*  The nominal durations above are indexed by the same code, so if the
        table the engine has is not the one they were taken from, say so now
        rather than singing every note the wrong length. */
    if (phonemes.empty() || phonemes[(size_t) 23] != kRest)
    {
        error = "the phoneme table in VocalWriter.rsrc is not the one this was built against";
        vw_ed_close (editor);
        editor = nullptr;
        return false;
    }
    return true;
}

int VoiceEngine::inBank (int index) const
{
    if (usable.empty())
        return 0;
    return usable[(size_t) juce::jlimit (0, (int) usable.size() - 1, index)];
}

juce::String VoiceEngine::voiceName (int index) const
{
    if (editor == nullptr || usable.empty())
        return {};
    const char* n = vw_ed_voice_name_at (editor, inBank (index));
    return n != nullptr ? juce::String (n) : juce::String();
}

bool VoiceEngine::voiceNeedsBank (int index) const
{
    return editor != nullptr && vw_ed_voice_needs_bank (editor, inBank (index)) == 1;
}

bool VoiceEngine::isPhoneme (const std::string& name) const
{
    for (auto& p : phonemes)
        if (p == name)
            return true;
    return false;
}

std::vector<std::vector<std::string>> VoiceEngine::wordSyllables (const juce::String& word) const
{
    std::vector<std::vector<std::string>> out;
    if (editor == nullptr || word.isEmpty())
        return out;

    unsigned char syllables[10][9] = {};
    int n = vw_ed_word (editor, word.toRawUTF8(), &syllables[0][0]);
    for (int s = 0; s < n && s < 10; ++s)
    {
        std::vector<std::string> syl;
        int len = syllables[s][0];              // a Pascal string of codes
        for (int k = 1; k <= len && k < 9; ++k)
        {
            int code = syllables[s][k];
            if (code >= 0 && code < (int) phonemes.size())
                syl.push_back (phonemes[(size_t) code]);
        }
        if (! syl.empty())
            out.push_back (std::move (syl));
    }
    return out;
}

std::vector<unsigned char> VoiceEngine::packSequence (const Part& part) const
{
    std::vector<int> phon, ctrl, dur;

    auto codeOf = [this] (const std::string& name) -> int
    {
        for (size_t i = 0; i < phonemes.size(); ++i)
            if (phonemes[i] == name)
                return (int) i;
        return -1;
    };

    for (auto& note : part.notes)
    {
        std::vector<int> codes;
        for (auto& name : note.phonemes)
            if (int c = codeOf (name); c >= 0)
                codes.push_back (c);
        if (codes.empty())
            codes.push_back (codeOf (kRest));   // a rest, or a note with nothing on it yet

        for (size_t k = 0; k < codes.size(); ++k)
        {
            phon.push_back (codes[k]);
            ctrl.push_back (k == 0 ? 1 : 0);    // bit 0: this phoneme begins a note
            dur.push_back (juce::jmax (1, kNominal[codes[k]]));
        }
    }

    /*  Syllable_Duration scales the last syllable against the note-start that
        follows it, so every sequence ends with a marked rest for it to find. */
    phon.push_back (codeOf (kRest));
    ctrl.push_back (1);
    dur.push_back (kNominal[23]);

    const int n = (int) phon.size();
    std::vector<unsigned char> out;
    out.reserve ((size_t) (4 + n * 8));
    putBE16 (out, 0);
    putBE16 (out, n);
    for (int v : phon) putBE16 (out, v);
    for (int v : ctrl) putBE16 (out, v);
    for (int i = 0; i < n; ++i) putBE16 (out, 0);   // a spare word each
    for (int v : dur)  putBE16 (out, v);
    return out;
}

float VoiceEngine::headroom (int voiceIndex)
{
    /*  The bank's voices are nothing like each other in level. The ones with
        people's names peak below full scale with the engine wide open --
        Robert at about 0.73 -- but every voice built on the wavetables comes
        out forty or fifty times over it, and the engine clamps each sample
        that goes past. Measured over the whole bank, that is 65 to 70 per
        cent of samples clipped: distortion baked into the audio before any
        volume of ours can reach it, which is what "massive distortion, then
        the voice gets quieter" is -- a wall of clipping on the loud part of
        the note, and a clean quiet tail once it decays under the ceiling.

        So each voice is sung once, quietly, to find out how loud it really
        is, and told to stay inside afterwards. This is what ppc/engine.py's
        `_headroom` does, with its constants: a d-AA on D4, probed at 1/100th,
        and 0.7 of full scale to come back to.

        A voice that already fits is left exactly alone. The test is whether
        it clips, not whether it is loud, so the voices that were right before
        render identically. */
    if (auto it = headrooms.find (voiceIndex); it != headrooms.end())
        return it->second;

    Part probe;
    Note n;
    n.startBeats = 0.0;
    n.beats = 0.6;
    n.midi = 62;
    n.velocity = 100;
    n.phonemes = { "d", "AA" };
    probe.notes.push_back (n);

    constexpr float kProbeLevel = 0.01f;
    constexpr float kHeadroom = 0.7f;

    auto y = renderAt (probe, voiceIndex, 120.0, {}, kProbeLevel);
    float peak = 0.0f;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        peak = juce::jmax (peak, y.getMagnitude (ch, 0, y.getNumSamples()));
    peak /= kProbeLevel;

    const float gain = peak <= 1.0f ? 1.0f : kHeadroom / juce::jmax (peak, 1.0e-6f);
    headrooms[voiceIndex] = gain;
    return gain;
}

juce::AudioBuffer<float> VoiceEngine::render (const Part& part, int voiceIndex,
                                              double bpm, const VoiceControls& controls)
{
    return renderAt (part, voiceIndex, bpm, controls, headroom (voiceIndex));
}

vw_editor* VoiceEngine::openEditor() const
{
    auto* e = vw_ed_open ((const unsigned char*) rsrc.getData(), rsrc.getSize(),
                          (const unsigned char*) gmspeech.getData(), gmspeech.getSize());
    if (e != nullptr && gmbank.getSize() > 0)
        vw_ed_bank (e, (const unsigned char*) gmbank.getData(), gmbank.getSize());
    return e;
}

juce::AudioBuffer<float> VoiceEngine::renderAt (const Part& part, int voiceIndex,
                                                double bpm, const VoiceControls& controls,
                                                float level)
{
    ranOut = false;
    juce::AudioBuffer<float> empty (2, 0);
    if (editor == nullptr || part.isEmpty())
        return empty;

    vw_editor* eng = openEditor();
    if (eng == nullptr)
    {
        error = "the engine would not open for this render";
        return empty;
    }
    const juce::ScopeGuard closeIt { [&] { vw_ed_close (eng); } };

    /*  MAX_FRAMES as ppc/render.py has it: a phrase longer than this is
        sixteen minutes of singing without a break in it. */
    constexpr int kMaxFrames = 200000;

    vw_ed_tempo_scale (eng, 1.0f / 240.0f);
    vw_ed_tempo (eng, juce::jlimit (10, 250, (int) std::lround (bpm)));

    if (vw_ed_voice (eng, inBank (voiceIndex)) != 0)
        vw_ed_voice (eng, inBank (0));

    auto blob = packSequence (part);
    if (vw_ed_sequence (eng, blob.data(), blob.size()) != 0)
    {
        error = "the engine would not take the phonemes for this part";
        return empty;
    }
    vw_ed_start (eng);

    /*  The glide table is wired after the defaults on purpose: the engine's
        own default portamento is read out of it, so wiring it earlier puts a
        glide on every note. ppc/render.py explains it at length. */
    vw_ed_defaults (eng, 1);
    vw_ed_volume (eng, 127);
    /*  After the volume, as ppc/render.py has it, and only when it is not the
        engine's own 1.0 -- a voice that already fits is not touched. */
    if (std::abs (level - 1.0f) > 1.0e-6f)
        vw_ed_level (eng, level);

    /*  Only the controls that have been moved off their default, so a part
        that sets none of them sounds exactly as the engine left it -- the
        values do not round-trip through a 0-127 knob perfectly. */
    const VoiceControls d {};
    if (controls.colour     != d.colour)     vw_ed_control (eng, "Speech_Color", controls.colour);
    if (controls.vibrato    != d.vibrato)    vw_ed_control (eng, "Speech_VibDepth", controls.vibrato);
    if (controls.vibratoRate!= d.vibratoRate)vw_ed_control (eng, "Speech_VibFreq", controls.vibratoRate);
    if (controls.breath     != d.breath)     vw_ed_control (eng, "Speech_Breath", controls.breath);
    if (controls.portamento != d.portamento) vw_ed_control (eng, "Speech_Portamento", controls.portamento);
    if (controls.detune     != d.detune)     vw_ed_control (eng, "Speech_Detune", controls.detune);

    size_t next = 0;
    auto feed = [&]
    {
        auto& n = part.notes[next++];
        vw_ed_note (eng, n.midi, 0, juce::jlimit (1, 127, n.velocity), n.beats);
    };
    feed();

    int frames = 0;
    while (frames < kMaxFrames)
    {
        int ran = vw_ed_frames (eng, kMaxFrames - frames);
        frames += ran;
        if (ran == 0)
        {
            /*  It ran nothing and is not asking for anything: there is no room
                left to write into, and asking again would ask for ever. */
            ranOut = true;
            break;
        }
        if (vw_ed_state (eng) == 3)
            break;
        if (vw_ed_wants_note (eng))
        {
            if (next >= part.notes.size())
                break;
            feed();
        }
    }
    if (frames >= kMaxFrames)
        ranOut = true;

    const int32_t halfwords = vw_ed_wave_index (eng);
    const int16_t* raw = vw_ed_wave (eng);
    if (raw == nullptr || halfwords <= 0)
        return empty;

    /*  Frame k is (raw[2k], raw[2k+1]). See the header: this arrangement is
        the one the scale in Test/rendertest.cpp confirms. */
    const int n = (int) (halfwords / 2);
    juce::AudioBuffer<float> out (2, n);
    auto* left = out.getWritePointer (0);
    auto* right = out.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        left[i]  = (float) raw[2 * i]     / 32768.0f;
        right[i] = (float) raw[2 * i + 1] / 32768.0f;
    }
    return out;
}
