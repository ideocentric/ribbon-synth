#include "fm.h"

using namespace idfk;

void Fm::Init(float sampleRate)
{
  car_freq_ = 0.0f;
  car_amp_ = 0.0f;
  mod_depth_ = 0.0f;
  ratio_ = 1.0f;

  car_.Init(sampleRate);
  car_.SetAmp(car_amp_);
  car_.SetFreq(car_freq_);
  car_.SetWaveform(Oscillator::WAVE_SIN);

  mod_.Init(sampleRate);
  mod_.SetAmp(mod_depth_);
  mod_.SetFreq(car_freq_ * ratio_);
  mod_.SetWaveform(Oscillator::WAVE_SIN);
}

float Fm::Process()
{
  float mod;
  mod_.SetAmp(mod_depth_ * car_freq_);
  mod_.SetFreq(car_freq_ * ratio_);
  mod = mod_.Process();
  car_.SetAmp(car_amp_);
  car_.SetFreq(car_freq_ + mod);
  return car_.Process();
}
    
void Fm::SetFreq(float freq)
{
  car_freq_ = freq;
}

void Fm::SetDepth(float depth)
{
  mod_depth_ = depth;
}

void Fm::SetRatio(float ratio)
{
  ratio_ = ratio;
}

void Fm::SetAmp(float amp)
{
  car_amp_ = amp;
}


void Fm::SetCarrierWaveform(const uint8_t wf)
{
  car_.SetWaveform(wf);
}

void Fm::SetModulatorWaveform(const uint8_t wf)
{
  mod_.SetWaveform(wf);
}