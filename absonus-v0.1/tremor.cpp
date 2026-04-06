#include "tremor.h"

using namespace idfk;

void Tremor::Init(float sampleRate)
{
  freq_ = 0.0f;
  depth_ = 0.0f;
  width_ = 0.0f;

  trem_.Init(sampleRate);
  trem_.SetWaveform(Oscillator::WAVE_SIN);
  trem_.SetAmp(1.0f);
  trem_.SetFreq(0.0f);

  treml_.Init(sampleRate);
  treml_.SetWaveform(Oscillator::WAVE_SIN);
  treml_.SetAmp(1.0f);
  treml_.SetFreq(0.1f);
  
  tremr_.Init(sampleRate);
  tremr_.SetWaveform(Oscillator::WAVE_SIN);
  tremr_.SetAmp(1.0f);
  tremr_.SetFreq(0.1f);
  /* Set inverted from treml_ */
  tremr_.PhaseAdd(0.5f);
}

float Tremor::Process()
{
  float trem;
  float depth = depth_ * 0.5f;

  trem_.SetFreq(freq_);
  trem_.SetAmp(depth);

  treml_.SetFreq(freq_ * 0.5f);
  treml_.SetAmp(depth * width_);

  tremr_.SetFreq(freq_ * 0.5f);
  tremr_.SetAmp(depth * width_);

  trem = 1 - (trem_.Process() + depth);
  outl_ = (1 - (treml_.Process() + depth * width_)) * trem;
  outr_ = (1 - (tremr_.Process() + depth * width_)) * trem;

  return trem;
}

float Tremor::GetLeft()
{
  return outl_;
}

float Tremor::GetRight()
{
  return outr_;
}

void Tremor::SetFreq(float freq)
{
  freq_ = freq;
}

void Tremor::SetDepth(float depth)
{
  depth_ = depth;
}

void Tremor::SetWidth(float width)
{
  width_ = width;
}

void Tremor::SetWaveform(const uint8_t wf)
{
  trem_.SetWaveform(wf);
  treml_.SetWaveform(wf);
  tremr_.SetWaveform(wf);
}
