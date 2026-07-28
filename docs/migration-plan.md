# Absonus Migration & Cleanup Plan

Working checklist for two efforts: (1) tidying the repository, and (2) porting the
firmware from the Arduino IDE (DaisyDuino) to a native C++ toolchain.

**Decisions locked in:**
- Firmware toolchain: **CMake**, driven from CLion / VS Code
- Dependencies: **libDaisy + DaisySP as git submodules**
- Generated hardware outputs: **keep `.stl` / `.step`, gitignore `.gcode`**

> Work through this top to bottom. Every step with a decision is called out — confirm
> the decision before acting on it.

---

## ▶ RESUME HERE (status as of 2026-07-27)

**Phases 1 and 2 are code-complete on branch `cpp-migration`** (pushed to `origin`),
kept off `main` until verified on real hardware. **Everything that remains needs the
hardware in hand.**

**Committed:** Phase 0 cleanup on `main` (`5b47291`, *not yet pushed* — `origin/main` is
one behind); Phase 1 + 2 on `cpp-migration` (`84738dd` submodules, `c9b7b25` sensor-test,
`25bfe98` absonus, `75035ca` VS Code tooling, `0094fba` CLion guide + OpenOCD config,
`5731fa5` `firmware/README.md`, `966164a` pin tables, plus this doc pass).

**Built & working (not yet flashed):**
- `firmware/sensor-test/` → C++ port + CMake, builds `.elf/.hex/.bin`.
- `firmware/absonus/` → C++ port + CMake, builds (FLASH 80.8%). Modernized past the
  DaisySP LGPL split: `LadderFilter` (res scaled 0–1→0–1.8), local `Port`, vendored
  `ReverbSc` (SDRAM).
- Submodules: libDaisy `v8.1.0`, DaisySP `599511b`.
- Dev tooling: `program`/`program-dfu` targets, `firmware/.vscode/` debug configs,
  `firmware/openocd/daisy.cfg`, `firmware/sensor-test/plot_sensors.py` + repo-root `.venv`.
- Docs: pin tables corrected against the v0.3 board; build instructions across
  `README.md`, `docs/firmware-architecture.md`, and `firmware/README.md` now describe the
  CMake/submodule flow.

**Next actions when you resume (on `cpp-migration`):**
1. **(hardware)** Flash `sensor-test` (`cd firmware/sensor-test && cmake -B build
   -DCMAKE_BUILD_TYPE=Release && cmake --build build && cmake --build build --target
   program`), plot with `plot_sensors.py`, confirm all 12 inputs.
2. **(hardware)** Flash `absonus`, A/B the audio vs. the last Arduino build. Listen
   especially for the `LadderFilter` resonance sweep and reverb tail (both are
   replacements, not the original modules).
3. If good: remove the now-obsolete `firmware/absonus/absonus.ino` +
   `firmware/sensor-test/sensor-test.ino` (still tracked; `archive/firmware/` preserves
   the Arduino-era lineage), commit.
4. **Merge `cpp-migration` → `main`**, then push `main` (it still carries the unpushed
   `5b47291`).

The branch is clean apart from the old `.ino` files awaiting step 3.

---

## Phase 0 — Repository cleanup

- [x] **Untrack LibreCAD autosave scratch files** (`#*.dxf`) accidentally committed.
  One of them (`#bud-cu-477-drill-patterns.dxf`) is the modified file in `git status`.
  ```bash
  git rm --cached "hardware/enclosure/#bud-cu-477-drill-patterns.dxf" \
                  "hardware/enclosure/#internal-drill-holes.dxf" \
                  "hardware/enclosure/#ribbon-panel-top.dxf" \
                  "hardware/enclosure/#ribbon-panel-v2.dxf"
  ```
- [x] **Remove the stray top-level `pcb/` directory** (only holds a `.DS_Store`; the real
  one is `hardware/pcb/`).
  ```bash
  rm -rf pcb/
  ```
- [x] **Extend `.gitignore`:**
  ```gitignore
  # LibreCAD / emacs-style autosave scratch files
  \#*
  # Slicer output (printer-specific: CE3V3SE = Ender 3 V3 SE)
  *.gcode
  # firmware build + toolchain artifacts (Phase 1)
  firmware/**/build/
  ```
- [x] **DECISION — Apple proprietary docs.** → **Moved to `archive/docs/`** via `git mv`.
- [x] **DECISION — untrack existing `.gcode`.** → **Untracked now** (`git rm --cached`,
  14 files; copies remain on disk, ignored going forward).
- [x] Commit Phase 0 as a single housekeeping commit. → `5b47291`

---

## Phase 1 — Firmware: Arduino → C++

### Target layout
```
firmware/
├── libDaisy/                 (git submodule)
├── DaisySP/                  (git submodule)
├── absonus/
│   ├── CMakeLists.txt
│   ├── absonus.cpp           (was absonus.ino)
│   ├── fmchorus.{h,cpp}
│   ├── cubicnl.{h,cpp}
│   └── tremor.{h,cpp}
└── sensor-test/
    ├── CMakeLists.txt
    └── sensor-test.cpp
```

### Phase 1a — Toolchain bring-up (validate on `sensor-test` first)
- [x] **DECISION — pin library versions.** → libDaisy pinned to **`v8.1.0`**, DaisySP
  pinned to main SHA **`599511b`** (2025-05-28). Submodules added under `firmware/`.
- [x] **DECISION — CMake layout.** → **Standalone per-firmware** (each firmware dir owns a
  `CMakeLists.txt` referencing `../libDaisy` / `../DaisySP`).
- [x] Author `sensor-test/CMakeLists.txt` from the libDaisy cmake template
  (`include(DaisyDefaultBuild)`; `-u _printf_float` enabled so `%f` works under
  newlib-nano; `program-dfu` custom target added).
- [x] Port `sensor-test.ino` → `sensor-test.cpp` (ADC via `AdcChannelConfig`, switches via
  `GPIO`, serial via `hw.PrintLine`).
- [x] Build succeeds — `sensor-test.elf/.hex/.bin` generated (~81 KB flash).
- [ ] **(hardware step)** Flash via DFU, confirm every input reads correctly over serial.
      ```bash
      cd firmware/sensor-test
      cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
      # put Daisy in DFU (hold BOOT, tap RESET), then:
      cmake --build build --target program-dfu
      ```

### Phase 1b — Port the main synth
- [x] Swap framework include in `fmchorus.h`, `cubicnl.h`, `tremor.h`:
  `#include "DaisyDuino.h"` → `#include "daisysp.h"` + `using namespace daisysp;`.
  `cubicnl.cpp` Arduino `min`/`max` → `fclamp`.
- [x] Port `absonus.ino` → `absonus.cpp` (faithful: `setup()`+`loop()` retained, called
  from `main()`; `analogRead`→`hw.adc.GetFloat`; `digitalRead`→`GPIO`; `map`/`constrain`
  → local `mapL`/`constrainL`; `Serial`→`hw.PrintLine`; callback ported verbatim).
- [x] **DECISION — DaisySP module gap.** Current DaisySP main removed `Port`,
  `MoogLadder`, `ReverbSc` (LGPL split, commit `cfcb239`). **Chosen: modernize** (not
  pin old / not add DaisySP-LGPL), using the DaisyDuino source as reference:
    - `MoogLadder` → current DaisySP **`LadderFilter`** (LP24). Res range now 0–1.8;
      firmware passes 0–1 (faithful, less extreme — rescale later if wanted).
    - `Port` → **reimplemented** locally (`port.{h,cpp}`, `idfk` namespace, one-pole,
      behavior-identical).
    - `ReverbSc` → **vendored** classic Sean Costello reverb (`reverbsc.{h,cpp}`, `idfk`
      namespace). ~395 KB buffer placed in SDRAM via `DSY_SDRAM_BSS`.
- [x] Builds clean: `absonus.elf/.hex/.bin` — FLASH 80.8% (105.9 KB/128 KB), SDRAM 0.59%.
- [ ] **(hardware step)** Flash via DFU; A/B the audio against the last Arduino build.
- [x] **DECISION — ADC pin map (for docs, Phase 2).** → **Settled and verified against the
  v0.3 board** in `966164a`: A0 distortion, A1 filterFreq, A2 noise, A3 volume, A4 reverb,
  A5 filterRes, A6 modDepth, A7 chorus, A8 force, A9 softpot. Docs now match the code.
- [ ] **(cleanup)** Remove `absonus.ino` + `sensor-test.ino` once both `.cpp` builds are
  hardware-verified.

### Phase 1 build & flash reference
```bash
cd firmware/<absonus|sensor-test>
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
# Flash — USB DFU (hold BOOT, tap RESET first):
cmake --build build --target program-dfu
# Flash — ST-Link V3 over SWD (no BOOT+RESET; also the debug path):
cmake --build build --target program
```

### Development tooling (added)
- [x] **ST-Link `program` target** in both CMakeLists (OpenOCD + `interface/stlink.cfg`
  + `target/stm32h7x.cfg`). `program-dfu` remains for USB-only flashing.
- [x] **VS Code live debugging** via `firmware/.vscode/launch.json` (+ `tasks.json`):
  Cortex-Debug over ST-Link for both firmwares. Open `firmware/` as the workspace.
  Requires the Cortex-Debug extension (OpenOCD + `arm-none-eabi-gdb` already present).
- [x] **Python serial plotter** `firmware/sensor-test/plot_sensors.py` (+ `requirements.txt`)
  — repo-local replacement for the Arduino Serial Plotter. Uses the project `.venv`:
  ```bash
  python3 -m venv .venv               # from repo root (already created)
  .venv/bin/pip install -r firmware/sensor-test/requirements.txt
  .venv/bin/python firmware/sensor-test/plot_sensors.py            # auto-detect port
  ```
  The Arduino IDE Serial Plotter and Serial Studio also still work on the same port.

### Resonance rescale
- [x] `filterRes` now scales the 0–1 knob to `LadderFilter`'s 0–1.8 range
  (`kFilterResMax`), so full-CW reaches self-oscillation like the old MoogLadder.

---

## Phase 2 — Documentation

- [x] **Fix the stale pin table** in `docs/firmware-architecture.md` and `design-notes.md`
  (`966164a`). Corrected against the v0.3 board (`absonus-v0.2.kicad_pcb/.kicad_sch`) and
  the firmware: 8 pots on A0–A7 via J10, FSR on A8 (J4), soft-pot on A9 (J8).
- [x] Update **Build** sections in `README.md` and `firmware-architecture.md` from
  "Arduino IDE + DaisyDuino" to the CMake / submodule flow. Also refreshed the repo-tree
  listing and the stale `MoogLadder` / `Port` module references (both were replaced in the
  C++ build) so the docs match the code.
- [x] Add `firmware/README.md` with submodule-init + build/flash commands (`5731fa5`).
- [x] Add `docs/clion-hardware-workflow.md` + `firmware/openocd/daisy.cfg` (`0094fba`) —
  not originally planned; added while setting up the hardware workflow.

---

## Notes
- `archive/firmware/` stays as-is — it documents the Arduino-era lineage.
- `firmware/sensor-test/.vscode/` already has Cortex-Debug state — a head start for 1a.