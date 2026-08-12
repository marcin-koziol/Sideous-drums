# patches/

`dpf-midi-note-names.patch` adds a small `Plugin::getMidiNoteName(uint8_t note, String& name)`
hook to the vendored `dpf/` submodule, wired into:

- VST3's `IUnitInfo` program-pitch-names interface (`DistrhoPluginVST3.cpp`)
- Ardour's LV2 MIDNAM extension, `http://ardour.org/lv2/midnam` (`DistrhoPluginLV2.cpp` +
  `DistrhoPluginLV2export.cpp` for the TTL `lv2:extensionData` declaration)

This is what lets `SideousDrums`/`SideousDrumsMulti` report fixed note names ("Kick", "Snare",
...) to hosts, so e.g. Ardour's piano roll shows names instead of note numbers.

The root `CMakeLists.txt` applies this patch automatically (and idempotently - it checks for
`getMidiNoteName` in `dpf/distrho/DistrhoPlugin.hpp` first) right before `add_subdirectory(dpf)`,
so a fresh `git submodule update --init --recursive` just works.

## If DPF gets updated

`git submodule update` to a newer DPF commit may cause the patch to no longer apply cleanly
(context lines shifted/changed upstream). If so:

1. `cd dpf && git apply ../patches/dpf-midi-note-names.patch` will fail with a rejection.
2. Re-apply the same changes by hand against the new DPF version (the patch is small and the
   diff hunks describe exactly what to change - see the summary above for *why* each hunk
   exists), or `git apply --reject` and resolve the `.rej` files.
3. Regenerate the patch: `cd dpf && git diff > ../patches/dpf-midi-note-names.patch`.
