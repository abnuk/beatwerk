#include "AdgParser.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <fstream>

static void logToFile (const juce::String& msg)
{
    auto logFile = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                       .getChildFile ("mps_drum_debug.log");
    logFile.appendText ("[AdgParser] " + msg + "\n");
}

AdgParser::AdgParser()
{
    abletonLibraryPath = autoDetectAbletonLibrary();
}

void AdgParser::setAbletonLibraryPath (const juce::File& path)
{
    abletonLibraryPath = path;
}

juce::File AdgParser::autoDetectAbletonLibrary()
{
    juce::File appsDir ("/Applications");
    auto children = appsDir.findChildFiles (juce::File::findDirectories, false, "Ableton Live*");

    children.sort();

    for (int i = children.size() - 1; i >= 0; --i)
    {
        auto coreLib = children[i].getChildFile ("Contents/App-Resources/Core Library");
        if (coreLib.isDirectory())
            return coreLib;
    }

    return {};
}

AdgDrumKit AdgParser::parseFile (const juce::File& adgFile) const
{
    AdgDrumKit kit;
    kit.sourceFile = adgFile;
    kit.kitName = adgFile.getFileNameWithoutExtension();

    juce::FileInputStream fileStream (adgFile);
    if (fileStream.failedToOpen())
    {
        logToFile ("AdgParser: failed to open file: " + adgFile.getFullPathName());
        return kit;
    }

    juce::GZIPDecompressorInputStream gzipStream (&fileStream, false,
                                                  juce::GZIPDecompressorInputStream::gzipFormat);

    juce::MemoryOutputStream memStream;
    memStream.writeFromInputStream (gzipStream, -1);
    auto xmlContent = memStream.toString();

    logToFile ("AdgParser: decompressed " + juce::String ((int) xmlContent.length()) + " bytes from " + adgFile.getFileName());

    if (xmlContent.isEmpty())
    {
        logToFile ("AdgParser: XML content is empty!");
        return kit;
    }

    auto xml = juce::XmlDocument::parse (xmlContent);
    if (xml == nullptr)
    {
        logToFile ("AdgParser: XML parse failed!");
        return kit;
    }

    logToFile ("AdgParser: root tag = " + xml->getTagName());

    // Ableton 12 drum rack .adg structure:
    // Ableton > GroupDevicePreset > BranchPresets > DrumBranchPreset[]
    // Each DrumBranchPreset has:
    //   ZoneSettings > ReceivingNote (MIDI note, typically 77-92 for 16-pad kits)
    //   DevicePresets > ... > SampleRef > FileRef > RelativePath

    // Collect all drum branch samples sorted by ReceivingNote
    struct ParsedPad
    {
        int receivingNote;
        juce::String samplePath;
        juce::String sampleName;
    };
    std::vector<ParsedPad> parsedPads;

    std::function<void(juce::XmlElement*)> findDrumBranches = [&] (juce::XmlElement* el)
    {
        if (el == nullptr) return;

        if (el->getTagName() == "DrumBranchPreset")
        {
            logToFile ("AdgParser: found DrumBranchPreset");
            parseBranch (el, kit.mappings);
            return;
        }

        for (auto* child : el->getChildIterator())
            findDrumBranches (child);
    };

    findDrumBranches (xml.get());

    logToFile ("AdgParser: found " + juce::String ((int) kit.mappings.size()) + " sample mappings");

    // Remap Ableton drum kit samples to MPS-1000 pads using filename-based matching.
    // Ableton kits use internal notes (77-92) that don't match the MPS-1000 (21-59).
    // We try to match by drum type keywords in sample filenames.

    struct PadSlot { int midiNote; const char* keywords; };
    static const PadSlot mpsSlots[] = {
        { 36, "kick bass 808 bd" },
        { 38, "snare sd" },
        { 37, "stick rim click clap snap" },
        { 40, "snare rim" },
        { 48, "tom high" },
        { 50, "tom" },
        { 45, "tom mid" },
        { 47, "tom" },
        { 43, "tom low floor" },
        { 58, "tom" },
        { 42, "hihat closed hat hh" },
        { 46, "hihat open hat" },
        { 23, "hihat hat" },
        { 44, "pedal hat" },
        { 21, "shaker tamb perc" },
        { 49, "crash cymbal" },
        { 55, "crash" },
        { 57, "crash cymbal" },
        { 52, "crash cymbal" },
        { 51, "ride cymbal" },
        { 53, "ride bell cowbell" },
        { 59, "ride" },
        { 41, "perc conga bongo wood" },
        { 39, "perc fx synth" }
    };

    auto nameContainsAny = [] (const juce::String& name, const char* keywords) -> bool
    {
        auto lower = name.toLowerCase();
        juce::StringArray words;
        words.addTokens (juce::String (keywords), " ", "");
        for (auto& w : words)
            if (lower.contains (w))
                return true;
        return false;
    };

    std::vector<AdgSampleMapping> remapped;
    std::vector<bool> used (kit.mappings.size(), false);

    // First pass: try keyword matching for each MPS pad
    for (auto& slot : mpsSlots)
    {
        int bestIdx = -1;
        for (size_t i = 0; i < kit.mappings.size(); ++i)
        {
            if (used[i]) continue;
            if (nameContainsAny (kit.mappings[i].sampleName, slot.keywords))
            {
                bestIdx = (int) i;
                break;
            }
        }

        if (bestIdx >= 0)
        {
            auto m = kit.mappings[(size_t) bestIdx];
            m.midiNote = slot.midiNote;
            remapped.push_back (m);
            used[(size_t) bestIdx] = true;
        }
    }

    // Second pass: assign remaining unmatched samples to empty MPS slots
    std::vector<int> emptySlots;
    for (auto& slot : mpsSlots)
    {
        bool taken = false;
        for (auto& r : remapped)
            if (r.midiNote == slot.midiNote) { taken = true; break; }
        if (! taken)
            emptySlots.push_back (slot.midiNote);
    }

    size_t emptyIdx = 0;
    for (size_t i = 0; i < kit.mappings.size(); ++i)
    {
        if (used[i] || emptyIdx >= emptySlots.size()) continue;
        auto m = kit.mappings[i];
        m.midiNote = emptySlots[emptyIdx++];
        remapped.push_back (m);
    }

    kit.mappings = remapped;

    return kit;
}

void AdgParser::parseBranch (juce::XmlElement* branch, std::vector<AdgSampleMapping>& mappings) const
{
    if (branch == nullptr)
        return;

    // Find ReceivingNote inside ZoneSettings
    int midiNote = -1;

    auto* zoneSettings = branch->getChildByName ("ZoneSettings");
    if (zoneSettings != nullptr)
    {
        auto* receivingNote = zoneSettings->getChildByName ("ReceivingNote");
        if (receivingNote != nullptr)
            midiNote = receivingNote->getIntAttribute ("Value", -1);
    }

    if (midiNote < 0 || midiNote > 127)
        return;

    // Find sample file path
    juce::String samplePath = findSamplePath (branch);
    logToFile ("AdgParser: branch note=" + juce::String (midiNote) + " samplePath=" + samplePath);

    if (samplePath.isEmpty())
        return;

    AdgSampleMapping mapping;
    mapping.midiNote = midiNote;
    mapping.samplePath = samplePath;
    mapping.sampleName = juce::File (samplePath).getFileNameWithoutExtension();

    logToFile ("AdgParser: mapped note " + juce::String (midiNote) + " -> " + mapping.sampleName
         + " (exists: " + juce::String (juce::File (samplePath).existsAsFile() ? "YES" : "NO") + ")");

    mappings.push_back (mapping);
}

juce::String AdgParser::findSamplePath (juce::XmlElement* element) const
{
    if (element == nullptr)
        return {};

    // Recursively find first SampleRef > FileRef > RelativePath
    std::function<juce::XmlElement*(juce::XmlElement*, const juce::String&)> findFirst;
    findFirst = [&findFirst] (juce::XmlElement* el, const juce::String& tagName) -> juce::XmlElement*
    {
        if (el == nullptr) return nullptr;
        if (el->getTagName() == tagName) return el;
        for (auto* child : el->getChildIterator())
        {
            auto* found = findFirst (child, tagName);
            if (found != nullptr) return found;
        }
        return nullptr;
    };

    auto* sampleRef = findFirst (element, "SampleRef");
    if (sampleRef == nullptr)
        return {};

    auto* fileRef = findFirst (sampleRef, "FileRef");
    if (fileRef == nullptr)
        return {};

    // Get RelativePathType (Ableton 12 uses type 5 for Core Library)
    auto* relPathType = fileRef->getChildByName ("RelativePathType");
    int pathType = 5;
    if (relPathType != nullptr)
        pathType = relPathType->getIntAttribute ("Value", 5);

    // Get RelativePath
    juce::String rawPath;
    auto* relPath = fileRef->getChildByName ("RelativePath");
    if (relPath != nullptr)
        rawPath = relPath->getStringAttribute ("Value");

    // Fallback: try Path element
    if (rawPath.isEmpty())
    {
        auto* pathEl = fileRef->getChildByName ("Path");
        if (pathEl != nullptr)
            rawPath = pathEl->getStringAttribute ("Value");
    }

    // Fallback: try Name element
    if (rawPath.isEmpty())
    {
        auto* nameEl = fileRef->getChildByName ("Name");
        if (nameEl != nullptr)
            rawPath = nameEl->getStringAttribute ("Value");
    }

    if (rawPath.isEmpty())
        return {};

    auto resolved = resolveRelativePath (rawPath, pathType);

    // If resolved path doesn't exist, try the absolute Path element as fallback
    if (! juce::File (resolved).existsAsFile())
    {
        auto* pathEl = fileRef->getChildByName ("Path");
        if (pathEl != nullptr)
        {
            juce::String absPath = pathEl->getStringAttribute ("Value");
            if (absPath.isNotEmpty() && juce::File (absPath).existsAsFile())
                return absPath;
        }
    }

    return resolved;
}

juce::String AdgParser::resolveRelativePath (const juce::String& relativePath, int pathType) const
{
    // Type 1 = External (absolute path)
    if (pathType == 1)
    {
        juce::File f (relativePath);
        if (f.existsAsFile())
            return f.getFullPathName();
        return relativePath;
    }

    // Types 2 and 5 = Library relative (resolve from Core Library path)
    if ((pathType == 2 || pathType == 5) && abletonLibraryPath.isDirectory())
    {
        juce::String cleaned = relativePath;

        if (cleaned.startsWith ("/"))
            cleaned = cleaned.substring (1);

        juce::File resolved = abletonLibraryPath.getChildFile (cleaned);
        if (resolved.existsAsFile())
            return resolved.getFullPathName();

        // Try Samples subfolder
        resolved = abletonLibraryPath.getChildFile ("Samples").getChildFile (cleaned);
        if (resolved.existsAsFile())
            return resolved.getFullPathName();

        return abletonLibraryPath.getChildFile (cleaned).getFullPathName();
    }

    // Type 6 = User Library relative (resolve from ~/Music/Ableton/User Library/)
    if (pathType == 6)
    {
        auto userLib = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                           .getChildFile ("Ableton/User Library");

        if (userLib.isDirectory())
        {
            juce::String cleaned = relativePath;
            if (cleaned.startsWith ("/"))
                cleaned = cleaned.substring (1);

            juce::File resolved = userLib.getChildFile (cleaned);
            if (resolved.existsAsFile())
                return resolved.getFullPathName();
        }
    }

    // Types 0 (Missing) and 3 (Current Project): return raw path
    return relativePath;
}
