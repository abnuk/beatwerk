#include "MidiMapper.h"

const std::array<PadInfo, 24> MidiMapper::pads = {{
    { 36, "Kick",       "Head"      },
    { 38, "Snare",      "Head"      },
    { 37, "Snare",      "X-Stick"   },
    { 40, "Snare",      "Rim"       },
    { 48, "Tom1",       "Head"      },
    { 50, "Tom1",       "Rim"       },
    { 45, "Tom2",       "Head"      },
    { 47, "Tom2",       "Rim"       },
    { 43, "Tom3",       "Head"      },
    { 58, "Tom3",       "Rim"       },
    { 42, "Hi-Hat",     "Closed"    },
    { 46, "Hi-Hat",     "Open"      },
    { 23, "Hi-Hat",     "Half Open" },
    { 44, "HH Pedal",   "Chick"     },
    { 21, "HH Pedal",   "Splash"    },
    { 49, "Crash1",     "Bow"       },
    { 55, "Crash1",     "Edge"      },
    { 57, "Crash2",     "Bow"       },
    { 52, "Crash2",     "Edge"      },
    { 51, "Ride",       "Bow"       },
    { 53, "Ride",       "Bell"      },
    { 59, "Ride",       "Edge"      },
    { 41, "EXT",        "Head"      },
    { 39, "EXT",        "Rim"       }
}};

MidiMapper::MidiMapper() {}

bool MidiMapper::isPadNote (int midiNote) const
{
    for (auto& p : pads)
        if (p.midiNote == midiNote)
            return true;
    return false;
}

const PadInfo* MidiMapper::getPadInfo (int midiNote) const
{
    for (auto& p : pads)
        if (p.midiNote == midiNote)
            return &p;
    return nullptr;
}

const std::array<PadInfo, 24>& MidiMapper::getAllPads()
{
    return pads;
}

void MidiMapper::setNavChannel (int channel)
{
    navChannel = channel;
}

void MidiMapper::setPrevCCNumber (int cc)
{
    prevCCNumber = cc;
}

void MidiMapper::setNextCCNumber (int cc)
{
    nextCCNumber = cc;
}

MidiMapper::NavAction MidiMapper::processForNavigation (const juce::MidiMessage& msg) const
{
    if (! msg.isController())
        return NavAction::None;

    if (navChannel > 0 && msg.getChannel() != navChannel)
        return NavAction::None;

    int cc = msg.getControllerNumber();
    int value = msg.getControllerValue();

    if (value == 0)
        return NavAction::None;

    if (cc == prevCCNumber)
        return NavAction::Previous;

    if (cc == nextCCNumber)
        return NavAction::Next;

    return NavAction::None;
}

bool MidiMapper::isDrumTrigger (const juce::MidiMessage& msg) const
{
    if (! msg.isNoteOn())
        return false;
    return isPadNote (msg.getNoteNumber());
}
