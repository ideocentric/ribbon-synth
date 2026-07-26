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

## ▶ RESUME HERE (status as of 2026-07-26)

**Phase 1 work is committed on branch `cpp-migration`** (4 commits, all building),
kept off `main` until verified on real hardware. Next actions are hardware + merge.

**Committed:** Phase 0 cleanup on `main` (`5b47291`); Phase 1 on `cpp-migration`
(`84738dd` submodules, `c9b7b25` sensor-test, `25bfe98` absonus, `75035ca` tooling).
Work here on `cpp-migration`; merge to `main` after verification.

**Built & working (not yet flashed):**
- `firmware/sensor-test/` → C++ port + CMake, builds `.elf/.hex/.bin`.
- `firmware/absonus/` → C++ port + CMake, builds (FLASH 80.8%). Modernized past the
  DaisySP LGPL split: `LadderFilter` (res scaled 0–1→0–1.8), local `Port`, vendored
  `ReverbSc` (SDRAM).
- Submodules: libDaisy `v8.1.0`, DaisySP `599511b`.
- Dev tooling: `program`/`program-dfu` targets, `firmware/.vscode/` debug configs,
  `firmware/sensor-test/plot_sensors.py` + repo-root `.venv`.

**Next actions when you resume (on `cpp-migration`):**
1. Flash `sensor-test` (`cd firmware/sensor-test && cmake -B build && cmake --build build
   && cmake --build build --target program`), plot with `plot_sensors.py`, confirm all
   12 inputs. Then flash `absonus`, A/B the audio vs. the Arduino build.
2. If good: remove the now-obsolete `absonus.ino` + `sensor-test.ino` (commit), do
   **Phase 2 — Documentation** (fix stale pin table, update build instructions), then
   **merge `cpp-migration` → `main`**.

The old `.ino` files are still tracked (remove after hardware verification). Nothing
else is uncommitted — the branch is clean.

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
- [ ] **DECISION — ADC pin map (for docs, Phase 2).** Verify pin→function against
  `assets/absonus-v0.3-schematic.pdf`. Derived map (from `absonus.ino` + `sensor-test`,
  both self-consistent): A0 distortion, A1 filterFreq, A2 noise, A3 volume, A4 reverb,
  A5 filterRes, A6 modDepth, A7 chorus, A8 force, A9 softpot.
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

- [ ] **Fix the stale pin table** in `docs/firmware-architecture.md` (lines ~140–153).
  It contradicts the code (`absonus.ino:26-39`). Regenerate from the settled Phase 1b
  `AdcChannelConfig` table so code and docs share one source.
- [ ] Update **Build** sections in `README.md` and `firmware-architecture.md` from
  "Arduino IDE + DaisyDuino" to the CMake / submodule flow.
- [ ] Add `firmware/README.md` with submodule-init + build/flash commands.

---

## Notes
- `archive/firmware/` stays as-is — it documents the Arduino-era lineage.
- `firmware/sensor-test/.vscode/` already has Cortex-Debug state — a head start for 1a.