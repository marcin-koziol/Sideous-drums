/*
 * Sideous Drums - a handful of factory example presets, seeded into the
 * user's preset directory (see ui/PresetStore.hpp) the first time the UI
 * ever runs with an empty preset library. Compiled in (rather than shipped
 * as separate data files) so they exist identically across every build/
 * install of the plugin regardless of platform or plugin format - no
 * bundle-path discovery needed, they just get written out as ordinary,
 * user-editable ".sdpreset" files on first run and behave exactly like any
 * preset the user saves themselves from that point on.
 */

#pragma once

#include "PresetStore.hpp"

#include <cstring>
#include <utility>
#include <vector>

namespace sideous {
namespace ui {

struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> overrides; // symbol -> value; everything else stays at that param's Params.hpp default
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        // Deep, clean 808-style kit: long sub-heavy kick with a slow pitch
        // sweep, no drive, softer hats
        { "Classic 808", {
            { "kick_tune", 45.0f }, { "kick_punch_depth", 30.0f }, { "kick_punch_decay", 0.06f },
            { "kick_decay", 0.6f }, { "kick_drive", 0.0f },
            { "snare_tune", 190.0f }, { "snare_tone_mix", 0.6f }, { "snare_bright", 1800.0f },
            { "hat_closed_decay", 0.06f }, { "hat_closed_metal", 0.7f },
            { "clap_decay", 0.2f },
        }},
        // Punchy driven kick, fully metallic hats, bright crash - the
        // classic 909-vibe four-on-the-floor kit
        { "909 Techno", {
            { "kick_tune", 60.0f }, { "kick_punch_depth", 36.0f }, { "kick_decay", 0.25f }, { "kick_drive", 0.5f },
            { "hat_closed_metal", 1.0f }, { "hat_closed_decay", 0.05f },
            { "hat_open_metal", 1.0f }, { "hat_open_decay", 0.35f },
            { "crash_tone", 0.8f }, { "master_drive", 0.15f }, { "master_drive_mix", 0.6f },
        }},
        // Short tight kick, snappy bright snare, fast clipped hats - built
        // for busy trap-style hi-hat rolls
        { "Trap Snap", {
            { "kick_tune", 50.0f }, { "kick_decay", 0.18f }, { "kick_punch_depth", 28.0f },
            { "snare_snap", 0.05f }, { "snare_bright", 4500.0f }, { "snare_tone_mix", 0.3f },
            { "hat_closed_decay", 0.03f }, { "hat_closed_metal", 0.6f },
        }},
        // Deep detuned toms with long decays, hats turned down toward the
        // plain-noise end of Metal for a soft dub-echo-friendly kit
        { "Dub Wobble Toms", {
            { "tom_low_tune", 65.0f }, { "tom_low_decay", 0.6f },
            { "tom_mid_tune", 100.0f }, { "tom_mid_decay", 0.55f },
            { "tom_high_tune", 160.0f }, { "tom_high_decay", 0.5f },
            { "hat_closed_metal", 0.2f }, { "hat_open_metal", 0.2f },
            { "master_drive", 0.3f }, { "master_drive_mix", 0.5f },
        }},
        // Everything bright and forward - snappy snare, bright cymbals,
        // a bit of kick drive for energy
        { "Bright Pop Kit", {
            { "snare_bright", 5000.0f }, { "snare_tone_mix", 0.55f },
            { "crash_tone", 0.75f }, { "ride_tone", 0.7f },
            { "kick_drive", 0.35f },
        }},
        // Metal knob all the way down (plain filtered noise, no FM buzz) on
        // both hats, heavy master grit - the "chip-simple" alternative kit
        { "Lo-Fi Chip", {
            { "hat_closed_metal", 0.0f }, { "hat_open_metal", 0.0f },
            { "snare_bright", 1200.0f },
            { "kick_drive", 0.5f },
            { "master_drive", 0.6f }, { "master_drive_mix", 0.8f },
        }},
        // Metal knob pinned at 1 everywhere it exists, heavy drive on kick
        // and master - the buzziest, most aggressive end of the kit's range
        { "Metal Overload", {
            { "hat_closed_metal", 1.0f }, { "hat_open_metal", 1.0f },
            { "kick_drive", 0.8f }, { "kick_punch_depth", 40.0f },
            { "crash_level", 0.95f },
            { "master_drive", 0.7f }, { "master_drive_mix", 1.0f },
        }},
        // Everything pulled back in level with longer, gentler decays and
        // low Metal - a quiet, brushed-sounding kit
        { "Soft Brushes", {
            { "kick_level", 0.6f }, { "kick_drive", 0.0f }, { "kick_decay", 0.5f },
            { "snare_level", 0.5f }, { "snare_tone_mix", 0.7f }, { "snare_bright", 1500.0f },
            { "hat_closed_metal", 0.15f }, { "hat_closed_level", 0.4f },
            { "maracas_level", 0.5f },
        }},
        // Full metallic hats/cymbals plus heavy master drive and a long,
        // loud clap - a harsh, industrial-leaning kit
        { "Industrial Noise", {
            { "hat_closed_metal", 1.0f }, { "hat_closed_tone", 0.9f },
            { "hat_open_metal", 1.0f },
            { "clap_decay", 0.4f }, { "clap_level", 0.9f },
            { "tom_low_tune", 55.0f }, { "tom_low_decay", 0.7f },
            { "master_drive", 0.85f }, { "master_drive_mix", 1.0f },
        }},
        // The kick's pitch-envelope trick pushed to its extreme: very low
        // base tune, deep/long pitch sweep, heavy drive - a showcase preset
        { "Deep Sub Growl", {
            { "kick_tune", 32.0f }, { "kick_punch_depth", 45.0f }, { "kick_punch_decay", 0.15f },
            { "kick_decay", 0.8f }, { "kick_drive", 0.6f },
        }},
    };
    return presets;
}

// writes every factory preset to disk via the normal savePreset() path (so
// they're indistinguishable from user-saved presets from that point on) -
// call once, only when the preset library is empty (first run).
inline void seedFactoryPresets()
{
    for (const FactoryPreset& fp : factoryPresets())
    {
        float values[kParamCount];
        for (uint32_t i = 0; i < kParamCount; ++i)
            values[i] = getParamInfo(i).def;

        for (const auto& kv : fp.overrides)
        {
            for (uint32_t i = 0; i < kParamCount; ++i)
            {
                if (std::strcmp(getParamInfo(i).symbol, kv.first) == 0)
                {
                    values[i] = kv.second;
                    break;
                }
            }
        }

        savePreset(fp.name, values);
    }
}

} // namespace ui
} // namespace sideous
