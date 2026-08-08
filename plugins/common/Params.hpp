/*
 * Sideous Drums - shared parameter definitions used by both the DSP plugin
 * side and the UI, so ranges/units/labels can't drift between the two.
 */

#pragma once

#include <cstdint>

namespace sideous {

enum Params : uint32_t {
    kParamMasterVolume = 0,
    kParamMasterDrive,
    kParamMasterDriveMix,

    kParamKickTune,
    kParamKickPunchDepth,
    kParamKickPunchDecay,
    kParamKickDecay,
    kParamKickDrive,
    kParamKickLevel,

    kParamSnareTune,
    kParamSnareToneMix,
    kParamSnareToneDecay,
    kParamSnareSnap,
    kParamSnareBright,
    kParamSnareLevel,

    kParamHatClosedDecay,
    kParamHatClosedTone,
    kParamHatClosedMetal,
    kParamHatClosedLevel,

    kParamHatOpenDecay,
    kParamHatOpenTone,
    kParamHatOpenMetal,
    kParamHatOpenLevel,

    kParamTomLowTune,
    kParamTomLowDecay,
    kParamTomLowLevel,

    kParamTomMidTune,
    kParamTomMidDecay,
    kParamTomMidLevel,

    kParamTomHighTune,
    kParamTomHighDecay,
    kParamTomHighLevel,

    kParamCrashDecay,
    kParamCrashTone,
    kParamCrashLevel,

    kParamRideDecay,
    kParamRideTone,
    kParamRideLevel,

    kParamRimTune,
    kParamRimLevel,

    kParamClapDecay,
    kParamClapLevel,

    kParamMaracasDecay,
    kParamMaracasLevel,

    kParamCount
};

enum class ParamShape { Linear, Logarithmic };

struct ParamInfo
{
    const char* name;
    const char* symbol;
    const char* unit;
    float min;
    float max;
    float def;
    ParamShape shape;
};

inline const ParamInfo& getParamInfo(uint32_t index) noexcept
{
    static constexpr ParamInfo table[kParamCount] = {
        { "Master Volume",   "master_volume",     "",   0.0f,    1.0f,    0.8f,  ParamShape::Linear },
        { "Master Drive",    "master_drive",      "",   0.0f,    1.0f,    0.0f,  ParamShape::Linear },
        { "Master Drive Mix","master_drive_mix",  "",   0.0f,    1.0f,    1.0f,  ParamShape::Linear },

        { "Kick Tune",        "kick_tune",         "Hz", 30.0f,  150.0f,  55.0f,  ParamShape::Logarithmic },
        { "Kick Punch Depth", "kick_punch_depth",  "st", 0.0f,   48.0f,   24.0f,  ParamShape::Linear },
        { "Kick Punch Decay", "kick_punch_decay",  "s",  0.01f,  0.3f,    0.04f,  ParamShape::Logarithmic },
        { "Kick Decay",       "kick_decay",        "s",  0.05f,  2.0f,    0.35f,  ParamShape::Logarithmic },
        { "Kick Drive",       "kick_drive",        "",   0.0f,   1.0f,    0.2f,   ParamShape::Linear },
        { "Kick Level",       "kick_level",        "",   0.0f,   1.0f,    0.95f,  ParamShape::Linear },

        { "Snare Tune",       "snare_tune",        "Hz", 100.0f, 400.0f,  180.0f, ParamShape::Logarithmic },
        { "Snare Tone Mix",   "snare_tone_mix",    "",   0.0f,   1.0f,    0.5f,   ParamShape::Linear },
        { "Snare Tone Decay", "snare_tone_decay",  "s",  0.02f,  0.5f,    0.12f,  ParamShape::Logarithmic },
        { "Snare Snap",       "snare_snap",        "s",  0.02f,  0.5f,    0.15f,  ParamShape::Logarithmic },
        { "Snare Bright",     "snare_bright",      "Hz", 500.0f, 8000.0f, 2500.0f, ParamShape::Logarithmic },
        { "Snare Level",      "snare_level",       "",   0.0f,   1.0f,    0.85f,  ParamShape::Linear },

        { "Hat Closed Decay", "hat_closed_decay",  "s",  0.02f,  0.3f,    0.08f,  ParamShape::Logarithmic },
        { "Hat Closed Tone",  "hat_closed_tone",   "",   0.0f,   1.0f,    0.55f,  ParamShape::Linear },
        { "Hat Closed Metal", "hat_closed_metal",  "",   0.0f,   1.0f,    0.5f,   ParamShape::Linear },
        { "Hat Closed Level", "hat_closed_level",  "",   0.0f,   1.0f,    0.75f,  ParamShape::Linear },

        { "Hat Open Decay",   "hat_open_decay",    "s",  0.1f,   1.5f,    0.5f,   ParamShape::Logarithmic },
        { "Hat Open Tone",    "hat_open_tone",     "",   0.0f,   1.0f,    0.55f,  ParamShape::Linear },
        { "Hat Open Metal",   "hat_open_metal",    "",   0.0f,   1.0f,    0.5f,   ParamShape::Linear },
        { "Hat Open Level",   "hat_open_level",    "",   0.0f,   1.0f,    0.75f,  ParamShape::Linear },

        { "Tom Low Tune",     "tom_low_tune",      "Hz", 60.0f,  200.0f,  90.0f,  ParamShape::Logarithmic },
        { "Tom Low Decay",    "tom_low_decay",     "s",  0.05f,  1.0f,    0.30f,  ParamShape::Logarithmic },
        { "Tom Low Level",    "tom_low_level",     "",   0.0f,   1.0f,    0.85f,  ParamShape::Linear },

        { "Tom Mid Tune",     "tom_mid_tune",      "Hz", 90.0f,  300.0f,  140.0f, ParamShape::Logarithmic },
        { "Tom Mid Decay",    "tom_mid_decay",     "s",  0.05f,  1.0f,    0.28f,  ParamShape::Logarithmic },
        { "Tom Mid Level",    "tom_mid_level",     "",   0.0f,   1.0f,    0.85f,  ParamShape::Linear },

        { "Tom High Tune",    "tom_high_tune",     "Hz", 140.0f, 450.0f,  220.0f, ParamShape::Logarithmic },
        { "Tom High Decay",   "tom_high_decay",    "s",  0.05f,  1.0f,    0.25f,  ParamShape::Logarithmic },
        { "Tom High Level",   "tom_high_level",    "",   0.0f,   1.0f,    0.85f,  ParamShape::Linear },

        { "Crash Decay",      "crash_decay",       "s",  0.3f,   3.0f,    1.2f,   ParamShape::Logarithmic },
        { "Crash Tone",       "crash_tone",        "",   0.0f,   1.0f,    0.6f,   ParamShape::Linear },
        { "Crash Level",      "crash_level",       "",   0.0f,   1.0f,    0.8f,   ParamShape::Linear },

        { "Ride Decay",       "ride_decay",        "s",  0.3f,   3.0f,    1.5f,   ParamShape::Logarithmic },
        { "Ride Tone",        "ride_tone",         "",   0.0f,   1.0f,    0.5f,   ParamShape::Linear },
        { "Ride Level",       "ride_level",        "",   0.0f,   1.0f,    0.75f,  ParamShape::Linear },

        { "Rimshot Tune",     "rim_tune",          "Hz", 200.0f, 800.0f,  400.0f, ParamShape::Logarithmic },
        { "Rimshot Level",    "rim_level",         "",   0.0f,   1.0f,    0.8f,   ParamShape::Linear },

        { "Clap Decay",       "clap_decay",        "s",  0.05f,  0.6f,    0.15f,  ParamShape::Logarithmic },
        { "Clap Level",       "clap_level",        "",   0.0f,   1.0f,    0.8f,   ParamShape::Linear },

        { "Maracas Decay",    "maracas_decay",     "s",  0.02f,  0.3f,    0.08f,  ParamShape::Logarithmic },
        { "Maracas Level",    "maracas_level",     "",   0.0f,   1.0f,    0.7f,   ParamShape::Linear },
    };
    return table[index];
}

} // namespace sideous
