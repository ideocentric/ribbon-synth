/*********************************************************************************
*
* Cubicnl
*
* SetDrive(float drive) - Drive should be set between 0.0f - 1.0f for the level of 
* drive desired in ouput.  Some gain adjustment is made for lower drive settings.
*
* SetOffset(float offset) - Offset should be set between 0.0f - 1.0f as desired.  
* Offset defaults to 0.0f.  It has been indicated a > 0.0 will increase in even
* harmonics.
*
* @in - Input signal is required for Cubicnl::Process(float in).  Signal to be 
* over driven.
*
* The following is an adaptation of the Faust cubicnl originally created by 
* Julius O. Smith III.  One slight difference is the dcblocker in the DaisySP
* uses a pole set at 0.999 whereas the Faust uses 0.995.  
* Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>
* ([jos](http://ccrma.stanford.edu/~jos/)), and released under the
* (MIT-style) [STK-4.3](#stk-4.3-license) license.
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