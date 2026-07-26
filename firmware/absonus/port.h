/*********************************************************************************
*
* Port — Portamento / control-rate slew
*
* Single-pole smoothing filter for control signals. At each Process() call the
* output moves toward the input target; htime is the half-time (in seconds) over
* which the output covers half the remaining distance — asymptotic, never quite
* arriving. Used throughout the synth to de-zipper ADC readings before they drive
* DSP parameters.
*
* Init(float sample_rate, float htime)
*   Must be called once before use.
*
* Process(float in)
*   Slews toward `in` and returns the smoothed value. Call once per sample.
*
* SetHtime(float htime) / GetHtime()
*   Get/set the slew half-time in seconds.
*
* Reimplemented for the modernized DaisySP build — the original daisysp::Port was
* removed when DaisySP split out its LGPL modules. Algorithm (one-pole):
*   y[n] = c1 * x[n] + c2 * y[n-1],  with  c2 = 0.5^(1 / (htime * sample_rate)).
*
* Copyright (C) 2026 by matt comeione — MIT License
*
**********************************************************************************/

#pragma once
#ifndef IDFK_PORT_H
#define IDFK_PORT_H
#ifdef __cplusplus

namespace idfk
{

class Port
{
  public:
    Port() {}
    ~Port() {}

    void Init(float sample_rate, float htime);
    float Process(float in);

    inline void SetHtime(float htime) { htime_ = htime; }
    inline float GetHtime() { return htime_; }

  private:
    float htime_;
    float c1_, c2_, yt1_, prvhtim_;
    float sample_rate_, onedsr_;
};

} // namespace idfk

#endif
#endif