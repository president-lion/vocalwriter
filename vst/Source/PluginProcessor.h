/*  PluginProcessor.h -- one VocalWriter voice as a VST3.

    What it does with the track's MIDI is the whole design, so: it writes it
    down. The engine cannot be fed a note at a time as MIDI arrives -- it is
    told a note's length in beats at the moment the note starts, and MIDI does
    not say how long a note is until the note-off, which is too late -- so
    while the transport runs the note-ons and note-offs are collected into a
    Part against their place on the host's timeline. When they stop arriving
    the Part is rendered in full on a background thread, and playing it back
    is then a read out of a finished buffer at the playhead.

    In practice: play the section once and the plugin has the notes; from then
    on it sings them, and it re-renders by itself whenever the MIDI, the
    words, the voice or the tempo change.
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <map>

#include "Part.h"
#include "VoiceEngine.h"

/*  A finished render, handed from the background thread to the audio thread
    whole. Reference counted so the audio thread can hold the one it is
    playing while the next is being swapped in. */
struct RenderedAudio : public juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<RenderedAudio>;
    juce::AudioBuffer<float> audio;      // stereo, at kEngineSampleRate
    double bpm = 120.0;
};

class VocalWriterVoiceProcessor : public juce::AudioProcessor,
                                  private juce::Thread
{
public:
    VocalWriterVoiceProcessor();
    ~VocalWriterVoiceProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true; }
    const juce::String getName() const override            { return JucePlugin_Name; }
    bool acceptsMidi() const override                      { return true; }
    bool producesMidi() const override                     { return false; }
    bool isMidiEffect() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0.0; }
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override       { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // -- what the editor talks to ------------------------------------------

    juce::AudioProcessorValueTreeState apvts;

    VoiceEngine& engine()                                  { return voice; }
    juce::String assetPath() const;
    /*  Point the plugin at the folder holding VocalWriter's data files, and
        try to open the engine on it. */
    bool setAssetPath (const juce::File& dir);

    /*  The notes as they stand, copied, for the editor to describe. */
    Part currentPart() const;
    int  noteCount() const                                 { return partNoteCount.load(); }
    bool isRendering() const                               { return rendering.load(); }
    juce::String status() const;

    /*  The words typed for this part. Setting them lays one syllable on each
        note, looks up what the dictionary knows, and re-renders. */
    juce::String lyrics() const                            { return lyricText; }
    void setLyrics (const juce::String& text);

    /*  Forget the captured notes; the next pass over the MIDI collects them
        again. */
    void clearPart();

    /*  Something the editor should say out loud when it next looks: a
        re-render finishing, a capture completing. Empty once read. */
    juce::String takeAnnouncement();

    std::function<void()> onPartChanged;

private:
    void run() override;                 // the render thread
    void requestRender();
    void applyDictionary (Part& part);
    void captureMidi (const juce::MidiBuffer& midi, const juce::AudioPlayHead::PositionInfo& pos,
                      double hostRate, int numSamples);
    VoiceControls controlsFromParameters() const;
    int voiceIndexFromParameters() const;

    VoiceEngine voice;
    juce::String assetDir;

    /*  The notes. Touched by the audio thread (capture) and the message and
        render threads (words, saving), so it is guarded -- the audio thread
        only ever tries the lock and drops the capture if it cannot have it,
        which costs a note that the next pass collects. */
    mutable juce::CriticalSection partLock;
    Part part;
    std::atomic<int> partNoteCount { 0 };
    juce::String lyricText;

    /*  Notes the transport has started but not yet finished, by pitch. */
    std::map<int, std::pair<double, int>> sounding;   // midi -> (startBeats, velocity)

    juce::SpinLock renderLock;
    RenderedAudio::Ptr rendered;
    std::atomic<bool> dirty { false };
    std::atomic<bool> rendering { false };
    std::atomic<double> lastBpm { 120.0 };

    double hostSampleRate = 44100.0;
    double readCursor = 0.0;             // where in the rendered audio we are

    juce::CriticalSection sayLock;
    juce::String pending;
    void say (const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalWriterVoiceProcessor)
};
