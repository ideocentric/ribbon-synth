# CLion Hardware Workflow — absonus / Daisy Seed

How to build, flash, and debug the firmware from CLion. This replaces the Arduino
IDE's board/port/programmer menus.

**Board:** Electrosmith Daisy Seed · **MCU:** STM32H750IB (ARM Cortex-M7, *not* an
ESP chip). The chip is fixed by the CMake project (compile defines + the
`STM32H750IB_flash.lds` linker script) — there is **no board or chip to select** per
build, unlike the Arduino IDE. What you choose each time is: the **target**
(`absonus` or `sensor-test`), the **build type** (Release/Debug), and the **flash
method** (ST-Link or USB DFU).

---

## One-time setup

1. **Tools** (already installed on this machine): `arm-none-eabi-gcc`/`-gdb`, `cmake`,
   `openocd`, `dfu-util`. Verify with `arm-none-eabi-gcc --version`, `openocd --version`.

2. **Open the project.** Each firmware is its own standalone CMake project. In CLion:
   *File → Open →* select **`firmware/absonus`** (open `firmware/sensor-test` the same
   way as a second project/window when you need it).

3. **CMake profiles.** *Settings → Build, Execution, Deployment → CMake.* Add two
   profiles so you can switch quickly:
   | Profile | Build type | Use for |
   |:--|:--|:--|
   | Release | `Release` (`-O3`) | Normal use / testing sound. **Required for real-time audio** |
   | Debug   | `Debug` (`-Og -ggdb3`) | Stepping/breakpoints via ST-Link |
   > The cross-compiler is selected automatically — the project's `CMakeLists.txt`
   > pulls in libDaisy's toolchain before `project()`, so you do **not** pick a compiler
   > by hand. If CLion shows a host compiler (clang/gcc), see Troubleshooting.

4. **OpenOCD location.** *Settings → Build, Execution, Deployment → Embedded
   Development →* set **OpenOCD Location** to `/usr/local/bin/openocd` and click *Test*.

---

## Flashing — two methods (same choices you had in Arduino)

### A) ST-Link V3 over SWD — recommended (flash **and** debug)

Wire the ST-Link to the Daisy's SWD pads — **SWDIO, SWCLK, GND** (NRST optional) — the
same connection you used from the Arduino IDE. Refer to the Electrosmith Daisy Seed
pinout for pad locations.

Create a CLion run config:
- *Run → Edit Configurations… → + → **OpenOCD Download & Run***
- **Name:** `absonus (ST-Link)`
- **Target / Executable:** `absonus` / `absonus.elf`
- **Board config file:** `firmware/openocd/daisy.cfg`
- **Download:** *Always* · **Reset:** *After download*

Press **Run** to flash and start, or **Debug** to flash and break at `main`.
Command-line equivalent: `cmake --build build --target program`.

### B) USB DFU — no ST-Link needed

1. Put the Daisy in bootloader mode: **hold BOOT, tap RESET, release BOOT.**
2. Flash: build the **`program-dfu`** target in CLion, or run
   `cmake --build build --target program-dfu`.

---

## Debugging (ST-Link only)

Run the **OpenOCD Download & Run** config with the **Debug** profile selected. Set
breakpoints, inspect variables, step. Notes:
- Use the **Debug** build type for clean symbols.
- Under a Debug build the **audio may glitch** (the DSP isn't optimized) — that's
  expected; it's still fine for inspecting control-rate logic. Flash **Release** for
  actual playing.
- (VS Code users: `firmware/.vscode/launch.json` provides the equivalent.)

---

## The update loop — every time you change code

1. **Pick the target:** `absonus` (the synth) or `sensor-test` (input/solder check).
2. **Pick the build type:** *Release* for normal use; *Debug* only when debugging.
3. **Build** (Build → Build Project, or ⌘F9). Confirm it links and check the printed
   `FLASH:` usage stays under 128 KB.
4. **Connect hardware:** ST-Link wired to SWD, *or* USB cable for DFU.
5. **Flash:** run the ST-Link config, or enter DFU and run `program-dfu`.
6. **Verify:**
   - `sensor-test` → open the serial port and run the plotter (below); wiggle all 12
     inputs (8 pots, soft-pot, pressure, 2 switches) and confirm each responds.
   - `absonus` → play it; sweep filter, reverb, distortion, tremolo, etc.

### Serial verification (sensor-test)

From the repo root, using the project venv:
```bash
.venv/bin/python firmware/sensor-test/plot_sensors.py          # auto-detect port
.venv/bin/python firmware/sensor-test/plot_sensors.py --list   # if you must pick the port
```
The Arduino IDE Serial Plotter and Serial Studio also work on the same port.

---

## Quick command-line reference

```bash
cd firmware/absonus            # or firmware/sensor-test
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build   # build (Release)
cmake --build build --target program        # flash via ST-Link (OpenOCD)
cmake --build build --target program-dfu    # flash via USB DFU (enter DFU first)
```

---

## Troubleshooting

- **CLion uses a host compiler / can't find `arm-none-eabi-gcc`:** ensure it's on PATH,
  then *Tools → CMake → Reset Cache and Reload Project*. If it persists, set the CMake
  profile's toolchain file to
  `firmware/libDaisy/cmake/toolchains/ArmGNUToolchain.cmake`.
- **OpenOCD "no device found" / can't connect:** check the ST-Link USB and the SWD
  wiring; an ST-Link V3 may need the newer transport — uncomment
  `transport select dapdirect_swd` in `firmware/openocd/daisy.cfg`.
- **DFU: "No DFU capable USB device":** you didn't enter the bootloader — hold BOOT,
  tap RESET, release BOOT, then flash.
- **`FLASH:` over 100% / link fails on size:** make sure you built **Release**. (We're
  at ~81% now; if it ever overflows we'd switch to QSPI boot — not needed today.)
- **Audio is glitchy/distorted:** you flashed a **Debug** build — reflash **Release**.