#include "fmchorus.h"

using namespace idfk;

void FmChorus::Init(float sampleRate)
{
  car_freq_ = 0.0f;
  car_amp_ = 0.0f;
  mod_depth_ = 0.0f;
  ratio_ = 1.0f;

  // semi-tone spread for chorus
  float upper_ratio, lower_ratio;
  upper_ratio = pow(2.0f,1.0f/12.0f);
  upper_ = upper_ratio - 1.0f;
  lower_ratio = 1.0f/upper_ratio;
  lower_ = 1.0f - lower_ratio;

  car1_.Init(sampleRate);
  car1_.SetAmp(car_amp_);
  car1_.SetFreq(car_freq_);
  car1_.SetWaveform(Oscillator::WAVE_SIN);

  car2_.Init(sampleRate);
  car2_.SetAmp(car_amp_);
  car2_.SetFreq(car_freq_);
  car2_.SetWaveform(Oscillator::WAVE_SIN);

  car3_.Init(sampleRate);
  car3_.SetAmp(car_amp_);
  car3_.SetFreq(car_freq_);
  car3_.SetWaveform(Oscillator::WAVE_SIN);

  mod1_.Init(sampleRate);
  mod1_.SetAmp(mod_depth_);
  mod1_.SetFreq(car_freq_ * ratio_);
  mod1_.SetWaveform(Oscillator::WAVE_SIN);

  mod2_.Init(sampleRate);
  mod2_.SetAmp(mod_depth_);
  mod2_.SetFreq(car_freq_ * ratio_);
  mod2_.SetWaveform(Oscillator::WAVE_SIN);

  mod3_.Init(sampleRate);
  mod3_.SetAmp(mod_depth_);
  mod3_.SetFreq(car_freq_ * ratio_);
  mod3_.SetWaveform(Oscillator::WAVE_SIN);

  width_ = 0.0f;
}

float FmChorus::Process()
{
  float mod1Freq, mod2Freq, mod3Freq;
  float car1Freq, car2Freq, car3Freq;
  float sig = 0.0f;

  car1Freq = car_freq_ - (car_freq_ * lower_ * width_);
  car2Freq = car_freq_;
  car3Freq = car_freq_ + (car_freq_ * upper_ * width_);
  
  mod1_.SetAmp(mod_depth_ * car1Freq * kDetunedScaling);
  mod1_.SetFreq(car1Freq * ratio_);
  mod1Freq = mod1_.Process();

  mod2_.SetAmp(mod_depth_ * car2Freq);
  mod2_.SetFreq(car2Freq * ratio_);
  mod2Freq = mod2_.Process();

  mod3_.SetAmp(mod_depth_ * car3Freq * kDetunedScaling);
  mod3_.SetFreq(car3Freq * ratio_);
  mod3Freq = mod3_.Process();

  car1_.SetAmp(car_amp_ / 3.0f);
  car1_.SetFreq(car1Freq + mod1Freq);
  sig += car1_.Process();

  car2_.SetAmp(car_amp_ / 3.0f);
  car2_.SetFreq(car2Freq + mod2Freq);
  sig += car2_.Process();

  car3_.SetAmp(car_amp_ / 3.0f);
  car3_.SetFreq(car3Freq + mod3Freq);
  sig += car3_.Process();

  return sig;
}
    
void FmChorus::SetFreq(float freq)
{
  car_freq_ = freq;
}

void FmChorus::SetDepth(float depth)
{
  mod_depth_ = depth;
}

void FmChorus::SetRatio(float ratio)
{
  ratio_ = ratio;
}

void FmChorus::SetAmp(float amp)
{
  car_amp_ = amp;
}

void FmChorus::SetWidth(float width)
{
  width_ = width;
}

void FmChorus::SetCarrierWaveform(const uint8_t wf)
{
  car1_.SetWaveform(wf);
  car2_.SetWaveform(wf);
  car3_.SetWaveform(wf);
}

void FmChorus::SetModulatorWaveform(const uint8_t wf)
{
  mod1_.SetWaveform(wf);
  mod2_.SetWaveform(wf);
  mod3_.SetWaveform(wf);
}