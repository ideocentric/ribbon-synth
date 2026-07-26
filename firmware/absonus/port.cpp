#include "port.h"
#include <math.h>

using namespace idfk;

void Port::Init(float sample_rate, float htime)
{
  yt1_         = 0.0f;
  prvhtim_     = -100.0f;
  htime_       = htime;
  sample_rate_ = sample_rate;
  onedsr_      = 1.0f / sample_rate_;
}

float Port::Process(float in)
{
  if(prvhtim_ != htime_)
  {
    c2_      = powf(0.5f, onedsr_ / htime_);
    c1_      = 1.0f - c2_;
    prvhtim_ = htime_;
  }
  return yt1_ = c1_ * in + c2_ * yt1_;
}