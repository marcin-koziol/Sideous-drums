#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND   "Sideous"
#define DISTRHO_PLUGIN_NAME    "Sideous Drums Multi"
#define DISTRHO_PLUGIN_URI     "https://github.com/sideous/sideous-drums#multi"
#define DISTRHO_PLUGIN_CLAP_ID "sideous.sideousdrumsmulti"

#define DISTRHO_PLUGIN_BRAND_ID  Side
#define DISTRHO_PLUGIN_UNIQUE_ID DrmM

#define DISTRHO_PLUGIN_HAS_UI           1
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_IS_SYNTH         1
#define DISTRHO_PLUGIN_NUM_INPUTS       0
// 13 stereo busses: Main mix + one per voice (Kick, Snare, Hat Closed, Hat
// Open, Tom Low/Mid/High, Crash, Ride, Rimshot, Clap, Maracas), so e.g. the
// clap can get its own reverb send in the host mixer instead of only ever
// reaching the summed Main bus. See SideousDrumsMultiPlugin.cpp.
#define DISTRHO_PLUGIN_NUM_OUTPUTS      26
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_STATE       0
#define DISTRHO_PLUGIN_WANT_PROGRAMS    0
#define DISTRHO_PLUGIN_WANT_TIMEPOS     0

#define DISTRHO_UI_USE_CAIRO             1
#define DISTRHO_UI_DEFAULT_WIDTH         1080
#define DISTRHO_UI_DEFAULT_HEIGHT        678
// same UI as the stereo build (ui/UIPainter.hpp doesn't care about output
// routing) - must match the height ui/UIPainter.hpp's buildLayout() actually
// lays out to (verified via the offline PNG renderer) - keep in sync by hand.

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
