/*********************************************************************************
*
* Tremor
* 
* Tremor is essentially a tremolo with stereo panning.  The depth controls the 
* amount to attenuate the signal with the range 0.0f - 1.0f.  The modulator is set
* to a sine wave by default, but can be altered for the intended purpose.  
*
* Process() returns the mono tremolo alone.  For stereo output, use the 
* GetLeft() and GetRight() as these will apply the panning according to the
* the specified width setting.  This is currently at 1/2 the tremolo frequency.
*
*
* Copyright (C) 2023 by matt comeione
* MIT License (MIT) license.
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