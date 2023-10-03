#pragma once
#ifndef IDFK_FM_H
#define IDFK_FM_H
#include "DaisyDuino.h"
#include <stdint.h>
#ifdef __cplusplus

namespace idfk
{
class Fm
{
  public:
    Fm() {}
    ~Fm() {}

    void Init(float sampleRate);
    float Process();
    void SetFreq(float in);
    void SetDepth(float in);
    void SetRatio(float ratio);
    void SetAmp(float in);
    void SetCarrierWaveform(const uint8_t wf);
    void SetModulatorWaveform(const uint8_t wf);

  private:
    Oscillator car_, mod_;
    float car_freq_, car_amp_, mod_depth_, ratio_;
};
} // namespace idfk


#endif
#endif