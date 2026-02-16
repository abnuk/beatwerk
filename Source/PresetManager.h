#pragma once
#include <juce_core/juce_core.h>
#include "AdgParser.h"
#include <vector>
#include <functional>

class PresetManager
{
public:
    PresetManager (AdgParser& parser);

    void scanForPresets();
    void addScanDirectory (const juce::File& dir);
    void clearScanDirectories();

    int getNumPresets() const;
    juce::String getPresetName (int index) const;
    int getCurrentPresetIndex() const { return currentIndex; }

    bool loadPreset (int index);
    bool loadNextPreset();
    bool loadPreviousPreset();

    const AdgDrumKit& getCurrentKit() const { return currentKit; }

    // Save/load custom JSON presets
    bool saveCustomPreset (const juce::String& name,
                           const std::map<int, juce::File>& padMappings);
    bool loadCustomPreset (const juce::File& jsonFile);

    // Callback for when a preset is loaded
    std::function<void(const AdgDrumKit&)> onPresetLoaded;

    juce::File getCustomPresetsDir() const;

private:
    struct PresetEntry
    {
        juce::String name;
        juce::File file;
        bool isCustom = false;
    };

    AdgParser& adgParser;
    std::vector<PresetEntry> presets;
    std::vector<juce::File> scanDirectories;
    int currentIndex = -1;
    AdgDrumKit currentKit;

    void scanDirectory (const juce::File& dir);
};
