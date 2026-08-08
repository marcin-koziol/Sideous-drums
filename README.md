# Sideous Drums

![build](https://github.com/marcin-koziol/Sideous-drums/actions/workflows/build.yml/badge.svg)

> **⚠️ VIBE CODED SLOP.** This entire plugin — DSP, GUI, CI — was built through
> conversational back-and-forth with an AI, not hand-engineered from a spec.
> It works, it's been tested, but go in with appropriate expectations.

A retro 8-bit-vibe drum synth plugin (VST3 / LV2 / CLAP / JACK standalone),
built on [DPF](https://github.com/DISTRHO/DPF) with a hand-drawn Cairo-based
UI — a companion to [Sideous](https://github.com/marcin-koziol/Sideous-synth),
same visual family, warmer drum-machine-chassis palette. RD-9-style layout:
one column per drum, each with its own knobs and accent color.

![screenshot](docs/screenshot.png)

## Kit

Fixed 12-voice kit, each voice independently tunable, fixed GM-ish MIDI note
map (kick=36, snare=38, closed hat=42, open hat=46, low/mid/high tom=41/45/48,
crash=49, ride=51, rimshot=37, clap=39, maracas=70):

- **Kick** — sine osc with its own pitch envelope (Punch Depth/Decay, the
  classic "pitch sweep" trick), amp decay, drive/saturation for punch
- **Snare** — pitch-enveloped tone layer + independently-decaying highpassed
  noise layer, with a Bright knob so the noise layer can open up into real
  top-end instead of staying capped in a dull mid-range peak
- **Hi-hat closed/open** — 6-oscillator metallic FM stack (classic 808/909
  trick) blendable against plain filtered noise via a Metal knob (1 = full
  buzz, 0 = the simpler "chip" alternative); closed/open form a choke group,
  a closed-hat hit always cuts a still-ringing open hat
- **Toms** (low/mid/high) — same engine as the kick, different pitch ranges,
  with a fixed (non-exposed) pitch envelope baked in for character
- **Crash / Ride** — the same metallic FM engine as the hi-hats, always fully
  metallic, different decay range/defaults and brightness sweep
- **Rimshot** — short sine tock + short filtered noise tick, both fixed-decay
- **Clap** — three fast retriggered noise bursts ("flams") + one longer body
  pulse, the classic 808/909 handclap trick
- **Maracas** — highpass-filtered noise burst, decay + level only
- **Master** — Volume, plus a Drive/Mix saturation stage as a character/grit
  control, not just a safety limiter

## Two builds

- **Sideous Drums** — a single stereo output, the whole kit summed together.
- **Sideous Drums Multi** — 13 stereo busses (Main mix + one dedicated bus
  per voice), so e.g. the handclap can get its own reverb send in the host
  mixer instead of only ever reaching the summed Main bus. Same engine, same
  UI, identical sound on the Main bus — just more routing options.

## Building

```sh
git clone --recursive <repo-url>
cd sideous-drums
cmake -S . -B build
cmake --build build -j$(nproc)
```

(If you already cloned without `--recursive`, run
`git submodule update --init --recursive` first.)

Built plugins land in `build/bin/`:
- `sideous-drums.vst3` / `.lv2` / `.clap` / `sideous-drums` (JACK/native
  standalone) — stereo build
- `sideous-drums-multi.vst3` / `.lv2` / `.clap` / `sideous-drums-multi` —
  multi-out build

CI ([`.github/workflows/build.yml`](.github/workflows/build.yml)) builds
Linux, Windows, and macOS (universal) packages on every push and attaches
them to GitHub Releases for tagged versions.
