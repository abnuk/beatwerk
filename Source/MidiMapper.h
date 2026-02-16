#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <string>

struct PadInfo
{
    int midiNote;
    std::string padName;
    std::string triggerName;
};

class MidiMapper
{
public:
    MidiMapper();

    bool isPadNote (int midiNote) const;
    const PadInfo* getPadInfo (int midiNote) const;

    static const std::array<PadInfo, 24>& getAllPads();

    // Navigation MIDI config
    void setNavChannel (int channel);   // 1-16, 0 = any
    void setNavCCNumber (int cc);
    int getNavChannel() const { return navChannel; }
    int getNavCCNumber() const { return navCCNumber; }

    enum class NavAction { None, Next, Previous };

    NavAction processForNavigation (const juce::MidiMessage& msg) const;
    bool isDrumTrigger (const juce::MidiMessage& msg) const;

private:
    int navChannel = 0;   // 0 = any channel
    int navCCNumber = 1;  // CC#1 by default

    static const std::array<PadInfo, 24> pads;
};
