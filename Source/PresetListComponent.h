#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetManager.h"
#include "LookAndFeel.h"

class PresetListContent : public juce::Component
{
public:
    PresetListContent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    void setPresetNames (const juce::StringArray& names);
    void setActiveIndex (int index);
    int getActiveIndex() const { return activeIndex; }

    int getFirstIndexForLetter (const juce::String& letter) const;

    std::function<void(int)> onPresetClicked;

    static constexpr int rowHeight = 40;

private:
    juce::StringArray presetNames;
    int activeIndex = -1;
};

class AlphabetBarComponent : public juce::Component
{
public:
    AlphabetBarComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    std::function<void(const juce::String&)> onLetterClicked;

private:
    juce::StringArray letters;
};

class PresetListComponent : public juce::Component
{
public:
    PresetListComponent (PresetManager& pm);

    void resized() override;
    void paint (juce::Graphics& g) override;

    void refreshPresetList();
    void setActivePreset (int index);

    std::function<void(int)> onPresetSelected;

private:
    PresetManager& presetManager;

    AlphabetBarComponent alphabetBar;
    juce::Viewport viewport;
    PresetListContent listContent;

    juce::TextButton upButton { "UP" };
    juce::TextButton downButton { "DOWN" };

    void scrollPageUp();
    void scrollPageDown();
    void ensureActiveVisible();
    void scrollToLetter (const juce::String& letter);
};
