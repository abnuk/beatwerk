#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "MidiMapper.h"
#include "SampleEngine.h"
#include "LookAndFeel.h"

class PadComponent : public juce::Component,
                     public juce::FileDragAndDropTarget,
                     public juce::Timer
{
public:
    PadComponent (const PadInfo& padInfo, SampleEngine& engine);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // Timer for flash animation
    void timerCallback() override;

    void triggerFlash (float velocity);
    void updateSampleDisplay();

    int getMidiNote() const { return padInfo.midiNote; }

    // Callback when a sample is dropped
    std::function<void(int midiNote, const juce::File& file)> onSampleDropped;

private:
    PadInfo padInfo;
    SampleEngine& sampleEngine;
    juce::String sampleName;
    bool isDragOver = false;
    float flashAlpha = 0.0f;
    float flashVelocity = 0.0f;
};
