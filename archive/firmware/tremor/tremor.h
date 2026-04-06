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