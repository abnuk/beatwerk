# Beatwerk

A drum machine audio plugin for live performance. Import **Ableton Live Drum Racks** into a portable `.dkit` preset format, browse and assign samples with drag & drop, map pads to your hardware MIDI controller, and navigate presets via MIDI — all from a dark-themed UI designed for the stage.

Built with [JUCE](https://juce.com/) and C++20. Available as **VST3**, **Audio Unit**, and **Standalone** application.

## Features

### Ableton Live Import

- Import Ableton Drum Rack presets (`.adg`) into the custom `.dkit` format
- Samples are copied to a shared directory with preserved folder structure — no duplication across kits
- Progress bar and detailed import summary (imported / skipped / errors)
- Supports Core Library, User Library, and external sample references

### Custom .dkit Preset Format

- Portable JSON format with relative sample paths
- Stores name, author, description, source, creation date, and per-pad sample assignments
- Configurable samples and presets directories in Settings
- Missing sample indicator: red pad background with exclamation badge when a referenced file is not found

### Multi-Kit Electronic Drum Support

- Built-in library of 100 electronic drum kit definitions from 15 manufacturers (Roland, Yamaha, Alesis, Pearl, and more)
- Kit selector grouped by manufacturer in Settings
- Pad grid dynamically adapts to the selected kit's pad count and column layout
- Kit selection persists in plugin state with backward compatibility

### Pad Grid

- Dynamic pad layout driven by the selected electronic drum kit
- Each pad displays: name, trigger type (Head / Rim / X-Stick / Open / Closed / etc.), MIDI note, and loaded sample name
- Per-pad volume slider (0–200%) for boosting or cutting individual pad levels
- Velocity-sensitive triggering with visual flash animation
- Click a pad to preview the sample

### Sample Browser

- TreeView-based sidebar for browsing the samples directory
- Click to audition, drag onto any pad to assign
- Import samples from Finder with overwrite detection
- Search field for filtering samples by name
- Right-click context menu: delete, move to folder, reveal in Finder
- Locate button on pads to reveal the assigned sample in the browser

### Drag & Drop

- **External files** — drop `.wav`, `.aif`, `.aiff`, `.flac`, or `.mp3` onto any pad
- **Pad-to-pad swapping** — drag one pad onto another to swap their samples
- Custom mappings are saved per preset and persist across sessions

### Preset Browser

- Scrollable list with alphabet bar (A-Z, #) for quick navigation
- Active preset highlighted with accent color
- Click to load any preset instantly

### Preset Management

- Create new presets from the current kit ("+" button)
- Rename and delete presets via right-click context menu
- Pad mappings and volume settings are cleaned up automatically on delete

### MIDI Preset Navigation

- Navigate presets with MIDI CC messages from your controller
- Configurable MIDI channel (Any, or Ch 1-16)
- Configurable CC numbers for Previous / Next preset (default: CC#1 / CC#2)

### MIDI Learn

- Click **Learn** next to Prev or Next CC in Settings
- Send any CC from your controller — it's captured and assigned automatically

### Sample Engine

- Up to 8 polyphonic voices per pad (192 total)
- Voice stealing (oldest voice) when all voices are active
- Automatic resampling to match host sample rate
- Mono and stereo sample support

## Installation

### macOS Installer

Download `Beatwerk-1.1.0-macOS.pkg` from the [Releases](https://github.com/abnuk/beatwerk/releases) page and run it. You can choose which components to install:

| Component | Install Location |
|---|---|
| Standalone App | `/Applications/Beatwerk.app` |
| VST3 Plugin | `/Library/Audio/Plug-Ins/VST3/Beatwerk.vst3` |
| Audio Unit | `/Library/Audio/Plug-Ins/Components/Beatwerk.component` |

After installing, rescan plugins in your DAW if needed.

### Uninstalling

```bash
./installer/uninstall.sh
```

Or manually remove the files from the locations listed above.

## Building from Source

### Requirements

- macOS 11.0+ (Big Sur or later)
- CMake 3.22+
- C++20 compatible compiler (Xcode 13+)

### Build

```bash
git clone --recursive https://github.com/abnuk/beatwerk.git
cd beatwerk
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Build artefacts will be in `build/Beatwerk_artefacts/Release/`.

### Universal Binary (Apple Silicon + Intel)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
```

### Create macOS Installer

```bash
./installer/create_installer.sh            # uses existing build
./installer/create_installer.sh --build    # compiles first, then creates installer
```

To sign the installer for distribution:

```bash
./installer/create_installer.sh --sign "Developer ID Installer: Your Name (TEAM_ID)"
```

## Project Structure

```
beatwerk/
├── CMakeLists.txt              # Build configuration
├── Source/
│   ├── PluginProcessor.*       # Audio processing & state management
│   ├── PluginEditor.*          # Main UI, settings overlay
│   ├── SampleEngine.*          # Polyphonic sample playback
│   ├── MidiMapper.*            # Pad layout, MIDI routing, MIDI Learn
│   ├── DrumKitLibrary.*        # 100 electronic drum kit definitions
│   ├── AdgParser.*             # Ableton .adg file parser
│   ├── AbletonImporter.*       # .adg → .dkit import with sample copying
│   ├── PresetManager.*         # Preset scanning, loading, saving
│   ├── PadComponent.*          # Pad UI with drag & drop and volume
│   ├── PadMappingManager.*     # Per-preset custom pad mappings & volumes
│   ├── PresetListComponent.*   # Preset browser with alphabet nav
│   ├── SampleBrowserComponent.*# Sample browser with search & preview
│   └── LookAndFeel.*           # Dark theme styling
├── installer/
│   ├── create_installer.sh     # macOS .pkg builder
│   ├── uninstall.sh            # Uninstall helper
│   ├── distribution.xml        # Installer component definitions
│   └── resources/              # Installer UI (welcome, readme)
└── JUCE/                       # JUCE framework
```

## License

All rights reserved.
