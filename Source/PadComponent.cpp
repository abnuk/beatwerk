#include "PadComponent.h"

PadComponent::PadComponent (const PadInfo& info, SampleEngine& engine)
    : padInfo (info), sampleEngine (engine)
{
    updateSampleDisplay();
}

void PadComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    // Background
    juce::Colour bgColour = DarkLookAndFeel::padDefault;
    if (sampleEngine.hasSample (padInfo.midiNote))
        bgColour = DarkLookAndFeel::padLoaded;
    if (isDragOver)
        bgColour = DarkLookAndFeel::padHover;

    g.setColour (bgColour);
    g.fillRoundedRectangle (bounds, 6.0f);

    // Flash overlay
    if (flashAlpha > 0.01f)
    {
        g.setColour (DarkLookAndFeel::triggerFlash.withAlpha (flashAlpha * flashVelocity));
        g.fillRoundedRectangle (bounds, 6.0f);
    }

    // Border
    g.setColour (DarkLookAndFeel::bgLight.brighter (0.3f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.5f);

    // Pad name
    g.setColour (DarkLookAndFeel::textBright);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    auto textArea = bounds.reduced (6.0f);
    g.drawText (juce::String (padInfo.padName), textArea.removeFromTop (18.0f),
                juce::Justification::centredLeft);

    // Trigger name
    g.setColour (DarkLookAndFeel::accent);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String (padInfo.triggerName), textArea.removeFromTop (15.0f),
                juce::Justification::centredLeft);

    // MIDI note
    g.setColour (DarkLookAndFeel::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("MIDI " + juce::String (padInfo.midiNote),
                textArea.removeFromTop (13.0f), juce::Justification::centredLeft);

    // Sample name
    if (sampleName.isNotEmpty())
    {
        g.setColour (DarkLookAndFeel::textBright.withAlpha (0.8f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawFittedText (sampleName, textArea.toNearestInt(),
                          juce::Justification::centredLeft, 2);
    }
    else
    {
        g.setColour (DarkLookAndFeel::textDim.withAlpha (0.4f));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("Drop sample here", textArea, juce::Justification::centred);
    }
}

void PadComponent::resized() {}

void PadComponent::mouseDown (const juce::MouseEvent& /*event*/)
{
    // Preview: trigger sample at medium velocity
    if (sampleEngine.hasSample (padInfo.midiNote))
    {
        sampleEngine.noteOn (padInfo.midiNote, 0.7f);
        triggerFlash (0.7f);
    }
}

bool PadComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File (f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3")
            return true;
    }
    return false;
}

void PadComponent::fileDragEnter (const juce::StringArray& /*files*/, int, int)
{
    isDragOver = true;
    repaint();
}

void PadComponent::fileDragExit (const juce::StringArray& /*files*/)
{
    isDragOver = false;
    repaint();
}

void PadComponent::filesDropped (const juce::StringArray& files, int, int)
{
    isDragOver = false;

    for (auto& f : files)
    {
        juce::File file (f);
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3")
        {
            sampleEngine.loadSample (padInfo.midiNote, file);
            updateSampleDisplay();

            if (onSampleDropped)
                onSampleDropped (padInfo.midiNote, file);
            break;
        }
    }

    repaint();
}

void PadComponent::timerCallback()
{
    flashAlpha -= 0.08f;
    if (flashAlpha <= 0.0f)
    {
        flashAlpha = 0.0f;
        stopTimer();
    }
    repaint();
}

void PadComponent::triggerFlash (float velocity)
{
    flashAlpha = 1.0f;
    flashVelocity = velocity;
    startTimerHz (30);
}

void PadComponent::updateSampleDisplay()
{
    sampleName = sampleEngine.getSampleName (padInfo.midiNote);
    repaint();
}
