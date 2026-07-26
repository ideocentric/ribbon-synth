/*********************************************************************************
*
* ReverbSc — 8-delay-line stereo feedback reverb
*
* Vendored into this project from DaisySP (pre-LGPL-split) so the modernized
* build keeps the same reverb the Arduino firmware used. Current DaisySP ships no
* reverb; this classic module was moved to the separate DaisySP-LGPL repo. Only
* the namespace has been changed (daisysp -> idfk) — the algorithm is unchanged.
*
* The internal delay buffer (`aux_`, ~395 KB) is large: declare instances with
* the libDaisy DSY_SDRAM_BSS attribute so they land in external SDRAM, e.g.
*   static ReverbSc DSY_SDRAM_BSS reverb;
*
* Reverb SC:              Ported from Csound / Soundpipe
* Original author(s):     Sean Costello, Istvan Varga (1999, 2005)
* Ported to Soundpipe by: Paul Batchelor
* Ported to DaisySP by:   Stephen Hensley
* Released under the LGPL (see DaisySP-LGPL).
*
**********************************************************************************/

#pragma once
#ifndef IDFK_REVERBSC_H
#define IDFK_REVERBSC_H

#define IDFK_REVERBSC_MAX_SIZE 98936

namespace idfk
{
/** Delay line for internal reverb use */
typedef struct
{
    int    write_pos;         /**< write position */
    int    buffer_size;       /**< buffer size */
    int    read_pos;          /**< read position */
    int    read_pos_frac;     /**< fractional component of read pos */
    int    read_pos_frac_inc; /**< increment for fractional */
    int    dummy;             /**<  dummy var */
    int    seed_val;          /**< randseed */
    int    rand_line_cnt;     /**< number of random lines */
    float  filter_state;      /**< state of filter */
    float *buf;               /**< buffer ptr */
} ReverbScDl;

/** Stereo Reverb (see file header for provenance) */
class ReverbSc
{
  public:
    ReverbSc() {}
    ~ReverbSc() {}
    /** Initializes the reverb module, and sets the sample_rate at which the Process function will be called.
        Returns 0 if all good, or 1 if it runs out of delay times exceed maximum allowed.
    */
    int Init(float sample_rate);

    /** Process the input through the reverb, and updates values of out with the new processed signal.
    */
    int Process(const float &in, float *out);

    /** Process the input through the reverb, and updates values of out1, and out2 with the new processed signal.
    */
    int Process(const float &in1, const float &in2, float *out1, float *out2);

    /** controls the reverb time. reverb tail becomes infinite when set to 1.0
        \param fb - sets reverb time. range: 0.0 to 1.0
    */
    inline void SetFeedback(const float &fb) { feedback_ = fb; }
    /** controls the internal dampening filter's cutoff frequency.
        \param freq - low pass frequency. range: 0.0 to sample_rate / 2
    */
    inline void SetLpFreq(const float &freq) { lpfreq_ = freq; }

  private:
    void       NextRandomLineseg(ReverbScDl *lp, int n);
    int        InitDelayLine(ReverbScDl *lp, int n);
    float      feedback_, lpfreq_;
    float      i_sample_rate_, i_pitch_mod_, i_skip_init_;
    float      sample_rate_;
    float      damp_fact_;
    float      prv_lpfreq_;
    int        init_done_;
    ReverbScDl delay_lines_[8];
    float      aux_[IDFK_REVERBSC_MAX_SIZE];
};


} // namespace idfk
#endif