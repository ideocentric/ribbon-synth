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
- [ ] Commit Phase 0 as a single housekeeping commit.

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
- [ ] **DECISION — pin library versions.** Add submodules; decide whether to pin to a
  released tag or track `main`.
  ```bash
  cd firmware
  git submodule add https://github.com/electro-smith/libDaisy
  git submodule add https://github.com/electro-smith/DaisySP
  git -C libDaisy submodule update --init
  make -C libDaisy && make -C DaisySP
  ```
- [ ] Author `sensor-test/CMakeLists.txt` from the libDaisy cmake template.
- [ ] Port `sensor-test.ino` → `sensor-test.cpp` (small; proves ADC + serial logging).
- [ ] Build, flash via DFU, confirm every input reads correctly over serial.

### Phase 1b — Port the main synth
- [ ] Swap framework include in `fmchorus.h`, `cubicnl.h`, `tremor.h`:
  `#include "DaisyDuino.h"` → `#include "daisysp.h"` + `using namespace daisysp;`
- [ ] Port `absonus.ino` → `absonus.cpp`:
  - `setup()/loop()` → `int main()` (+ `while(1)` control loop)
  - `DAISY.init/begin` → `DaisySeed hw; hw.Configure(); hw.Init(); hw.StartAudio(cb)`
  - `analogRead` ×10 → `AdcChannelConfig[]` + `hw.adc.GetFloat(ch)` (already 0–1)
  - `digitalRead(D13/D14)` → two `daisy::Switch` (or GPIO inputs)
  - `map()` / `constrain()` → local helpers (`fmap` / `fclamp`)
  - `MyCallback` DSP body ports almost verbatim
- [ ] **DECISION — ADC pin map.** As each `AdcChannelConfig` is set up, verify the
  physical pin against `assets/absonus-v0.3-schematic.pdf`. Do **not** trust the old
  `A0…A9` labels or the current doc table (known stale). Settle the true map here.
- [ ] **DECISION — debug/serial.** Keep the `kDebug` serial block (as `hw.PrintLine`)
  or drop it.
- [ ] Build, flash, A/B the audio against the last Arduino build.

### Phase 1 build & flash reference
```bash
cmake -B build -S . && cmake --build build
# DFU: hold BOOT, tap RESET, then:
make program-dfu   # or a dfu-util CMake target
```

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