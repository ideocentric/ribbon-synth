# Absonus Firmware Architecture

Firmware for the Absonus ribbon synthesizer. Written in C++ for the Electrosmith Daisy Seed against libDaisy and the DaisySP DSP library, built with CMake.

Source: `firmware/absonus/`

---

## Signal Chain

```
SoftPot (ribbon) ──► pitch (carrierFreq)
FSR (pressure)   ──► gain + gate
                                          ┌─────────┐
                          gate ──────────►│         │
                          gain ──────────►│ Envelope│──► envelopeOut × gain
                                          │  (ADSR) │
                                          └─────────┘
                                                │
                                                ▼
                               ┌─────────────────────────────┐
                  carrierFreq ►│                             │
                   modDepth   ►│       FmChorus              │──► sig (mono)
                   chorusWidth►│  (3-voice FM + detuning)    │
                               └─────────────────────────────┘
                                                │
                                                ▼
                               noiseLevel ──► + WhiteNoise (×0.5 × gain)
                                                │
                                                ▼
                             distortionLevel ──► Cubicnl (cubic waveshaper)
                                                │
                                                ▼
                   filterFreq ──► LadderFilter (LP24) ◄── filterRes
                                                │
                                                ▼
                                          ┌─────────┐
                  tremoloFreq  ──────────►│         │──► sig × trem (mono)
                  tremoloDepth ──────────►│  Tremor │──► sigl = sig × treml
                  tremoloWidth ──────────►│         │──► sigr = sig × tremr
                                          └─────────┘
                                                │
                                   sigl ────────┤
                                   sigr ────────┤
                                                ▼
                              reverbLevel ──► ReverbSc (SCReverb)
                                                │
                                                ▼
                                 CrossFade (dry/wet blend at reverbLevel)
                                                │
                              volumeLevel ──────┤
                                                ▼
                                     out[0] (L)   out[1] (R)
```

All control parameters are processed through a `Port` (slew/portamento) object before being applied. This prevents zipper noise and audible stepping from ADC readings.

---

## Modules

### FmChorus (`fmchorus.h/.cpp`)

Three-voice frequency modulation synthesizer with built-in chorus detuning.

Each voice is a carrier/modulator oscillator pair (6 oscillators total). The center voice runs at the target pitch. The upper and lower voices are detuned symmetrically by an amount proportional to `width`, up to one equal-tempered semitone. Detuned voices are output at 90% amplitude to preserve the center pitch as dominant.

**Fixed parameters in current patch:**
- Carrier waveform: sawtooth (`WAVE_SAW`)
- Modulator waveform: triangle (`WAVE_TRI`)
- Modulator/carrier ratio: √2 ≈ 1.4142 (equal-tempered tritone)

**Controllable parameters:**
| Parameter | Control | Range | Notes |
|:--|:--|:--|:--|
| Carrier frequency | SoftPot | 55–3520 Hz (A1–A7) | Exponential mapping via `pow(2, normalized × scalar)` |
| Modulation depth | Mod knob (A6\*) | 0.0–1.0 | Scales internally to 0–carrierFreq Hz |
| Chorus width | Chorus knob (A8\*) | 0.0–1.0 | 0 = unison, 1 = ±1 semitone |

*\*See pin mapping below for actual ADC assignments.*

---

### Cubicnl (`cubicnl.h/.cpp`)

Soft-clipping waveshaper using a cubic polynomial. Ported from the Faust `cubicnl.dsp` algorithm by Julius O. Smith III. A DaisySP `DcBlock` filter (pole at 0.999) is applied post-distortion.

| Parameter | Control | Range |
|:--|:--|:--|
| Drive | Distortion knob (A0) | 0.0–1.0 |
| Offset | Fixed | 0.0 (not exposed to panel) |

---

### Tremor (`tremor.h/.cpp`)

Stereo tremolo with panning inspired by the Leslie speaker cabinet. Three internal sine-wave LFOs: one for amplitude modulation and two for stereo panning running at half the tremolo frequency.

The panel uses two toggle switches rather than a continuous knob:

| Switch State | Rate | Depth | Width |
|:--|:--|:--|:--|
| Off (`tremoloOnPin` HIGH) | 0 Hz | 0 | 0 (bypassed) |
| Slow (`tremoloSpeedPin` HIGH) | 1.0 Hz | 0.50 | 0.66 |
| Fast (`tremoloSpeedPin` LOW) | 13.0 Hz | 0.75 | 0.83 |

Slow-to-fast rate transition uses a long portamento time (4 seconds) on `tremoloFreqPortamento` to ramp frequency gradually, mimicking a Leslie speed change.

---

### LadderFilter, ReverbSc, WhiteNoise

`LadderFilter` and `WhiteNoise` are stock DaisySP modules used without modification.
`ReverbSc` was removed from DaisySP in the LGPL split and is vendored into
`firmware/absonus/reverbsc.{h,cpp}` (`idfk` namespace); its ~395 KB buffer lives in
external SDRAM via `DSY_SDRAM_BSS`.

| Module | Control | Range | Notes |
|:--|:--|:--|:--|
| LadderFilter | Cutoff knob (A1\*), Res knob (A5\*) | 20–24000 Hz / 0.0–1.0 | LP24 mode; exponential frequency mapping. Replaces the removed `MoogLadder`; the 0–1 knob is scaled ×1.8 (`kFilterResMax`) to reach self-oscillation |
| ReverbSc | Reverb knob (A4\*) | 0.0–1.0 | LP cutoff fixed at 18 kHz; level controls feedback AND dry/wet crossfade |
| WhiteNoise | Noise knob (A2\*) | 0.0–1.0 | Amplitude scaled by ×0.5 × gain |

---

## Envelope and Gate

An ADSR envelope (DaisySP `Adsr`) shapes the amplitude output of `FmChorus`. Gate open/close is driven by the FSR force sensor: gate opens when the normalized FSR reading exceeds `kGateThreshhold` (0.1).

| Segment | Time |
|:--|:--|
| Attack | 100 ms |
| Decay | 100 ms |
| Sustain | 70% |
| Release | 10 ms |

**Frequency hold on gate-off:** When the gate closes, `carrierPortamento` continues slewing toward the *last played frequency* rather than falling back to the minimum. This allows a longer release tail without the pitch dropping to the bottom of the range.

---

## Control Surface Pin Mapping

Function and scaling are taken from the firmware (`absonus.cpp`). Physical routing is
confirmed against the v0.3 board netlist: the 8 pots arrive on connector **J10** (2×5
IDC ribbon), the FSR on **J4**, and the soft-pot on **J8**. Wire colors are the ribbon
harness colors used by `sensor-test` (verify on hardware with `sensor-test` before
trusting the color column — the crimp order is not captured in any file).

| Daisy Pin | Connector | Wire / control | Function (firmware) | Scaling |
|:--|:--|:--|:--|:--|
| A0 | J10-9 | White | Distortion drive | Linear 0–1 |
| A1 | J10-7 | Purple | Filter cutoff | Exponential 20–24000 Hz |
| A2 | J10-5 | Green | Noise level | Linear 0–1 |
| A3 | J10-3 | Orange | Volume | Linear 0–1 |
| A4 | J10-4 | Yellow | Reverb level | Linear 0–1 |
| A5 | J10-6 | Blue | Filter resonance | Linear 0–1, scaled ×1.8 into LadderFilter |
| A6 | J10-8 | Gray | FM modulation depth | Linear 0–1 |
| A7 | J10-10 | Black | Chorus width | Linear 0–1 |
| A8 | J4 | Pressure (FSR) | Gain + gate | Linear 0–1 |
| A9 | J8 | Soft-pot (ribbon) | Pitch (carrier freq) | Exponential 55–3520 Hz |
| D13 | via U1 (LS18-P) | SW2 | Tremolo speed (slow/fast) | Digital |
| D14 | via U1 (LS18-P) | SW1 | Tremolo on/off | Digital |

Daisy pin aliases: A7 = DAC_OUT2 (pin 29), A8 = DAC_OUT1 (pin 30), A9 = SAI2_MCLK
(pin 31). The two switches are debounced through the U1 LS18-P before reaching D13/D14.

**Sensor thresholds:**
- FSR (A8): raw readings below 20 are floored to 0; gate opens above normalized 0.1
- Soft-pot (A9): raw readings below 44 are floored to 0 to suppress idle noise

> **Note:** `docs/design-notes.md` currently lists a different (incorrect) pin map —
> e.g. Pressure=A4, Ribbon=A5 — which the board netlist contradicts (Pressure=A8,
> soft-pot=A9). Update design-notes.md to match this table.

---

## Portamento Times

All control parameters are slewed through a `Port` object (a local one-pole reimplementation in `firmware/absonus/port.{h,cpp}`, `idfk` namespace — DaisySP's `Port` was removed in the LGPL split) to eliminate ADC stepping noise. Times below are the slew time constants (seconds to reach ~63% of target).

| Parameter | Slew Time |
|:--|:--|
| Carrier frequency | 10 ms |
| Modulator depth | 10 ms |
| Chorus width | 10 ms |
| Gain | 10 ms |
| Volume | 10 ms |
| Distortion | 10 ms |
| Filter freq | 10 ms |
| Filter res | 10 ms |
| Reverb level | 10 ms |
| Noise level | 10 ms |
| Tremolo frequency | 4000 ms (Leslie ramp) |
| Tremolo depth | 5000 ms |
| Tremolo width | 5000 ms |

---

## Build

Standalone CMake project. Requires `arm-none-eabi-gcc`, `cmake` (≥3.26), and `dfu-util` or `openocd`.

```bash
git submodule update --init --recursive        # first time only
cd firmware/absonus
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

cmake --build build --target program-dfu       # USB DFU: hold BOOT, tap RESET, then flash
cmake --build build --target program           # ST-Link V3 over SWD (also the debug path)
```

Release is required for glitch-free real-time audio. Watch the `FLASH:` line — it must stay
under 128 KB. Full quickstart: [`firmware/README.md`](../firmware/README.md); CLion
walkthrough: [`clion-hardware-workflow.md`](clion-hardware-workflow.md).

### Dependencies
- [libDaisy](https://github.com/electro-smith/libDaisy) `v8.1.0` — git submodule at `firmware/libDaisy/`
- [DaisySP](https://github.com/electro-smith/DaisySP) (main, `599511b`) — git submodule at `firmware/DaisySP/`
- Custom DSP modules (`FmChorus`, `Cubicnl`, `Tremor`) and the locally-provided `Port` /
  `ReverbSc` are self-contained in `firmware/absonus/`.