# Absonus Firmware Architecture

Firmware for the Absonus ribbon synthesizer. Written in C++ for the Electrosmith Daisy Seed using the DaisyDuino Arduino framework and the DaisySP DSP library.

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
                   filterFreq ──► MoogLadder Filter ◄── filterRes
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

### MoogLadder, ReverbSc, WhiteNoise

Standard DaisySP modules used without modification.

| Module | Control | Range | Notes |
|:--|:--|:--|:--|
| MoogLadder | Cutoff knob (A1\*), Res knob (A5\*) | 20–24000 Hz / 0.0–1.0 | Exponential frequency mapping |
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

| ADC Pin | Label | Function | Scaling |
|:--|:--|:--|:--|
| A0 | White | Distortion drive | Linear 0–1 |
| A1 | Purple | Volume | Linear 0–1 |
| A2 | Green | Noise level | Linear 0–1 |
| A3 | Orange | Filter cutoff | Exponential 20–24000 Hz |
| A4 | Pressure | FSR — gain + gate | Linear 0–1, threshold at 0.025 |
| A5 | Ribbon | SoftPot — pitch | Exponential 55–3520 Hz |
| A6 | Yellow | Filter resonance | Linear 0–1 |
| A7 | Blue | Reverb level | Linear 0–1 |
| A8 | Gray | Chorus width | Linear 0–1 |
| A9 | Black | FM modulation depth | Linear 0–1 |
| D13 | SW2 | Tremolo speed (slow/fast) | Digital |
| D14 | SW1 | Tremolo on/off | Digital |

**Sensor thresholds:**
- FSR: readings below 20 (raw) are floored to 0; gate opens above normalized 0.1
- SoftPot: readings below 44 (raw) are floored to 0 to suppress idle noise

---

## Portamento Times

All control parameters are slewed through a `Port` object to eliminate ADC stepping noise. Times below are the slew time constants (seconds to reach ~63% of target).

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

Open `firmware/absonus/absonus.ino` in Arduino IDE with the [DaisyDuino](https://github.com/electro-smith/DaisyDuino) board package installed. Select **Daisy Seed** as the target board and upload via the Daisy boot-loader.

### Dependencies
- [DaisyDuino](https://github.com/electro-smith/DaisyDuino) (includes DaisySP)
- All custom DSP modules (`FmChorus`, `Cubicnl`, `Tremor`) are self-contained in the sketch folder.