#include "PluginEditor.h"

VocalWriterVoiceEditor::VocalWriterVoiceEditor (VocalWriterVoiceProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    /*  The window itself is not a place to stand: it has nothing to say, and
        a stop with no name on it is a stop a reader cannot account for. The
        controls inside it take the focus, which is what tabbing wants. It is
        still named, so anything that describes the container has something to
        describe it by. */
    setTitle ("VocalWriter Voice");
    setWantsKeyboardFocus (false);
    setFocusContainerType (juce::Component::FocusContainerType::focusContainer);

    titleLabel.setText ("VocalWriter Voice", juce::dontSendNotification);
    titleLabel.setTitle ("VocalWriter Voice");
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    titleLabel.setAccessible (false);      // the window is already called this
    addAndMakeVisible (titleLabel);

    statusLabel.setTitle ("Status");
    statusLabel.setHelpText ("What the plugin is doing, and whether it has notes to sing.");
    addAndMakeVisible (statusLabel);

    // -- the words ---------------------------------------------------------

    lyricsLabel.setText ("Words", juce::dontSendNotification);
    lyricsLabel.attachToComponent (&lyrics, false);
    lyricsLabel.setAccessible (false);     // the box below it says "Words to sing"
    addAndMakeVisible (lyricsLabel);

    lyrics.setMultiLine (true, true);
    lyrics.setReturnKeyStartsNewLine (true);
    lyrics.setTitle ("Words to sing");
    lyrics.setHelpText ("One syllable per note, in the order the notes come. Separate syllables "
                        "of a word with a hyphen, as in twin-kle twin-kle lit-tle star. "
                        "Notes with nothing typed for them sing AA, the open vowel of father.");
    lyrics.setText (processor.lyrics(), juce::dontSendNotification);
    lyrics.onFocusLost = [this]
    {
        if (lyrics.getText() != processor.lyrics())
        {
            processor.setLyrics (lyrics.getText());
            announce ("words set");
        }
    };
    addAndMakeVisible (lyrics);

    // -- the voice ---------------------------------------------------------

    voiceLabel.setText ("Voice", juce::dontSendNotification);
    voiceLabel.attachToComponent (&voices, false);
    voiceLabel.setAccessible (false);      // the combo below it says "Voice"
    addAndMakeVisible (voiceLabel);

    voices.setTitle ("Voice");
    voices.setHelpText ("Which of VocalWriter's voices sings this part. The ones with "
                        "instrument names are built on the wavetables and sing lyrics "
                        "like any other.");
    addAndMakeVisible (voices);
    refreshVoices();

    // -- the controls ------------------------------------------------------

    addSlider ("colour", "Colour", "Brighter as the voice rises. The engine's own default is 95.");
    addSlider ("vibrato", "Vibrato depth", "How far the pitch wavers. 0 for none.");
    addSlider ("vibratoRate", "Vibrato rate", "How fast it wavers.");
    addSlider ("breath", "Breath", "Adds air to the tone.");
    addSlider ("portamento", "Portamento", "Glide between notes. 0 steps straight to the pitch.");
    addSlider ("detune", "Voice detune", "The engine's own detune, which hardly moves the pitch.");
    addSlider ("level", "Level", "How loud this voice is. 1 is the engine's own level.");

    // -- the buttons -------------------------------------------------------

    clearButton.setTitle ("Clear notes");
    clearButton.setHelpText ("Forget the notes captured from the track. Play the part again "
                             "and they are collected afresh.");
    clearButton.onClick = [this]
    {
        processor.clearPart();
        announce ("notes cleared");
    };
    addAndMakeVisible (clearButton);

    assetsButton.setTitle ("Find VocalWriter's files");
    assetsButton.setHelpText ("Choose the folder holding VocalWriter's own data files: "
                              "VocalWriter.rsrc, GMSpeech.rsrc, GMBank.rsrc and EnglishLex. "
                              "They are not part of this plugin and it cannot sing without them.");
    assetsButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> (
            "Where are VocalWriter's data files?",
            juce::File (processor.assetPath()));
        chooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto dir = fc.getResult();
                                  if (dir == juce::File())
                                      return;
                                  if (processor.setAssetPath (dir))
                                  {
                                      refreshVoices();
                                      announce ("voices loaded");
                                  }
                                  else
                                  {
                                      announce (processor.engine().lastError());
                                  }
                              });
    };
    addAndMakeVisible (assetsButton);

    processor.onPartChanged = [this] { refreshStatus(); };

    refreshStatus();
    setSize (520, 560);
    startTimer (300);
}

VocalWriterVoiceEditor::~VocalWriterVoiceEditor()
{
    processor.onPartChanged = nullptr;
}

juce::Slider& VocalWriterVoiceEditor::addSlider (const juce::String& id,
                                                 const juce::String& name,
                                                 const juce::String& help)
{
    auto* label = sliderLabels.add (new juce::Label ({}, name));
    auto* slider = sliders.add (new juce::Slider (juce::Slider::LinearHorizontal,
                                                  juce::Slider::TextBoxRight));

    /*  The value box beside a slider is an editable Label, which is a keyboard
        stop of its own and reports itself as an edit field. So tabbing landed
        on it rather than on the slider, and every one of these read out as
        "edit, selected, 95" -- the value with nothing to say what it was the
        value of, because the name is on the slider behind it.

        Read-only takes the box out of the tab order without taking it off the
        screen, so the value stays visible and the stop is the slider itself,
        which has the name and announces both. Typing a number straight in goes
        with it; the arrow keys move the slider, which is how it is reached
        from the keyboard anyway. */
    slider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 72, 20);

    slider->setTitle (name);
    slider->setName (name);
    slider->setHelpText (help);
    slider->setWantsKeyboardFocus (true);

    /*  The label repeats what the slider already announces, so it is one more
        thing to read past on the way down the window rather than information.
        It stays for the eye and goes for the reader. */
    label->attachToComponent (slider, true);
    label->setAccessible (false);
    addAndMakeVisible (label);
    addAndMakeVisible (slider);

    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        processor.apvts, id, *slider));
    return *slider;
}

void VocalWriterVoiceEditor::refreshVoices()
{
    voiceAttachment.reset();
    voices.clear (juce::dontSendNotification);

    auto& engine = processor.engine();
    const int n = engine.voiceCount();
    for (int i = 0; i < n; ++i)
    {
        auto name = engine.voiceName (i);
        voices.addItem (name.isNotEmpty() ? name : ("voice " + juce::String (i + 1)), i + 1);
    }
    if (n > 0)
        voiceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, "voice", voices);
    voices.setEnabled (n > 0);
}

void VocalWriterVoiceEditor::refreshStatus()
{
    auto text = processor.status();
    if (text == lastStatus)
        return;
    lastStatus = text;
    statusLabel.setText (text, juce::dontSendNotification);
}

void VocalWriterVoiceEditor::announce (const juce::String& text)
{
    if (text.isEmpty())
        return;
    juce::AccessibilityHandler::postAnnouncement (
        text, juce::AccessibilityHandler::AnnouncementPriority::high);
}

void VocalWriterVoiceEditor::timerCallback()
{
    refreshStatus();
    announce (processor.takeAnnouncement());
}

void VocalWriterVoiceEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void VocalWriterVoiceEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (26));
    statusLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (14);

    lyrics.setBounds (area.removeFromTop (90));
    area.removeFromTop (18);
    voices.setBounds (area.removeFromTop (24).withTrimmedLeft (110));
    area.removeFromTop (14);

    for (auto* s : sliders)
    {
        s->setBounds (area.removeFromTop (26).withTrimmedLeft (110));
        area.removeFromTop (6);
    }

    area.removeFromTop (10);
    auto row = area.removeFromTop (28);
    clearButton.setBounds (row.removeFromLeft (140));
    row.removeFromLeft (10);
    assetsButton.setBounds (row.removeFromLeft (200));
}
