# VocalWriter Voice

One VocalWriter voice as a VST3, for singing a part written in a DAW.

`engine/include/vw_editor.h` says *"One editor is one voice with its own
context"*, so one plugin instance is one voice: put it on a track, give the
track a melody and the plugin some words, and it sings them. A second voice is
a second track with a second instance on it.

## Building

    powershell -ExecutionPolicy Bypass -File vst/build_plugin.ps1

Needs Visual Studio with the C++ toolset and CMake. JUCE is cloned on the first
build; the engine comes from the `engine` submodule and is compiled straight
into the plugin. To install, copy
`build/VocalWriterVoice_artefacts/Release/VST3/VocalWriter Voice.vst3` into
`C:\Program Files\Common Files\VST3\` and re-scan in Reaper.

`rendertest.exe`, built alongside, sings a scale and checks each note comes out
on the pitch it was given.

## VocalWriter's data files

None of VocalWriter is in the plugin. It needs the same four files the rest of
this repository needs -- `VocalWriter.rsrc`, `GMSpeech.rsrc`, `GMBank.rsrc` and
`EnglishLex` -- and finds the `assets` folder on its own where it can. Where it
cannot, **Find VocalWriter's files** points it at them, and the choice is saved
with the project.

## How the notes get in, and why it works that way

The plugin **writes the track's MIDI down** rather than singing it as it
arrives, and that is the engine's doing rather than a preference:

- A note's length in beats has to be handed over at the moment the note
  begins. MIDI does not say how long a note is until the note-off, which is
  after that moment has passed.
- A phrase's phonemes have to be packed into one block and handed over before
  any of the phrase sounds (`vw_ed_sequence`, then `vw_ed_start`).
- The engine writes into a buffer it grows as it goes, which is not something
  to do on the audio thread.

So while the transport runs, note-ons and note-offs are collected against their
place on the host's timeline. When they stop changing, the part is rendered in
full on a background thread, and playing it back is a read out of a finished
buffer at the playhead. **Play the section once and the plugin has it**; after
that it sings, and re-renders itself whenever the MIDI, the words, the voice or
the tempo change.

The upside of rendering ahead is that the plugin reports no latency, and the
engine never has to be real-time safe.

## The words

One syllable per note, in the order the notes come, typed into the **Words**
box:

    twin-kle twin-kle lit-tle star

A syllable ending in a hyphen is half of a word: the halves are joined back
together, looked up in VocalWriter's own dictionary once, and the pronunciation
divided over the notes they came from. A word the dictionary does not know
sings the open vowel and keeps its text, so you can see which one it was. Notes
with nothing typed for them sing `AA`, so a part with no words at all still
makes a sound you can check the tune against.

## What comes from the host

Tempo and the playhead, per the user's ask. Chorus and reverb are deliberately
not here -- reverb belongs on the track, where the host has better ones. The
engine's own voice controls that remain are Colour, Vibrato depth, Vibrato
rate, Breath, Portamento and Voice detune, all automatable.

## Reading it out loud

Every control has a title and a help text, because that is what a screen reader
says when it lands on one, and anything that happens without a keypress causing
it is announced through `AccessibilityHandler::postAnnouncement`. That is the
same arrangement the Studio uses: something to arrow through, and a line that
says what just happened.
