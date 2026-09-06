/*  PluginEditor.h -- the window, built to be read out loud.

    Every control carries a title and a help text, because that is what a
    screen reader says when it lands on one, and anything that happens without
    the keyboard causing it is announced. This is the same arrangement the
    Studio uses -- a list you arrow through and a line that says what just
    happened -- rather than a picture of a mixer.
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class VocalWriterVoiceEditor : public juce::AudioProcessorEditor,
                               private juce::Timer
{
public:
    explicit VocalWriterVoiceEditor (VocalWriterVoiceProcessor&);
    ~VocalWriterVoiceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void announce (const juce::String& text);
    void refreshVoices();
    void refreshStatus();
    juce::Slider& addSlider (const juce::String& id, const juce::String& name,
                             const juce::String& help);

    VocalWriterVoiceProcessor& processor;

    juce::Label  titleLabel, statusLabel;
    juce::Label  lyricsLabel;
    juce::TextEditor lyrics;
    juce::Label  voiceLabel;
    juce::ComboBox voices;
    juce::TextButton clearButton { "Clear notes" };
    juce::TextButton assetsButton { "Find VocalWriter's files" };

    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> sliderLabels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voiceAttachment;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String lastStatus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalWriterVoiceEditor)
};
