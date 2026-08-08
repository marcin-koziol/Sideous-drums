/*
 * Sideous Drums - GUI layout and Cairo painting, shared between the real
 * plugin UI and an offline PNG renderer used to check the look during
 * development (see sideous's own UIPainter.hpp for why this split matters).
 *
 * RD-9-style layout: one narrow column per drum voice, each with its own
 * accent color and a small vertical grid of knobs - unlike sideous's
 * knob-panel-*rows*, this is knob-panel-*columns*. Only one interactive
 * widget type is needed here (Knob) since every drum parameter is
 * continuous - no selectors/dropdowns in this kit.
 *
 * Same retro chiptune "family" as sideous (dark bg, bold monospace labels,
 * flat shapes, per-section accent colors) but a warmer drum-machine-chassis
 * palette instead of sideous's cool navy/purple synth palette.
 */

#pragma once

#include <cairo.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <vector>

#include "../Params.hpp"
#include "../DrumMap.hpp"

namespace sideous {
namespace ui {

// -----------------------------------------------------------------------------------------------------------
// palette

struct Color { double r, g, b; };

static constexpr Color kBg        { 0.055, 0.035, 0.032 };
static constexpr Color kPanelBg   { 0.145, 0.086, 0.070 };
static constexpr Color kPanelEdge { 0.55,  0.30,  0.16  };
static constexpr Color kTextMain  { 0.95,  0.88,  0.78  };
static constexpr Color kTextDim   { 0.58,  0.46,  0.40  };
static constexpr Color kKnobBody  { 0.10,  0.06,  0.05  };
static constexpr Color kKnobRing  { 0.32,  0.20,  0.15  };

// per-voice accents - each drum column gets its own color, like the RD-9's
// per-section panel colors, but a warmer overall hue family than sideous
static constexpr Color kKickAccent      { 1.00, 0.42, 0.18 };
static constexpr Color kSnareAccent     { 1.00, 0.82, 0.35 };
static constexpr Color kHatClosedAccent { 0.90, 0.78, 0.55 };
static constexpr Color kHatOpenAccent   { 0.70, 0.55, 0.35 };
static constexpr Color kTomLowAccent  { 0.25, 0.75, 0.62 };
static constexpr Color kTomMidAccent  { 0.40, 0.85, 0.50 };
static constexpr Color kTomHighAccent { 0.65, 0.92, 0.40 };
static constexpr Color kCrashAccent   { 0.45, 0.88, 0.92 };
static constexpr Color kRideAccent    { 0.40, 0.62, 0.95 };
static constexpr Color kRimAccent     { 0.95, 0.35, 0.65 };
static constexpr Color kClapAccent    { 0.95, 0.25, 0.30 };
static constexpr Color kMaracasAccent { 0.68, 0.48, 0.95 };
static constexpr Color kMasterAccent  { 0.92, 0.90, 0.85 };

inline void setColor(cairo_t* cr, Color c, double a = 1.0) noexcept
{
    cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

// -----------------------------------------------------------------------------------------------------------
// layout model

struct Knob
{
    uint32_t param;
    float cx, cy, radius;
    const char* label;
    Color accent;
};

struct PanelBox
{
    float x, y, w, h;
    const char* title;
    Color accent;
    int midiNote = -1; // -1 = no note caption drawn (the Master column)
};

struct Layout
{
    float width, height;
    std::vector<PanelBox> panels;
    std::vector<Knob> knobs;
};

// -----------------------------------------------------------------------------------------------------------
// layout builder

// lays out one column's knobs in a 2-wide grid, vertically centered in the
// column's content area so voices with fewer knobs (toms, rimshot, maracas)
// get RD-9-style breathing room instead of stretching to fill the column
inline void addColumnKnobs(std::vector<Knob>& knobs, const PanelBox& p, Color accent,
                            std::initializer_list<std::pair<uint32_t, const char*>> items)
{
    const size_t n = items.size();
    const size_t rows = (n + 1) / 2;
    const float radius = 22.0f;
    const float rowPitch = 84.0f;
    const float contentTop = p.y + 58.0f;
    const float contentBottom = p.y + p.h - 10.0f;
    const float blockH = (float)rows * rowPitch;
    const float extra = (contentBottom - contentTop) - blockH;
    const float startY = contentTop + (extra > 0.0f ? extra * 0.5f : 0.0f) + rowPitch * 0.5f;

    size_t i = 0;
    for (const auto& item : items)
    {
        const size_t row = i / 2;
        const bool lastAlone = (n % 2 == 1) && (i == n - 1);
        const float cx = lastAlone       ? p.x + p.w * 0.5f
                        : (i % 2 == 0)    ? p.x + p.w * 0.30f
                        :                   p.x + p.w * 0.70f;
        const float cy = startY + (float)row * rowPitch;
        knobs.push_back({ item.first, cx, cy, radius, item.second, accent });
        ++i;
    }
}

struct ColumnSpec
{
    const char* title;
    int midiNote; // -1 = no caption (Master)
    Color accent;
    std::initializer_list<std::pair<uint32_t, const char*>> knobs;
};

// lays out one row of columns spanning the full width, each column's height
// fixed at rowH (tall enough for that row's own tallest knob-count column -
// see buildLayout's row1H/row2H) so both rows read as clean full-width strips
// even though they hold different column counts.
inline void layoutRow(Layout& L, const ColumnSpec* specs, int n, float margin, float gap,
                       float width, float rowY, float rowH)
{
    const float colW = (width - margin * 2.0f - gap * (float)(n - 1)) / (float)n;
    for (int c = 0; c < n; ++c)
    {
        const float x = margin + (colW + gap) * (float)c;
        L.panels.push_back({ x, rowY, colW, rowH, specs[c].title, specs[c].accent, specs[c].midiNote });
        addColumnKnobs(L.knobs, L.panels.back(), specs[c].accent, specs[c].knobs);
    }
}

inline Layout buildLayout(float width, float height)
{
    Layout L;
    L.width = width;
    L.height = height;

    const float margin = 16.0f;
    const float gap = 8.0f;

    // top row: the "hands-on" percussion (kick/snare/hats/rim/clap) - kick
    // and snare each need 3 knob-rows, so this row is the taller of the two
    const ColumnSpec row1[6] = {
        { "KICK", kDrumMidiNotes[kVoiceKick], kKickAccent,
          { { kParamKickTune, "TUNE" }, { kParamKickPunchDepth, "PUNCH" },
            { kParamKickPunchDecay, "P-DEC" }, { kParamKickDecay, "DECAY" },
            { kParamKickDrive, "DRIVE" }, { kParamKickLevel, "LEVEL" } } },
        { "SNARE", kDrumMidiNotes[kVoiceSnare], kSnareAccent,
          { { kParamSnareTune, "TUNE" }, { kParamSnareToneMix, "TONE" },
            { kParamSnareToneDecay, "T-DEC" }, { kParamSnareSnap, "SNAP" },
            { kParamSnareBright, "HIGH" }, { kParamSnareLevel, "LEVEL" } } },
        { "HAT CL", kDrumMidiNotes[kVoiceHatClosed], kHatClosedAccent,
          { { kParamHatClosedDecay, "DECAY" }, { kParamHatClosedTone, "TONE" },
            { kParamHatClosedLevel, "LEVEL" } } },
        { "HAT OP", kDrumMidiNotes[kVoiceHatOpen], kHatOpenAccent,
          { { kParamHatOpenDecay, "DECAY" }, { kParamHatOpenTone, "TONE" },
            { kParamHatOpenLevel, "LEVEL" } } },
        { "RIMSHOT", kDrumMidiNotes[kVoiceRim], kRimAccent,
          { { kParamRimTune, "TUNE" }, { kParamRimLevel, "LEVEL" } } },
        { "CLAP", kDrumMidiNotes[kVoiceClap], kClapAccent,
          { { kParamClapDecay, "DECAY" }, { kParamClapLevel, "LEVEL" } } },
    };

    // bottom row: toms/cymbals/perc/master - none need more than 2 knob-rows
    const ColumnSpec row2[7] = {
        { "TOM LO", kDrumMidiNotes[kVoiceTomLow], kTomLowAccent,
          { { kParamTomLowTune, "TUNE" }, { kParamTomLowDecay, "DECAY" }, { kParamTomLowLevel, "LEVEL" } } },
        { "TOM MID", kDrumMidiNotes[kVoiceTomMid], kTomMidAccent,
          { { kParamTomMidTune, "TUNE" }, { kParamTomMidDecay, "DECAY" }, { kParamTomMidLevel, "LEVEL" } } },
        { "TOM HI", kDrumMidiNotes[kVoiceTomHigh], kTomHighAccent,
          { { kParamTomHighTune, "TUNE" }, { kParamTomHighDecay, "DECAY" }, { kParamTomHighLevel, "LEVEL" } } },
        { "CRASH", kDrumMidiNotes[kVoiceCrash], kCrashAccent,
          { { kParamCrashDecay, "DECAY" }, { kParamCrashTone, "TONE" }, { kParamCrashLevel, "LEVEL" } } },
        { "RIDE", kDrumMidiNotes[kVoiceRide], kRideAccent,
          { { kParamRideDecay, "DECAY" }, { kParamRideTone, "TONE" }, { kParamRideLevel, "LEVEL" } } },
        { "MARACAS", kDrumMidiNotes[kVoiceMaracas], kMaracasAccent,
          { { kParamMaracasDecay, "DECAY" }, { kParamMaracasLevel, "LEVEL" } } },
        { "MASTER", -1, kMasterAccent,
          { { kParamMasterVolume, "VOL" }, { kParamMasterDrive, "DRIVE" }, { kParamMasterDriveMix, "MIX" } } },
    };

    const float row1Y = 64.0f;
    const float row1H = 320.0f; // fits 3 knob-rows (kick/snare's 6 knobs each)
    const float rowGap = 16.0f;
    const float row2Y = row1Y + row1H + rowGap;
    const float row2H = height - row2Y - margin;

    layoutRow(L, row1, 6, margin, gap, width, row1Y, row1H);
    layoutRow(L, row2, 7, margin, gap, width, row2Y, row2H);

    return L;
}

// -----------------------------------------------------------------------------------------------------------
// value <-> knob-rotation mapping (shared by drawing and mouse-drag hit logic)

inline float paramToNormalized(uint32_t param, float value) noexcept
{
    const ParamInfo& info = getParamInfo(param);
    float t;
    if (info.shape == ParamShape::Logarithmic)
    {
        const float lo = info.min > 0.0f ? info.min : 1e-6f;
        const float hi = info.max > lo ? info.max : lo * 1.0001f;
        const float v = value < lo ? lo : (value > hi ? hi : value);
        t = (float)(std::log(v / lo) / std::log(hi / lo));
    }
    else
    {
        t = (value - info.min) / (info.max - info.min);
    }
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

inline float normalizedToParam(uint32_t param, float t) noexcept
{
    const ParamInfo& info = getParamInfo(param);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    if (info.shape == ParamShape::Logarithmic)
    {
        const float lo = info.min > 0.0f ? info.min : 1e-6f;
        const float hi = info.max > lo ? info.max : lo * 1.0001f;
        return lo * std::pow(hi / lo, t);
    }
    return info.min + (info.max - info.min) * t;
}

// -----------------------------------------------------------------------------------------------------------
// value formatting

inline void formatParamValue(uint32_t index, float value, char* out, size_t outSize) noexcept
{
    const ParamInfo& info = getParamInfo(index);

    if (std::strcmp(info.unit, "Hz") == 0)
    {
        if (value >= 1000.0f)
            std::snprintf(out, outSize, "%.2fk", value / 1000.0);
        else
            std::snprintf(out, outSize, "%.0f", value);
    }
    else if (std::strcmp(info.unit, "s") == 0)
    {
        if (value < 1.0f)
            std::snprintf(out, outSize, "%.0fms", value * 1000.0);
        else
            std::snprintf(out, outSize, "%.2fs", value);
    }
    else if (std::strcmp(info.unit, "%") == 0)
    {
        std::snprintf(out, outSize, "%.0f%%", value);
    }
    else if (std::strcmp(info.unit, "st") == 0)
    {
        std::snprintf(out, outSize, "%.0fst", value);
    }
    else
    {
        std::snprintf(out, outSize, "%.2f", value);
    }
}

// -----------------------------------------------------------------------------------------------------------
// drawing helpers

inline void roundedRect(cairo_t* cr, double x, double y, double w, double h, double r)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -M_PI_2, 0.0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0.0,      M_PI_2);
    cairo_arc(cr, x + r,     y + h - r, r, M_PI_2,   M_PI);
    cairo_arc(cr, x + r,     y + r,     r, M_PI,     3.0 * M_PI_2);
    cairo_close_path(cr);
}

// cairo_show_text() leaves a dangling current point; if the next drawing call
// is cairo_arc()/cairo_line_to() without an explicit new subpath, cairo will
// implicitly connect from that stray point. Always reset the path after text.
inline void drawText(cairo_t* cr, const char* text, double x, double y)
{
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);
    cairo_new_path(cr);
}

inline void centeredText(cairo_t* cr, const char* text, double cx, double cy)
{
    cairo_text_extents_t ext;
    cairo_text_extents(cr, text, &ext);
    drawText(cr, text, cx - ext.width / 2.0 - ext.x_bearing, cy - ext.height / 2.0 - ext.y_bearing);
}

inline void setFont(cairo_t* cr, double size, bool bold = true)
{
    cairo_select_font_face(cr, "monospace",
                            CAIRO_FONT_SLANT_NORMAL,
                            bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
}

// -----------------------------------------------------------------------------------------------------------
// paint

struct PaintState
{
    const float* values = nullptr;   // [kParamCount]
    int hoverKnob = -1;               // index into layout.knobs, or -1
    int dragKnob = -1;
    const bool* autoShowValue = nullptr; // [kParamCount], true = briefly show this
                                          // knob's value even without dragging
                                          // (e.g. just changed via automation)
};

inline void paintKnob(cairo_t* cr, const Knob& k, float value, bool hovered, bool dragging, bool autoShow)
{
    const float t = paramToNormalized(k.param, value);

    const double startAngle = M_PI * 0.75;   // 135deg
    const double sweep = M_PI * 1.5;         // 270deg total
    const double valueAngle = startAngle + sweep * t;

    // outer ring track
    cairo_set_line_width(cr, 5.0);
    setColor(cr, kKnobRing);
    cairo_arc(cr, k.cx, k.cy, k.radius, startAngle, startAngle + sweep);
    cairo_stroke(cr);

    // value arc
    setColor(cr, k.accent, dragging ? 1.0 : (hovered ? 0.9 : 0.75));
    cairo_arc(cr, k.cx, k.cy, k.radius, startAngle, valueAngle);
    cairo_stroke(cr);

    // body
    setColor(cr, kKnobBody);
    cairo_arc(cr, k.cx, k.cy, k.radius - 6.0, 0.0, 2.0 * M_PI);
    cairo_fill(cr);

    if (hovered || dragging)
    {
        setColor(cr, k.accent, 0.18);
        cairo_arc(cr, k.cx, k.cy, k.radius - 6.0, 0.0, 2.0 * M_PI);
        cairo_fill(cr);
    }

    // pointer
    setColor(cr, k.accent);
    cairo_set_line_width(cr, 3.0);
    const double px = k.cx + std::cos(valueAngle) * (k.radius - 10.0);
    const double py = k.cy + std::sin(valueAngle) * (k.radius - 10.0);
    cairo_move_to(cr, k.cx, k.cy);
    cairo_line_to(cr, px, py);
    cairo_stroke(cr);

    // label
    setFont(cr, 9.5);
    setColor(cr, kTextDim);
    centeredText(cr, k.label, k.cx, k.cy + k.radius + 13.0);

    if (dragging || autoShow)
    {
        char buf[32];
        formatParamValue(k.param, value, buf, sizeof(buf));
        setFont(cr, 10.5);
        setColor(cr, k.accent);
        centeredText(cr, buf, k.cx, k.cy - k.radius - 11.0);
    }
}

inline void paint(cairo_t* cr, const Layout& L, const PaintState& state)
{
    // background
    setColor(cr, kBg);
    cairo_paint(cr);

    // header
    setFont(cr, 24.0);
    setColor(cr, kKickAccent);
    drawText(cr, "SIDEOUS DRUMS", 20.0, 38.0);

    setFont(cr, 10.0, false);
    setColor(cr, kTextDim);
    {
        const char* subtitle = "RETRO 8-BIT DRUM MACHINE";
        cairo_text_extents_t ext;
        cairo_text_extents(cr, subtitle, &ext);
        drawText(cr, subtitle, L.width - 20.0 - ext.width, 24.0);
    }

    setColor(cr, kPanelEdge, 0.6);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 16.0, 52.0);
    cairo_line_to(cr, L.width - 16.0, 52.0);
    cairo_stroke(cr);

    // columns
    for (const PanelBox& p : L.panels)
    {
        roundedRect(cr, p.x, p.y, p.w, p.h, 8.0);
        setColor(cr, kPanelBg);
        cairo_fill_preserve(cr);
        setColor(cr, p.accent, 0.7);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);

        if (p.title != nullptr)
        {
            setFont(cr, 11.5);
            setColor(cr, p.accent);
            centeredText(cr, p.title, p.x + p.w * 0.5, p.y + 19.0);
        }

        if (p.midiNote >= 0)
        {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "MIDI %d", p.midiNote);
            setFont(cr, 8.0, false);
            setColor(cr, kTextDim);
            centeredText(cr, buf, p.x + p.w * 0.5, p.y + 33.0);
        }
    }

    // knobs
    for (size_t i = 0; i < L.knobs.size(); ++i)
    {
        const Knob& k = L.knobs[i];
        const bool autoShow = state.autoShowValue != nullptr && state.autoShowValue[k.param];
        paintKnob(cr, k, state.values[k.param], (int)i == state.hoverKnob, (int)i == state.dragKnob, autoShow);
    }
}

} // namespace ui
} // namespace sideous
