# absonus — Ribbon Synthesizer

A touch-controlled FM ribbon synthesizer built on the [Electrosmith Daisy Seed](https://electro-smith.com/products/daisy-seed) platform. Pitch is controlled by a 150mm soft-pot ribbon sensor; expression by a force-sensitive resistor. A panel of knobs and switches shapes the FM synthesis, distortion, filter, tremolo, and reverb.

## Repository Structure

```
absonus/
├── firmware/
│   ├── absonus/          Main firmware (C++ / CMake, libDaisy + DaisySP)
│   ├── sensor-test/      Utility firmware — reads all panel inputs over serial
│   ├── libDaisy/         Daisy hardware library (git submodule)
│   └── DaisySP/          DSP library (git submodule)
│
├── hardware/
│   ├── enclosure/        FreeCAD / DXF / STL files for the enclosure and drill templates
│   ├── pcb/
│   │   ├── absonus-v0.2/ KiCad schematic and PCB layout (current)
│   │   ├── battery-clamp/ 3D-printable clamp for the 18650 battery holder
│   │   └── library/      KiCad component libraries (symbols + footprints)
│   └── testing-panel/    Laser-cut test panel drawings
│
├── bom/
│   ├── pcb-assembly-bom.csv    JLCPCB assembly BOM for the main PCB
│   └── sourced-components.md   Hand-sourced components (pots, sensors, enclosure, etc.)
│
├── docs/
│   ├── design-notes.md         Hardware layout and component design notes
│   ├── firmware-architecture.md Signal chain, module reference, pin mapping
│   └── datasheets/             Component datasheets (FSR, SoftPot, LS18-P, etc.)
│
├── assets/               Photos, renders (to be added)
│
└── archive/
    ├── firmware/         Earlier prototype sketches (fm, fm-chorus, tremor, sensor-test)
    └── hardware/         Earlier PCB revisions (pcb-v0.1, lipo-charger-booster, power-supply)
```

## Documentation

- [Firmware Architecture](docs/firmware-architecture.md) — signal chain, DSP modules, pin mapping, portamento times
- [Design Notes](docs/design-notes.md) — panel layout, Daisy Seed pin assignments, component selection notes
- [Sourced Components BOM](bom/sourced-components.md) — hand-sourced parts with supplier links
- [PCB Assembly BOM](bom/pcb-assembly-bom.csv) — JLCPCB SMT assembly BOM for the main board

## Build

Native C++ built with CMake against libDaisy + DaisySP, which are included as git submodules:

```bash
git submodule update --init --recursive        # first time only
cd firmware/absonus
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
cmake --build build --target program-dfu       # hold BOOT, tap RESET, then flash
```

See [`firmware/README.md`](firmware/README.md) for prerequisites, the ST-Link flashing path, and the serial plotter; [`docs/clion-hardware-workflow.md`](docs/clion-hardware-workflow.md) walks through build/flash/debug in CLion.

## Hardware Verification

Before troubleshooting from audio output, flash `firmware/sensor-test/` to the Daisy Seed immediately after assembly. It reads every pot, switch, force sensor, and soft-pot and streams their normalized values over serial (100 ms interval). Plot them with `firmware/sensor-test/plot_sensors.py` (or any serial monitor) to confirm each input responds correctly and is wired to the right pin before loading the main firmware.

## Acknowledgments

Portions of the firmware refactor, documentation, and repository organization were developed with the assistance of [Claude Code](https://claude.com/claude-code) (Anthropic).