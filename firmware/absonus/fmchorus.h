/*********************************************************************************
*
* FmChorus
*
* Three-voice FM synthesis engine with stereo chorus detuning. Each voice is a
* carrier/modulator oscillator pair. The center voice runs at the specified
* frequency; the upper and lower voices are detuned symmetrically and output
* at a reduced amplitude (kDetunedScaling) to keep the center pitch dominant.
*
* Init(float sampleRate)
*   Must be called once before use. Pass the hardware sample rate (e.g. 48000).
*
* Process()
*   Advances all six oscillators by one sample and returns the mixed output.
*   Call once per sample inside the audio callback.
*
* SetFreq(float freq)
*   Carrier frequency in Hz for the center voice. The detuned voices are
*   derived automatically from the width setting.
*
* SetDepth(float depth)
*   FM modulation depth in the range 0.0–1.0. Internally scaled to
*   0–carrier_frequency Hz. At 0 the output is pure carrier; at 1 the
*   modulator sweeps a full octave around the carrier.
*
* SetRatio(float ratio)
*   Modulator frequency = carrier frequency × ratio. The current patch fixes
*   this at sqrt(2) ≈ 1.4142 (an equal-tempered tritone).
*
* SetAmp(float amp)
*   Output amplitude of the center voice, 0.0–1.0. Driven by the envelope
*   multiplied by the force-sensor gain reading.
*
* SetWidth(float width)
*   Chorus detuning amount, 0.0–1.0. At 0 all three voices are in unison;
*   at 1 the outer voices are detuned by one equal-tempered semitone
*   (frequency ratio 2^(1/12)) above and below the center.
*
* SetCarrierWaveform(const uint8_t wf)
* SetModulatorWaveform(const uint8_t wf)
*   Waveform selection using the Oscillator::WAVE_* constants from DaisySP
*   (e.g. WAVE_SAW, WAVE_TRI, WAVE_SIN). Carrier defaults to WAVE_SAW;
*   modulator defaults to WAVE_TRI.
*
* Copyright (C) 2023 by matt comeione
* MIT License
*
**********************************************************************************/

#pragma once
#ifndef IDFK_FM_H
#define IDFK_FM_H
#include "DaisyDuino.h"
#include <stdint.h>
#ifdef __cplusplus

namespace idfk
{

class FmChorus
{
  public:
    FmChorus() {}
    ~FmChorus() {}

    void Init(float sampleRate);
    float Process();
    void SetFreq(float in);
    void SetDepth(float depth);
    void SetRatio(float ratio);
    void SetAmp(float amp);
    void SetWidth(float width);
    void SetCarrierWaveform(const uint8_t wf);
    void SetModulatorWaveform(const uint8_t wf);

  private:
    // Detuned voices are output at 90% amplitude to keep center pitch dominant.
    const float kDetunedScaling = 0.9;
    Oscillator car1_, car2_, car3_, mod1_, mod2_, mod3_;
    float car_freq_, car_amp_, mod_depth_, ratio_, width_, lower_, upper_;
};

} // namespace idfk

#endif
#endif