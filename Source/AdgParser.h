#pragma once
#include <juce_core/juce_core.h>
#include <map>

struct AdgSampleMapping
{
    int midiNote;
    juce::String samplePath;     // resolved absolute path
    juce::String sampleName;
};

struct AdgDrumKit
{
    juce::String kitName;
    juce::File sourceFile;
    std::vector<AdgSampleMapping> mappings;
};

class AdgParser
{
public:
    AdgParser();

    void setAbletonLibraryPath (const juce::File& path);
    juce::File getAbletonLibraryPath() const { return abletonLibraryPath; }

    AdgDrumKit parseFile (const juce::File& adgFile) const;

    static juce::File autoDetectAbletonLibrary();

private:
    juce::File abletonLibraryPath;

    juce::String resolveRelativePath (const juce::String& relativePath, int pathType) const;
    void parseBranch (juce::XmlElement* branch, std::vector<AdgSampleMapping>& mappings) const;
    juce::String findSamplePath (juce::XmlElement* element) const;
};
