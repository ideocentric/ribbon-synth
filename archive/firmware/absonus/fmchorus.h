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
    const float kDetunedScaling = 0.9;
    Oscillator car1_, car2_, car3_, mod1_, mod2_, mod3_;
    float car_freq_, car_amp_, mod_depth_, ratio_, width_, lower_, upper_;
};

} // namespace idfk

#endif
#endif