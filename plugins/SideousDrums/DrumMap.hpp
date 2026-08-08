/*
 * Sideous Drums - fixed GM-ish MIDI note map, shared between the plugin
 * (which matches incoming note-on/off against these) and the UI (which
 * shows the note number in each column's caption so the mapping is at
 * least transparent even though it isn't user-remappable in v1).
 */

#pragma once

namespace sideous {

enum DrumVoiceIndex {
    kVoiceKick = 0,
    kVoiceSnare,
    kVoiceHatClosed,
    kVoiceHatOpen,
    kVoiceTomLow,
    kVoiceTomMid,
    kVoiceTomHigh,
    kVoiceCrash,
    kVoiceRide,
    kVoiceRim,
    kVoiceClap,
    kVoiceMaracas,
    kVoiceCount
};

// index-aligned with DrumVoiceIndex above
static constexpr int kDrumMidiNotes[kVoiceCount] = {
    36, // Kick       - C1
    38, // Snare      - D1
    42, // Hat Closed - F#1 (GM "Closed Hi-Hat")
    46, // Hat Open   - A#1 (GM "Open Hi-Hat")
    41, // Tom Low    - F1
    45, // Tom Mid    - A1
    48, // Tom High   - C2
    49, // Crash      - C#2
    51, // Ride       - D#2
    37, // Rimshot    - C#1
    39, // Clap       - GM "Hand Clap"
    70, // Maracas    - GM "Maracas"
};

} // namespace sideous
