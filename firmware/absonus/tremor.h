/*********************************************************************************
*
* Tremor — Stereo Tremolo with Leslie-Inspired Panning
*
* Amplitude modulator with independent stereo panning. Loosely inspired by the
* Leslie speaker cabinet: tremolo rate ramps between slow/fast presets and the
* stereo panning oscillates at half the tremolo frequency, creating an
* alternating left-right sweep that mimics the Doppler-like movement of a
* rotating speaker without directly replicating it.
*
* Init(float sampleRate)
*   Must be called once before use. Pass the hardware sample rate (e.g. 48000).
*
* Process()
*   Advances all three internal oscillators (trem, treml, tremr) by one sample
*   and returns the mono tremolo scalar. Apply to signal before GetLeft/GetRight.
*
* GetLeft() / GetRight()
*   Returns the stereo panning scalar for the left or right channel. Multiply
*   the mono-tremolo-processed signal by each to produce the stereo output.
*   The pan oscillators run at 0.5× the tremolo frequency.
*
* SetFreq(float freq)
*   Tremolo LFO frequency in Hz. Range used in practice: 1.0–13.0 Hz.
*   Portamento on this parameter provides the Leslie speed-ramp effect.
*
* SetDepth(float depth)
*   Tremolo depth 0.0–1.0. At 0 the amplitude is unmodulated; at 1 the signal
*   is fully gated at the LFO trough.
*
* SetWidth(float width)
*   Stereo panning width 0.0–1.0. At 0 both channels are centred; at 1 the
*   pan sweep reaches full left/right alternation.
*
* SetWaveform(const uint8_t wf)
*   LFO waveform using Oscillator::WAVE_* constants. Defaults to WAVE_SIN.
*
* Copyright (C) 2023 by matt comeione — MIT License
*
**********************************************************************************/

#pragma once
#ifndef IDFK_TREMOR_H
#define IDFK_TREMOR_H
#include "DaisyDuino.h"
#include "math.h"
#include <stdint.h>
#ifdef __cplusplus

namespace idfk
{

  class Tremor
  {
    public:
      Tremor() {}
      ~Tremor() {}
      
      void Init(float sampleRate);
      float Process();
      float GetLeft();
      float GetRight();
      void SetFreq(float freq);
      void SetDepth(float depth);
      void SetWidth(float width);
      void SetWaveform(const uint8_t wf);

    private:
      float freq_, depth_, width_;
      float outl_, outr_;
      Oscillator trem_, treml_, tremr_;
  };

} // namespace idfk

#endif
#endif