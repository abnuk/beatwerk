#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <array>
#include <atomic>
#include <functional>
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
    void setPrevCCNumber (int cc);
    void setNextCCNumber (int cc);
    int getNavChannel() const { return navChannel; }
    int getPrevCCNumber() const { return prevCCNumber; }
    int getNextCCNumber() const { return nextCCNumber; }

    enum class NavAction { None, Next, Previous };

    NavAction processForNavigation (const juce::MidiMessage& msg) const;
    bool isDrumTrigger (const juce::MidiMessage& msg) const;

    // MIDI Learn
    enum class LearnTarget { None, Prev, Next };

    void startLearn (LearnTarget target);
    void cancelLearn();
    bool isLearning() const;
    LearnTarget getLearnTarget() const;

    // Called from audio thread; returns true if the message was consumed by learn
    bool processForLearn (const juce::MidiMessage& msg);

    // Fired on the message thread when learn completes: (target, ccNumber)
    std::function<void (LearnTarget, int)> onLearnComplete;

private:
    int navChannel = 0;     // 0 = any channel
    int prevCCNumber = 1;   // CC#1 for previous preset
    int nextCCNumber = 2;   // CC#2 for next preset

    std::atomic<int> learnTarget { 0 }; // 0=None, 1=Prev, 2=Next

    static const std::array<PadInfo, 24> pads;
};
