/*********************************************************************************
*
* Cubicnl — Cubic Nonlinearity Distortion
*
* Waveshaping distortion using a cubic soft-clipping polynomial. Adds odd-order
* harmonics (primarily 3rd) for a warm overdrive character. A DC blocker is
* applied post-distortion to remove any DC offset introduced by the offset
* parameter.
*
* Init(float sampleRate)
*   Must be called once before use. Initialises the internal DC blocking filter.
*
* Process(float in)
*   Applies cubic nonlinearity to the input sample and returns the result.
*   Call once per sample inside the audio callback.
*
* SetDrive(float drive)
*   Drive amount 0.0–1.0. Controls the steepness of the waveshaper curve.
*   Gain compensation is applied at lower drive settings to maintain a
*   consistent output level.
*
* SetOffset(float offset)
*   DC offset applied before the waveshaper, 0.0–1.0. Defaults to 0.0.
*   A non-zero offset introduces even-order harmonics (asymmetric clipping),
*   adding a slight tube-like character.
*
* Adapted from the Faust cubicnl algorithm by Julius O. Smith III.
* Note: the DaisySP DcBlock pole is 0.999 vs 0.995 in the original Faust version.
*
* Original algorithm:
* Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>
* Released under the STK-4.3 (MIT-style) license.
*
* Daisy port copyright (C) 2023 by matt comeione — MIT License
*
**********************************************************************************/

#pragma once
#ifndef IDFK_CUBICNL_H
#define IDFK_CUBICNL_H
#include "DaisyDuino.h"
#include "math.h"
#ifdef __cplusplus

namespace idfk
{

  class Cubicnl
  {
    public:
      Cubicnl() {}
      ~Cubicnl() {}
      
      void Init(float sampleRate);
      float Process(float in);
      void SetDrive(float drive);
      void SetOffset(float offset);

    private:
      float offset_, drive_;
      DcBlock dcblocker_;
  };

} // namespace idfk

#endif
#endif