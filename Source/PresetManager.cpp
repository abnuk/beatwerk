#include "PresetManager.h"
#include <fstream>

static juce::File getLogFile()
{
    return juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
               .getChildFile ("mps_drum_debug.log");
}

static void logPM (const std::string& msg)
{
    getLogFile().appendText ("[PresetMgr] " + juce::String (msg) + "\n");
}

PresetManager::PresetManager (AdgParser& parser) : adgParser (parser) {}

void PresetManager::addScanDirectory (const juce::File& dir)
{
    if (dir.isDirectory())
    {
        scanDirectories.push_back (dir);
        logPM ("addScanDirectory: " + dir.getFullPathName().toStdString());
    }
}

void PresetManager::clearScanDirectories()
{
    scanDirectories.clear();
}

void PresetManager::scanForPresets()
{
    presets.clear();

    logPM ("scanForPresets: scanning " + std::to_string (scanDirectories.size()) + " directories");

    for (auto& dir : scanDirectories)
        scanDirectory (dir);

    auto customDir = getCustomPresetsDir();
    if (customDir.isDirectory())
    {
        auto jsonFiles = customDir.findChildFiles (juce::File::findFiles, true, "*.json");
        for (auto& f : jsonFiles)
        {
            PresetEntry entry;
            entry.name = f.getFileNameWithoutExtension();
            entry.file = f;
            entry.isCustom = true;
            presets.push_back (entry);
        }
    }

    std::sort (presets.begin(), presets.end(), [] (const PresetEntry& a, const PresetEntry& b)
    {
        return a.name.compareIgnoreCase (b.name) < 0;
    });

    logPM ("scanForPresets: total presets found = " + std::to_string (presets.size()));
}

void PresetManager::scanDirectory (const juce::File& dir)
{
    if (! dir.isDirectory())
        return;

    auto adgFiles = dir.findChildFiles (juce::File::findFiles, true, "*.adg");
    adgFiles.sort();

    logPM ("scanDirectory: " + dir.getFullPathName().toStdString() + " found " + std::to_string (adgFiles.size()) + " .adg files");

    for (auto& f : adgFiles)
    {
        PresetEntry entry;
        entry.name = f.getFileNameWithoutExtension();
        entry.file = f;
        entry.isCustom = false;
        presets.push_back (entry);
    }
}

int PresetManager::getNumPresets() const
{
    return (int) presets.size();
}

juce::String PresetManager::getPresetName (int index) const
{
    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;
    return {};
}

bool PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= (int) presets.size())
    {
        logPM ("loadPreset: index " + std::to_string (index) + " out of range (size=" + std::to_string (presets.size()) + ")");
        return false;
    }

    auto& entry = presets[(size_t) index];
    logPM ("loadPreset: index=" + std::to_string (index) + " name=" + entry.name.toStdString() + " file=" + entry.file.getFullPathName().toStdString());

    if (entry.isCustom)
    {
        if (! loadCustomPreset (entry.file))
            return false;
    }
    else
    {
        currentKit = adgParser.parseFile (entry.file);
        logPM ("loadPreset: parsed kit has " + std::to_string (currentKit.mappings.size()) + " mappings");
    }

    currentIndex = index;

    if (onPresetLoaded)
    {
        logPM ("loadPreset: calling onPresetLoaded callback");
        onPresetLoaded (currentKit);
    }
    else
    {
        logPM ("loadPreset: WARNING - onPresetLoaded callback is null!");
    }

    return true;
}

bool PresetManager::loadNextPreset()
{
    if (presets.empty())
        return false;

    int next = currentIndex + 1;
    if (next >= (int) presets.size())
        next = 0;

    return loadPreset (next);
}

bool PresetManager::loadPreviousPreset()
{
    if (presets.empty())
        return false;

    int prev = currentIndex - 1;
    if (prev < 0)
        prev = (int) presets.size() - 1;

    return loadPreset (prev);
}

juce::File PresetManager::getCustomPresetsDir() const
{
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return appData.getChildFile ("MPSDrumMachine/Presets");
}

bool PresetManager::saveCustomPreset (const juce::String& name,
                                       const std::map<int, juce::File>& padMappings)
{
    auto dir = getCustomPresetsDir();
    dir.createDirectory();

    auto file = dir.getChildFile (name + ".json");

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("name", name);

    juce::Array<juce::var> padsArray;
    for (auto& [note, sampleFile] : padMappings)
    {
        juce::DynamicObject::Ptr pad = new juce::DynamicObject();
        pad->setProperty ("midiNote", note);
        pad->setProperty ("samplePath", sampleFile.getFullPathName());
        pad->setProperty ("sampleName", sampleFile.getFileNameWithoutExtension());
        padsArray.add (juce::var (pad.get()));
    }

    root->setProperty ("pads", padsArray);

    auto jsonText = juce::JSON::toString (juce::var (root.get()));
    return file.replaceWithText (jsonText);
}

bool PresetManager::loadCustomPreset (const juce::File& jsonFile)
{
    auto jsonText = jsonFile.loadFileAsString();
    auto parsed = juce::JSON::parse (jsonText);

    if (! parsed.isObject())
        return false;

    currentKit = AdgDrumKit();
    currentKit.kitName = parsed.getProperty ("name", "Custom").toString();
    currentKit.sourceFile = jsonFile;

    auto padsArray = parsed.getProperty ("pads", juce::var());
    if (padsArray.isArray())
    {
        for (int i = 0; i < padsArray.size(); ++i)
        {
            auto padVar = padsArray[i];
            AdgSampleMapping mapping;
            mapping.midiNote = (int) padVar.getProperty ("midiNote", -1);
            mapping.samplePath = padVar.getProperty ("samplePath", "").toString();
            mapping.sampleName = padVar.getProperty ("sampleName", "").toString();
            if (mapping.midiNote >= 0)
                currentKit.mappings.push_back (mapping);
        }
    }

    return true;
}
