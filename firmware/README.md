# absonus firmware

Native C++ firmware for the Electrosmith Daisy Seed (STM32H750), built with CMake
against libDaisy + DaisySP. Two standalone projects:

| Project | What it is |
|:--|:--|
| [`absonus/`](absonus/) | The synth — FM engine, filter, distortion, tremolo, reverb |
| [`sensor-test/`](sensor-test/) | Reads all 12 panel inputs to serial; flash first to verify wiring/solder |

Dependencies live as git submodules: `libDaisy/` (v8.1.0), `DaisySP/`.

## Prerequisites

`arm-none-eabi-gcc`/`-gdb`, `cmake` (≥3.26), `dfu-util`, and `openocd` (for ST-Link).
On macOS: `brew install cmake dfu-util open-ocd` and the Arm GNU toolchain.

## Get the source (with submodules)

```bash
git clone git@personal-github:ideocentric/ribbon-synth.git
cd ribbon-synth
git submodule update --init --recursive        # pulls libDaisy + DaisySP (+ nested)
```
Already cloned without submodules? Just run the `submodule update` line.

## Build

Each firmware is its own CMake project. From `firmware/`:

```bash
cd absonus                 # or: cd sensor-test
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build        # first build also compiles libDaisy + DaisySP (a few min)
```
Output: `build/<name>.elf`, `.hex`, and `.bin`. Watch the `FLASH:` line — it must stay
under 128 KB. Use `-DCMAKE_BUILD_TYPE=Debug` only for debugging (Release is required for
glitch-free real-time audio).

## Flash

```bash
# ST-Link V3 over SWD (also the debug path):
cmake --build build --target program

# USB DFU — first put the Daisy in bootloader mode: hold BOOT, tap RESET, release BOOT
cmake --build build --target program-dfu
```

## Verify sensor-test

Flash `sensor-test`, then plot the inputs from the repo root (uses the project `.venv`):

```bash
python3 -m venv .venv                                            # first time only
.venv/bin/pip install -r firmware/sensor-test/requirements.txt   # first time only
.venv/bin/python firmware/sensor-test/plot_sensors.py            # auto-detect port
```
The Arduino IDE Serial Plotter and Serial Studio also work on the same port.

## More

- Full IDE walkthrough (build/flash/debug in CLion): [`../docs/clion-hardware-workflow.md`](../docs/clion-hardware-workflow.md)
- Signal chain, modules, pin mapping: [`../docs/firmware-architecture.md`](../docs/firmware-architecture.md)