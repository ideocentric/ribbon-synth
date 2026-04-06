#include "cubicnl.h"

using namespace idfk;

void Cubicnl::Init(float sampleRate)
{
  drive_ = 0.0f;
  offset_ = 0.0f;
  dcblocker_.Init(sampleRate);
}

float Cubicnl::Process(float in)
{
  float sig, pregain, postgain;
  sig = in + offset_;
  pregain = pow(10.0f, 2.0f * drive_);
  sig = sig * pregain;
  sig = min(sig, 1.0f);
  sig = max(sig, -1.0f);
  sig = sig - (sig*sig*sig)/3;
  sig = dcblocker_.Process(sig);
  postgain = 1.0 - 0.292 * fmap(drive_, 0.0f, 1.0f, Mapping::LINEAR);
  return sig * postgain;
}
    
void Cubicnl::SetDrive(float drive)
{
  drive_ = drive;
}

void Cubicnl::SetOffset(float offset)
{
  offset_ = offset;
}

