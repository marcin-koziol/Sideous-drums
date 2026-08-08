# sideous-drums — plan for a new project (sibling to `sideous/`)

Written after building `sideous` (SID/POKEY-vibe VST3/LV2 synth, DPF + custom Cairo UI).
This doc is for bootstrapping a **separate, new project directory** — don't build inside `sideous/`.

## 1. Reuse the same stack, on purpose

DPF + hand-drawn Cairo UI worked well for `sideous` and there's no reason to pick a different
approach for a drum synth — same plugin formats (VST3 + LV2), same "no external asset files"
philosophy, same shared-layout architecture. Concretely:

- `dpf` as a git submodule. **Remember**: `git submodule update --init --recursive` — DPF has
  its own nested `pugl` submodule (`dgl/src/pugl-upstream`) that a plain `git submodule add`
  does NOT pull in. Build fails with `pugl/pugl.h: No such file or directory` until you do this.
- CMake: root `CMakeLists.txt` → `add_subdirectory(dpf)` + `add_subdirectory(plugins/X)`.
  Plugin's own `CMakeLists.txt` uses `dpf_add_plugin(... TARGETS lv2 vst3 clap jack UI_TYPE cairo
  FILES_DSP ... FILES_UI ...)`.
- `DISTRHO_UI_USE_CAIRO 1` + `DISTRHO_UI_DEFAULT_WIDTH/HEIGHT` in `DistrhoPluginInfo.h`.

### Files worth copying wholesale from `sideous` (generic, not chip-synth-specific)
- `dsp/ADSR.hpp` — shapeable envelope (linear↔analog curve knob). Drums mostly want fast
  attack + short decay, but the curve shaping is still useful for a natural-feeling snap.
- `dsp/Sync.hpp` — tempo-division enum + `hzFromSyncDivision()`. Directly useful if the drum
  synth gets a step sequencer or synced LFO/retrigger.
- The Cairo drawing helpers in `ui/UIPainter.hpp`: `drawText()`, `centeredText()`, `roundedRect()`,
  the palette pattern (`kBg/kPanelBg/kPanelEdge/kTextMain/kTextDim` + per-section accent colors),
  `setFont()`. All generic, all reusable as-is.
- The `Dropdown` widget (closed box + open list, with the auto-flip-upward-if-near-bottom-of-window
  logic) — useful for anything with >4-ish options (e.g. per-drum sample/waveform pick, MIDI note
  assignment).

Keep the same "Sideous" visual identity (retro C64/Atari palette, monospace bold labels, flat
shapes) so it reads as a companion product, not a random unrelated plugin — same header/footer
style, same knob/selector drawing, just a new panel layout and a new accent-color set per section.

## 2. Architecture pattern that mattered most: shared layout+paint header

Put ALL UI layout and drawing logic in one header (`ui/UIPainter.hpp`) with:
- A `Layout` struct (vector of panels/knobs/selectors/dropdowns) built by `buildLayout(width, height)`
- A `paint(cairo_t*, Layout, PaintState)` function
- Pure `paramToNormalized`/`normalizedToParam` mapping functions

...shared between the **real DPF UI class** (which only does mouse hit-testing + calls `paint()`)
and a **standalone offline PNG-rendering tool** (`g++` a tiny `main()` against the same header,
render to PNG, view with an image-viewing tool). This was the single most valuable technique in
`sideous` — it means the UI can be *designed and visually verified without a display*, iterating
on layout/color/spacing purely by rendering PNGs and looking at them, before ever touching a real
windowing system. Set this up on day one, don't bolt it on later.

Also share the **option-rect math** between drawing and hit-testing
(`getSelectorOptionRect`/`getDropdownOptionRect` take the same inputs paint uses) — otherwise the
clickable area and the drawn area silently drift apart as the layout changes.

## 3. Gotchas hit while building `sideous` — avoid repeating these

- **`cairo_show_text()` leaves a dangling "current point".** If the next drawing call is
  `cairo_arc()`/`cairo_line_to()` without an explicit new subpath, Cairo draws a stray connecting
  line from wherever the text cursor ended up. Symptom: random diagonal lines crossing the whole
  UI. Fix: wrap every text draw in a `drawText()` helper that calls `cairo_new_path()` right after
  `cairo_show_text()`, and never call the raw Cairo text functions directly.
- **`ParameterEnumerationValues` defaults to owning its array.** The default constructor sets
  `deleteLater = true`; if you point `.values` at a `static` array (not `new[]`-allocated), you
  must explicitly set `.deleteLater = false` or DPF calls `delete[]` on static memory and crashes.
- **The UI window size is two independent numbers that must be kept in sync by hand**:
  `DISTRHO_UI_DEFAULT_WIDTH/HEIGHT` (compile-time macro) vs whatever `buildLayout()` actually
  produces at runtime. These drifted twice in `sideous` (once when UI was first added, once when
  a third panel row was added later), causing a knob to render clipped off the bottom of the
  window. **Always do the final visual check with the offline PNG tool using the exact macro
  values** — not some other convenient test resolution — right before calling UI work done.
- **`getTimePosition().bpm` doesn't exist.** It's `getTimePosition().bbt.valid &&
  .bbt.beatsPerMinute`, and needs `DISTRHO_PLUGIN_WANT_TIMEPOS 1`.
- **Cascading two resonant filter stages for a 24dB mode compounds gain quadratically** if both
  stages get the same resonance value — measured peaks of ±400 in testing. Only give the *first*
  stage the user's resonance; keep the second stage undamped/neutral, it just adds slope. (Only
  relevant if `sideous-drums` ends up with a resonant filter on a voice, e.g. a filtered-noise
  hi-hat/snare — worth remembering if any voice chains two filter stages.)
- **Don't eyeball tiny screenshot thumbnails to check "did X change."** Use
  `compare -metric AE before.png after.png diff.png` (ImageMagick) for an objective pixel-diff.
  Repeatedly misjudged knob rotation angles by eye at thumbnail size during `sideous` development;
  the diff tool settled it immediately.
- **When testing waveform edges (e.g. detecting a sawtooth reset for a frequency estimate),
  don't route through a lowpass filter** — it rounds the sharp edge you're trying to detect.
  Raising the sample rate does *not* help and can make it worse: a fixed-Hz filter cutoff becomes
  relatively lower vs. Nyquist at a higher sample rate, so it filters *more* aggressively, not
  less. Use a highpass at minimum cutoff instead if a test needs to preserve sharp transients.

## 4. Testing methodology (worked well, keep doing this)

1. **Pure-function/algorithm unit tests, compiled standalone** — `g++` directly on the relevant
   `dsp/*.hpp` headers with no DPF dependency, in the scratchpad directory. Anything that's a pure
   function (envelope curve shaping, filter stability at extreme settings, sequencer/arp pattern
   math) gets validated this way. For logic embedded inside the DPF-derived `Plugin` subclass that
   can't be instantiated standalone, hand-copy just that algorithm into the test file and validate
   it in isolation (done for the arpeggiator pattern sequencing in `sideous`).
2. **Offline Cairo→PNG rendering tool** for visual UI iteration, see §2.
3. **Live interactive testing**: `Xvfb` (virtual X server) + the built JACK/standalone binary +
   `xdotool` (mousemove/mousedown/mousemove.../mouseup, click) to simulate real drag/click
   sequences + ImageMagick `import -window <id>` for screenshots + `compare -metric AE` for
   pixel-diffing before/after. This is what actually proved mouse drag, dropdown open/close, and
   double-click-reset worked in the real plugin, not just in the offline renderer.

## 5. sideous-drums — feature ideas ("cooler, not just basic")

This is a menu, not a spec — pick what's interesting. A "basic" chip drum synth is a handful of
oscillator+noise voices with an amp envelope. Here's what would make it more than that:

**Core kit** (each voice independently tunable, all voices always available — no user-configurable
kit structure needed, keep it a bounded fixed set like SID had a bounded set of channels):
- Kick — sine/triangle osc **with a pitch envelope** (starts high, drops fast — this pitch sweep
  is *the* defining kick-drum trick and is probably the single highest-value "not just basic"
  feature to get right), amp envelope, optional clip/saturation for punch.
- Snare — tone layer (short pitch-enveloped osc burst, like a fast mini-kick) + independently
  mixed filtered-noise layer.
- Hi-hat closed/open — either metallic FM (classic 808/909 trick: 6 square oscillators at
  inharmonic ratios, filtered) or simpler filtered/resonant noise for a more "chip-simple" option;
  **choke group** so a closed-hat hit cuts a still-ringing open-hat.
- Clap — several fast retriggered noise bursts (comb-like) for the characteristic "flam."
- Tom(s) — same engine as kick, different pitch range; 2-3 of them (low/mid/high) is plenty.
- Cowbell/perc — 2-operator FM square tones (classic 808 cowbell = two square waves at a fixed
  frequency ratio).

**Differentiating features, roughly in priority order:**
1. **Per-voice pitch envelope**, separate from the amp envelope, with depth + decay controls.
   This is the biggest lever for character — most "basic" drum synths skip this and just have a
   static-pitch click.
2. **Bitcrusher / sample-rate reducer**, per-voice or master — genuinely on-theme for "chip," and
   cheap to implement (quantize + hold samples).
3. **Waveshaping/saturation stage for punch** — reuse the `tanh`-crossfade drive trick from
   `sideous`'s `Filter.hpp` (`in + drive*(tanh(in*gain)-in)`), it's simple and it works.
4. **A small internal step sequencer** (e.g. 16 steps × all voices, tempo-synced via the same
   `Sync.hpp` math as `sideous`'s LFO/arp) — turns this from "a set of drum sounds triggered over
   MIDI" into something groove-programmable without a DAW. Probably the biggest scope item on this
   list; consider it a v2 feature rather than launch scope.
5. **Noise color** (white/pink) where a voice uses noise.
6. **Per-voice pan + level mini-mixer.**
7. **Velocity → tone**, not just velocity → level (e.g. velocity also opens the pitch-env depth or
   a brightness/filter parameter) — makes it feel expressive/playable rather than static.
8. **GM-ish drum-map MIDI note assignment** (kick on C1, snare D1, hats F#1/A#1, etc.), ideally
   user-remappable per voice, so it drops into existing DAW drum patterns/piano-roll lanes without
   remapping everything by hand.
9. **Master drive/grit saturation** as a musical character knob (not just the safety limiter
   `sideous` has) — same `tanh` trick, framed as a "vibe" control instead of a safety net.

**UI idea**: a literal step-sequencer grid (8 or 16 step buttons per row) would be a genuinely new,
visually distinct widget compared to `sideous`'s knob-heavy panels, and a good excuse to extend
`UIPainter.hpp` with a `StepButton`/`StepGrid` widget using the same shared-layout approach.

## 6. Suggested project layout

```
sideous-drums/                     (new sibling dir to sideous/, NOT inside it)
  dpf/                              (git submodule — remember --init --recursive)
  plugins/SideousDrums/
    DistrhoPluginInfo.h
    Params.hpp                      (shared param table: name/symbol/unit/min/max/def/shape)
    SideousDrumsPlugin.cpp
    SideousDrumsUI.cpp               (mouse handling only; layout/paint lives in ui/)
    CMakeLists.txt
    dsp/
      ADSR.hpp                      (copy from sideous, works as-is)
      Sync.hpp                      (copy from sideous, works as-is)
      PitchEnvelope.hpp             (new — separate from ADSR, or ADSR reused with a "depth in
                                      semitones" wrapper)
      Noise.hpp                     (new — simple LCG/xorshift white noise + optional pink filter)
      KickVoice.hpp / SnareVoice.hpp / HiHatVoice.hpp / ... (or one parameterized DrumVoice.hpp
                                      if the voices end up structurally similar enough)
    ui/
      UIPainter.hpp                 (copy the drawing helpers + palette + Dropdown widget from
                                      sideous, add StepGrid widget, new layout for a drum-kit-style
                                      panel arrangement instead of the synth's OSC/FILTER/ENV rows)
  CMakeLists.txt
```

## 7. What to ask the user before/while building (things I'd otherwise guess)

- Fixed kit (kick/snare/hats/clap/toms/cowbell, always present) vs. user-assignable slots?
  (Recommend fixed — simpler, matches the "basic vibe but polished" philosophy from `sideous`.)
- Is the step sequencer in scope for v1, or triggered purely over MIDI to start? (Recommend MIDI-
  only for v1, sequencer as a clearly-separated v2 addition — it's the single biggest scope item.)
- Metallic FM hi-hats (more authentic, more DSP work) vs. filtered-noise hi-hats (simpler, still
  convincing for a "chip" aesthetic)?
