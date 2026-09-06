#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    /*  The bank has 87 voices and a program change reaches only the ones its
        own map names, so the voice is chosen by its place in the bank. The
        range is fixed here because a parameter's range cannot change after
        the plugin is constructed, and the bank is not open yet; names come
        from the engine when the editor asks. */
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "voice", 1 }, "Voice", 0, 127, 0));

    /*  The engine's own voice controls, at the engine's own defaults. Chorus
        and reverb are deliberately not here: the song's reverb belongs to the
        track in the host, which has better ones. */
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "colour", 1 }, "Colour", 0, 127, 95));
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "vibrato", 1 }, "Vibrato depth", 0, 127, 31));
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "vibratoRate", 1 }, "Vibrato rate", 0, 127, 47));
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "breath", 1 }, "Breath", 0, 127, 0));
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "portamento", 1 }, "Portamento", 0, 127, 0));
    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { "detune", 1 }, "Voice detune", -8192, 8191, 0));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "level", 1 }, "Level",
        NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 1.0f));
    return layout;
}
} // namespace

VocalWriterVoiceProcessor::VocalWriterVoiceProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      Thread ("VocalWriter render"),
      apvts (*this, nullptr, "state", makeLayout())
{
    if (auto guess = VoiceEngine::guessAssetDir(); guess != juce::File())
        setAssetPath (guess);
    startThread (juce::Thread::Priority::low);
}

VocalWriterVoiceProcessor::~VocalWriterVoiceProcessor()
{
    signalThreadShouldExit();
    notify();
    stopThread (2000);
}

// -- setting up ------------------------------------------------------------

juce::String VocalWriterVoiceProcessor::assetPath() const { return assetDir; }

bool VocalWriterVoiceProcessor::setAssetPath (const juce::File& dir)
{
    const bool ok = voice.open (dir);
    assetDir = dir.getFullPathName();
    if (ok)
    {
        say ("VocalWriter's voices are loaded: " + juce::String (voice.voiceCount())
             + " of them");
        requestRender();
    }
    else
    {
        say (voice.lastError());
    }
    return ok;
}

void VocalWriterVoiceProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate;
    readCursor = 0.0;
}

bool VocalWriterVoiceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

// -- the audio thread ------------------------------------------------------

void VocalWriterVoiceProcessor::captureMidi (const juce::MidiBuffer& midi,
                                             const juce::AudioPlayHead::PositionInfo& pos,
                                             double hostRate, int)
{
    if (midi.isEmpty())
        return;

    const double bpm = pos.getBpm().orFallback (120.0);
    const double startPpq = pos.getPpqPosition().orFallback (0.0);
    const double beatsPerSample = bpm / 60.0 / hostRate;

    /*  The audio thread never waits for the part: a capture missed because the
        editor happened to be holding it is one pass of the transport, and the
        next pass collects it. */
    const juce::ScopedTryLock lock (partLock);
    if (! lock.isLocked())
        return;

    bool changed = false;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const double at = startPpq + meta.samplePosition * beatsPerSample;

        if (msg.isNoteOn())
        {
            sounding[msg.getNoteNumber()] = { at, msg.getVelocity() };
        }
        else if (msg.isNoteOff() || msg.isAllNotesOff())
        {
            auto it = sounding.find (msg.getNoteNumber());
            if (it == sounding.end())
                continue;
            Note n;
            n.startBeats = it->second.first;
            n.beats      = juce::jmax (1.0 / 32.0, at - it->second.first);
            n.midi       = msg.getNoteNumber();
            n.velocity   = it->second.second;
            sounding.erase (it);

            /*  A note captured again -- a second pass over the same bar -- is
                the same note, not another one. */
            auto same = std::find_if (part.notes.begin(), part.notes.end(),
                                      [&n] (const Note& o)
                                      {
                                          return o.midi == n.midi
                                              && std::abs (o.startBeats - n.startBeats) < 1.0e-4;
                                      });
            if (same != part.notes.end())
            {
                if (std::abs (same->beats - n.beats) < 1.0e-4)
                    continue;            // nothing new
                same->beats = n.beats;
            }
            else
            {
                part.notes.push_back (std::move (n));
            }
            changed = true;
        }
    }
    if (changed)
    {
        partNoteCount.store ((int) part.notes.size());
        dirty.store (true);
        notify();
    }
}

void VocalWriterVoiceProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    juce::AudioPlayHead::PositionInfo pos;
    if (auto* ph = getPlayHead())
        if (auto p = ph->getPosition())
            pos = *p;

    const double bpm = pos.getBpm().orFallback (120.0);
    if (std::abs (bpm - lastBpm.load()) > 1.0e-6)
    {
        /*  The engine converts a note's length in beats with the tempo in
            force, so a tempo change makes the render stale. */
        lastBpm.store (bpm);
        dirty.store (true);
        notify();
    }

    if (pos.getIsPlaying())
        captureMidi (midi, pos, hostSampleRate, numSamples);
    midi.clear();

    const juce::SpinLock::ScopedTryLockType lock (renderLock);
    if (! lock.isLocked() || rendered == nullptr || rendered->audio.getNumSamples() == 0)
        return;

    const auto& src = rendered->audio;
    const int srcLen = src.getNumSamples();

    /*  Where the playhead is, in the rendered audio. The part begins at the
        start of the timeline, so a beat position converts straight across. */
    if (pos.getIsPlaying())
    {
        const double ppq = pos.getPpqPosition().orFallback (0.0);
        readCursor = ppq * (60.0 / juce::jmax (1.0e-6, rendered->bpm)) * kEngineSampleRate;
    }
    else
    {
        return;                          // stopped: nothing to sing
    }

    /*  The engine renders at 44100 and nothing else; anything the host asks
        for is reached from there. Linear interpolation is enough for a voice
        whose top formant is under 5 kHz. */
    const double step = kEngineSampleRate / hostSampleRate;
    const float level = apvts.getRawParameterValue ("level")->load();
    const int outChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const double at = readCursor + i * step;
        if (at < 0.0 || at >= srcLen - 1)
            continue;
        const int k = (int) at;
        const float frac = (float) (at - k);
        for (int ch = 0; ch < outChannels; ++ch)
        {
            const float* s = src.getReadPointer (juce::jmin (ch, src.getNumChannels() - 1));
            buffer.getWritePointer (ch)[i] = level * (s[k] + frac * (s[k + 1] - s[k]));
        }
    }
    readCursor += numSamples * step;
}

// -- rendering -------------------------------------------------------------

VoiceControls VocalWriterVoiceProcessor::controlsFromParameters() const
{
    VoiceControls c;
    auto get = [this] (const char* id) {
        return (int) std::lround (apvts.getRawParameterValue (id)->load());
    };
    c.colour      = get ("colour");
    c.vibrato     = get ("vibrato");
    c.vibratoRate = get ("vibratoRate");
    c.breath      = get ("breath");
    c.portamento  = get ("portamento");
    c.detune      = get ("detune");
    return c;
}

int VocalWriterVoiceProcessor::voiceIndexFromParameters() const
{
    const int wanted = (int) std::lround (apvts.getRawParameterValue ("voice")->load());
    return juce::jlimit (0, juce::jmax (0, voice.voiceCount() - 1), wanted);
}

void VocalWriterVoiceProcessor::requestRender()
{
    dirty.store (true);
    notify();
}

void VocalWriterVoiceProcessor::run()
{
    while (! threadShouldExit())
    {
        if (! dirty.exchange (false))
        {
            wait (200);
            continue;
        }
        if (! voice.isOpen())
            continue;

        Part copy;
        {
            const juce::ScopedLock lock (partLock);
            copy = part;
        }
        if (copy.isEmpty())
        {
            const juce::SpinLock::ScopedLockType lock (renderLock);
            rendered = nullptr;
            continue;
        }

        rendering.store (true);
        copy.makeMonophonic();
        applyDictionary (copy);

        /*  A part captured from the timeline starts wherever its first note
            does, and the engine sings from the first note it is given, so the
            silence in front has to be part of what it sings. */
        if (copy.notes.front().startBeats > 1.0e-6)
        {
            Note rest;
            rest.startBeats = 0.0;
            rest.beats = copy.notes.front().startBeats;
            copy.notes.insert (copy.notes.begin(), rest);
        }
        /*  Gaps between notes are rests too, for the same reason. */
        for (size_t i = 0; i + 1 < copy.notes.size(); ++i)
        {
            const double end = copy.notes[i].startBeats + copy.notes[i].beats;
            const double gap = copy.notes[i + 1].startBeats - end;
            if (gap > 1.0 / 64.0)
            {
                Note rest;
                rest.startBeats = end;
                rest.beats = gap;
                copy.notes.insert (copy.notes.begin() + (long) i + 1, rest);
                ++i;
            }
        }

        auto out = new RenderedAudio();
        out->bpm = lastBpm.load();
        out->audio = voice.render (copy, voiceIndexFromParameters(), out->bpm,
                                   controlsFromParameters());
        {
            const juce::SpinLock::ScopedLockType lock (renderLock);
            rendered = out;
        }
        rendering.store (false);
        say (juce::String ((int) copy.notes.size()) + " notes rendered, "
             + juce::String (out->audio.getNumSamples() / kEngineSampleRate, 1)
             + " seconds");
        if (onPartChanged != nullptr)
            juce::MessageManager::callAsync ([this] { if (onPartChanged) onPartChanged(); });
    }
}

void VocalWriterVoiceProcessor::applyDictionary (Part& copy)
{
    /*  A syllable ending in a hyphen is half a word, so the halves are joined
        back together, looked up once, and the pronunciation divided over the
        notes they came from -- the way VocalWriter's own import does it. */
    size_t i = 0;
    while (i < copy.notes.size())
    {
        if (copy.notes[i].word.isEmpty())
        {
            ++i;
            continue;
        }
        size_t last = i;
        juce::String whole;
        for (;;)
        {
            auto w = copy.notes[last].word;
            whole += w.endsWithChar ('-') ? w.dropLastCharacters (1) : w;
            if (! w.endsWithChar ('-') || last + 1 >= copy.notes.size())
                break;
            ++last;
        }

        const bool haveAll = [&]
        {
            for (size_t k = i; k <= last; ++k)
                if (copy.notes[k].phonemes.empty())
                    return false;
            return true;
        }();

        if (! haveAll)
        {
            auto syllables = voice.wordSyllables (whole.retainCharacters (
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'"));
            const size_t notes = last - i + 1;
            for (size_t k = 0; k < notes; ++k)
            {
                auto& n = copy.notes[i + k];
                if (k < syllables.size())
                    n.phonemes = syllables[k];
                else if (! syllables.empty())
                    /*  More notes than the word has syllables: the last one is
                        held over the rest, which is what singing a word over
                        several notes means. */
                    n.phonemes = syllables.back();
                else
                    /*  A word the dictionary does not know. The note still has
                        a pitch and a length, so it sings the open vowel rather
                        than falling silent, and the word stays on it to be
                        seen. */
                    n.phonemes = { "AA" };
            }
            /*  A word of more syllables than there are notes puts what is left
                on the last one, so nothing of it is lost. */
            for (size_t k = notes; k < syllables.size(); ++k)
                for (auto& p : syllables[k])
                    copy.notes[last].phonemes.push_back (p);
        }
        i = last + 1;
    }

    /*  Anything still with nothing on it sings the open vowel, so a part with
        no words at all arrives as something that can be heard. */
    for (auto& n : copy.notes)
        if (n.phonemes.empty() && n.word.isEmpty() && n.velocity > 0)
            n.phonemes = { "AA" };
}

// -- what the editor asks for ----------------------------------------------

Part VocalWriterVoiceProcessor::currentPart() const
{
    const juce::ScopedLock lock (partLock);
    return part;
}

void VocalWriterVoiceProcessor::setLyrics (const juce::String& text)
{
    {
        const juce::ScopedLock lock (partLock);
        lyricText = text;
        part.makeMonophonic();
        part.setLyrics (text);
    }
    requestRender();
}

void VocalWriterVoiceProcessor::clearPart()
{
    {
        const juce::ScopedLock lock (partLock);
        part.notes.clear();
        sounding.clear();
    }
    partNoteCount.store (0);
    requestRender();
    say ("notes cleared. Play the part again to capture it.");
}

juce::String VocalWriterVoiceProcessor::status() const
{
    if (! voice.isOpen())
        return voice.lastError().isNotEmpty() ? voice.lastError()
                                              : "VocalWriter's data files have not been found yet";
    if (rendering.load())
        return "rendering";
    const int n = partNoteCount.load();
    if (n == 0)
        return "no notes yet. Play the part once and they are captured.";
    return juce::String (n) + (n == 1 ? " note" : " notes") + " ready";
}

void VocalWriterVoiceProcessor::say (const juce::String& text)
{
    const juce::ScopedLock lock (sayLock);
    pending = text;
}

juce::String VocalWriterVoiceProcessor::takeAnnouncement()
{
    const juce::ScopedLock lock (sayLock);
    auto out = pending;
    pending.clear();
    return out;
}

// -- state -----------------------------------------------------------------

void VocalWriterVoiceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("assets", assetDir, nullptr);
    state.setProperty ("lyrics", lyrics(), nullptr);
    state.appendChild (currentPart().toValueTree(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalWriterVoiceProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;

    auto saved = state.getChildWithName ("part");
    auto restored = Part::fromValueTree (saved);
    state.removeChild (saved, nullptr);

    {
        const juce::ScopedLock lock (partLock);
        part = restored;
        lyricText = state.getProperty ("lyrics", juce::String());
    }
    partNoteCount.store ((int) restored.notes.size());

    auto dir = state.getProperty ("assets", juce::String()).toString();
    if (dir.isNotEmpty() && ! voice.isOpen())
        setAssetPath (juce::File (dir));

    apvts.replaceState (state);
    requestRender();
}

juce::AudioProcessorEditor* VocalWriterVoiceProcessor::createEditor()
{
    return new VocalWriterVoiceEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalWriterVoiceProcessor();
}
