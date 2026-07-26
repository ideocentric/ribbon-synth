// absonus — ribbon synthesizer firmware for the Electrosmith Daisy Seed.
//
// Ported from the DaisyDuino sketch to bare-metal libDaisy + DaisySP. The signal
// chain, control mapping, and DSP behavior are unchanged from the Arduino version;
// only the framework glue differs:
//   - setup()/loop()   -> main() preamble + while(1) control loop
//   - analogRead()      -> hw.adc.GetFloat() (ADC configured via AdcChannelConfig)
//   - digitalRead()     -> daisy::GPIO
//   - DAISY.begin(cb)   -> hw.StartAudio(AudioCallback)
//
// See docs/firmware-architecture.md for the signal chain and pin mapping.

#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/sdram.h"
#include <math.h>
#include "fmchorus.h"
#include "cubicnl.h"
#include "tremor.h"
#include "port.h"
#include "reverbsc.h"

using namespace daisy;
using namespace daisysp;
using namespace idfk;

/* Hardware Setup */
static DaisySeed hw;

/* Debugging Settings */
const bool kDebug = false;

/* Analog Reading Settings */
const long kAdcMax = 4095;

/* Frequency Range: A1-A7 */
const float kCarrierFrequencyLow = 55.0;
const float kCarrierFrequencyHigh = 3520.0;

/* ADC channels — configured in this order, indexed by function.
   Physical pins (from the original sketch): distortion=A0, filterFreq=A1,
   noise=A2, volume=A3, reverb=A4, filterRes=A5, modDepth=A6, chorus=A7,
   force=A8, softPot=A9. */
enum AdcChannel
{
  CH_DISTORTION = 0, // A0
  CH_FILTER_FREQ,    // A1
  CH_NOISE,          // A2
  CH_VOLUME,         // A3
  CH_REVERB,         // A4
  CH_FILTER_RES,     // A5
  CH_MOD_DEPTH,      // A6
  CH_CHORUS,         // A7
  CH_FORCE,          // A8
  CH_SOFTPOT,        // A9
  ADC_CHANNELS
};

/* Tremolo control switches (digital) */
static GPIO tremoloSpeedSwitch; // D13
static GPIO tremoloOnSwitch;    // D14

/* Sensor calibration settings for lower threshold */
const long kForceSensorThreshold = 20;
const float kGateThreshhold = 0.1;
const long kSoftPotThreshold = 44;

/* Envelope, Gain, Volume */
static Adsr envelope;

static Port gainPortamento;
static bool gate = false;
static long gainReading;
static float gain;

static long volumeReading;
static Port volumePortamento;
static float volumeLevel;

/* FM Synthesis Section */
static FmChorus fmOsc;
static Port carrierPortamento;
static float carrierScalar;
static float carrierFreq;
static long carrierFreqReading;
static float previousCarrierFreq;

static long modulatorDepthReading;
static Port modulatorDepthPortamento;
static float modulatorDepth;

static long modulatorRatioReading;
static Port modulatorRatioPortamento;
static float modulatorRatio;

static Port chorusDepthPortamento;
static long chorusDepthReading;
static float chorusDepth;

/* Tremolo Settings */
static Tremor tremolo;
static Port tremoloDepthPortamento;
static float tremoloDepth;
static Port tremoloWidthPortamento;
static float tremoloWidth;
static Port tremoloFreqPortamento;
static float tremoloFreq;
static int tremoloSpeedReading;
static int tremoloOnReading;
static bool tremoloOn;
static float tremoloScalar;

const float kTremoloFreqLow = 1.0f;
const float kTremoloDepthLow = 0.5f;
const float kTremoloWidthLow = 0.66;

const float kTremoloFreqHigh = 13.0f;
const float kTremoloDepthHigh = 0.75f;
const float kTremoloWidthHigh = 0.83f;

/* Distortion Settings */
static Cubicnl distortion;
static long distortionLevelReading;
static Port distortionLevelPortamento;
static float distortionLevel;

/* Filter Settings */
static LadderFilter filter;  // modern DaisySP replacement for the removed MoogLadder (LP24 by default)
static float filterFreq;
static long filterFreqReading;
static Port filterFreqPortamento;
static float filterRes;
static long filterResReading;
static Port filterResPortamento;
static float filterScalar;

const float kFilterFreqLow = 20.0f;
const float kFilterFreqHigh = 24000.0f;
// LadderFilter resonance goes to 1.8 (self-oscillation); scale the 0-1 knob up to it
// so full-CW reaches self-oscillation like the old MoogLadder. (MoogLadder used 0-1.)
const float kFilterResMax = 1.8f;

/* Reverb Settings */
// idfk::ReverbSc (vendored). DSY_SDRAM_BSS puts its ~395 KB buffer in external SDRAM.
static ReverbSc DSY_SDRAM_BSS reverb;
static Port reverbLevelPortamento;
static float reverbLevel;
static long reverbLevelReading;
static CrossFade crossfadel, crossfader;

/* Noise Settings */
static WhiteNoise noise;
static Port noiseLevelPortamento;
static float noiseLevel;
static long noiseLevelReading;

/* Arduino compatibility helpers (integer constrain/map, matching Arduino semantics) */
static inline long constrainL(long x, long lo, long hi)
{
  return x < lo ? lo : (x > hi ? hi : x);
}

static inline long mapL(long x, long inMin, long inMax, long outMin, long outMax)
{
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

/* Read a channel as a 0..kAdcMax raw value (reconstructed from the normalized
   16-bit ADC), so the original 12-bit threshold math is preserved. */
static inline long analogReadRaw(int channel)
{
  return (long)(hw.adc.GetFloat(channel) * (float)kAdcMax);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  float slewedVolumeLevel, slewedGain;
  float envelopeOut, sig, sigl, sigr;
  float slewedCarrierFreq, slewedModulatorDepth, slewedModulatorRatio, slewedChorusDepth;
  float slewedDistortionLevel;
  float slewedTremoloFreq, slewedTremoloDepth, slewedTremoloWidth;
  float trem, treml, tremr;
  float slewedReverbLevel;
  float slewedNoiseLevel;

  for (size_t i = 0; i < size; i++)
  {
    envelopeOut = envelope.Process(gate);
    slewedVolumeLevel = volumePortamento.Process(volumeLevel);
    slewedGain = gainPortamento.Process(gain);

    if(gate == true)
    {
      previousCarrierFreq = carrierFreq;
      slewedCarrierFreq = carrierPortamento.Process(carrierFreq);
      previousCarrierFreq = slewedCarrierFreq;
    }
    else
    {
      // Retain previousCarrierFreq on Note Off to prevent fall off to kCarrierFrequencyLow
      slewedCarrierFreq = carrierPortamento.Process(previousCarrierFreq);
    }

    //slewedModulatorRatio = modulatorRatioPortamento.Process(modulatorRatio);
    slewedModulatorDepth = modulatorDepthPortamento.Process(modulatorDepth);

    slewedChorusDepth = chorusDepthPortamento.Process(chorusDepth);

    fmOsc.SetAmp(envelopeOut * slewedGain);
    //fmOsc.SetRatio(slewedModulatorRatio);  // switched to fixed ratio
    fmOsc.SetRatio(1.414214);
    fmOsc.SetDepth(slewedModulatorDepth);
    fmOsc.SetFreq(slewedCarrierFreq);
    fmOsc.SetWidth(slewedChorusDepth);
    sig = fmOsc.Process();

    slewedNoiseLevel = noiseLevelPortamento.Process(noiseLevel);
    noise.SetAmp(slewedNoiseLevel);
    sig = sig + noise.Process() * 0.5 * slewedGain;

    slewedDistortionLevel = distortionLevelPortamento.Process(distortionLevel);
    distortion.SetDrive(slewedDistortionLevel);
    sig = distortion.Process(sig);

    filter.SetRes(filterRes);
    filter.SetFreq(filterFreq);
    sig = filter.Process(sig);

    slewedTremoloFreq = tremoloFreqPortamento.Process(tremoloFreq);
    slewedTremoloDepth = tremoloDepthPortamento.Process(tremoloDepth);
    slewedTremoloWidth = tremoloWidthPortamento.Process(tremoloWidth);
    tremolo.SetFreq(slewedTremoloFreq);
    tremolo.SetDepth(slewedTremoloDepth);
    tremolo.SetWidth(slewedTremoloWidth);

    trem = tremolo.Process();
    treml = tremolo.GetLeft();
    tremr = tremolo.GetRight();

    sig = sig * trem;
    sigl = sig * treml;
    sigr = sig * tremr;

    slewedReverbLevel = reverbLevelPortamento.Process(reverbLevel);
    reverb.SetFeedback(slewedReverbLevel);
    float outl, outr;
    reverb.Process(sigl, sigr, &outl, &outr);

    crossfadel.SetPos(slewedReverbLevel);
    crossfader.SetPos(slewedReverbLevel);

    sigl = crossfadel.Process(sigl, outl);
    sigr = crossfadel.Process(sigr, outr);
    //sigl = sigl * (1 - slewedReverbLevel) + outl * slewedReverbLevel;
    //sigr = sigr * (1 - slewedReverbLevel) + outr * slewedReverbLevel;

    out[0][i] = sigl * slewedVolumeLevel;
    out[1][i] = sigr * slewedVolumeLevel;
  } // end for loop
} // End AudioCallback


float normalizeSoftpot(long sensorValue)
{
  long value;
  float normalizedValue;
  value = constrainL(sensorValue, kSoftPotThreshold, kAdcMax);
  value = mapL(value, kSoftPotThreshold, kAdcMax, 0, kAdcMax);
  normalizedValue = (float)value/(float)kAdcMax;
  //return 1.0f - normalizedValue;
  return normalizedValue;
} // End normalizeSoftpot(long sensorValue)


float normalizeForceSensor(long sensorValue)
{
  long value;
  float normalizedValue;
  value = constrainL(sensorValue, kForceSensorThreshold, kAdcMax);
  value = mapL(value, kForceSensorThreshold, kAdcMax, 0, kAdcMax);
  normalizedValue = (float) value / (float) kAdcMax;
  if (normalizedValue > kGateThreshhold) {
    gate = true;
  }
  else {
    gate = false;
  }
  return normalizedValue;
} // End normalizeForceSensor(long sensorValue)


void setup()
{
  float sampleRate;
  /* hardware initialization */
  hw.Init();
  hw.SetAudioBlockSize(48);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  sampleRate = hw.AudioSampleRate();

  /* ADC initialization — all ten analog inputs */
  AdcChannelConfig adcConfig[ADC_CHANNELS];
  adcConfig[CH_DISTORTION].InitSingle(seed::A0);
  adcConfig[CH_FILTER_FREQ].InitSingle(seed::A1);
  adcConfig[CH_NOISE].InitSingle(seed::A2);
  adcConfig[CH_VOLUME].InitSingle(seed::A3);
  adcConfig[CH_REVERB].InitSingle(seed::A4);
  adcConfig[CH_FILTER_RES].InitSingle(seed::A5);
  adcConfig[CH_MOD_DEPTH].InitSingle(seed::A6);
  adcConfig[CH_CHORUS].InitSingle(seed::A7);
  adcConfig[CH_FORCE].InitSingle(seed::A8);
  adcConfig[CH_SOFTPOT].InitSingle(seed::A9);
  hw.adc.Init(adcConfig, ADC_CHANNELS);
  hw.adc.Start();

  /* Tremolo control switches (floating INPUT, matching the original sketch) */
  tremoloSpeedSwitch.Init(seed::D13, GPIO::Mode::INPUT, GPIO::Pull::NOPULL);
  tremoloOnSwitch.Init(seed::D14, GPIO::Mode::INPUT, GPIO::Pull::NOPULL);

  if(kDebug)
  {
    hw.StartLog(false);
  }

  /* FM Synthesis Initialization */
  carrierScalar = log2(kCarrierFrequencyHigh / kCarrierFrequencyLow);
  modulatorRatio = 1.0f;
  modulatorDepth = 0.0f;
  chorusDepth = 0.0f;

  fmOsc.Init(sampleRate);
  fmOsc.SetFreq(kCarrierFrequencyLow);
  fmOsc.SetCarrierWaveform(Oscillator::WAVE_SAW);
  fmOsc.SetModulatorWaveform(Oscillator::WAVE_TRI);

  fmOsc.SetRatio(modulatorRatio);
  fmOsc.SetDepth(modulatorDepth);
  fmOsc.SetAmp(gain);

  carrierPortamento.Init(sampleRate, 0.01f);
  modulatorDepthPortamento.Init(sampleRate, 0.01);
  //modulatorRatioPortamento.Init(sampleRate, 0.01);
  chorusDepthPortamento.Init(sampleRate, 0.01);

  /* Envelope, Gain and Volume Initialization */
  gate = false;
  gain = 0.0f;

  envelope.Init(sampleRate);
  envelope.SetTime(ADSR_SEG_ATTACK,0.1);
  envelope.SetTime(ADSR_SEG_DECAY, 0.1);
  envelope.SetTime(ADSR_SEG_RELEASE, 0.01);
  envelope.SetSustainLevel(0.7);
  gainPortamento.Init(sampleRate, 0.01);
  volumePortamento.Init(sampleRate, 0.01);

  /* Tremolo Initialization */
  tremoloFreq = 0.0f;
  tremoloOn = false;
  tremoloScalar = log2(kTremoloFreqHigh / kTremoloFreqLow);  // TODO: Ensure this is better than linear scaling
  tremolo.Init(sampleRate);
  tremolo.SetDepth(0.5f);
  tremolo.SetFreq(kTremoloFreqLow);
  tremolo.SetWidth(0.5f);
  tremoloFreqPortamento.Init(sampleRate, 4.0f);
  tremoloDepthPortamento.Init(sampleRate, 5.0f);
  tremoloWidthPortamento.Init(sampleRate, 5.0f);

  /* Distortion Initialization */
  distortionLevelPortamento.Init(sampleRate, 0.01);
  distortion.Init(sampleRate);
  distortion.SetDrive(0.0f);
  distortion.SetOffset(0.0f);

  /* Filter Initialization */
  filter.Init(sampleRate);
  filter.SetRes(0.0f);
  filterFreqPortamento.Init(sampleRate, 0.01);
  filterResPortamento.Init(sampleRate, 0.01);
  filterFreq = kFilterFreqHigh;
  filterRes = 0.0f;
  filterScalar = log2(kFilterFreqHigh / kFilterFreqLow);

  /* Reverb Initialization */
  reverb.Init(sampleRate);
  reverb.SetFeedback(0.0f);
  reverb.SetLpFreq(18000.0f);
  reverbLevelPortamento.Init(sampleRate, 0.01);

  crossfadel.Init();
  crossfadel.SetCurve(CROSSFADE_CPOW);
  crossfader.Init();
  crossfader.SetCurve(CROSSFADE_CPOW);

  /* Noise Initialization */
  noise.Init();
  noise.SetAmp(0.0f);
  noiseLevelPortamento.Init(sampleRate, 0.01);

  hw.StartAudio(AudioCallback);
}  // End setup()


void loop()
{
  static uint32_t lastPrintTime = 0;
  uint32_t now;

  /* Oscillator Gain and Volume */
  volumeReading = analogReadRaw(CH_VOLUME);
  volumeLevel = volumeReading / (float) kAdcMax;

  gainReading = analogReadRaw(CH_FORCE);
  gain = normalizeForceSensor(gainReading);

  carrierFreqReading = analogReadRaw(CH_SOFTPOT);
  carrierFreq = pow(2.0, normalizeSoftpot(carrierFreqReading) * carrierScalar) * kCarrierFrequencyLow;

  modulatorDepthReading = analogReadRaw(CH_MOD_DEPTH);
  modulatorDepth = modulatorDepthReading  / (float) kAdcMax;

  //modulatorRatioReading = analogReadRaw(CH_MOD_RATIO);
  //modulatorRatio = (modulatorRatioReading / (float) kAdcMax) + 1.0f;

  chorusDepthReading = analogReadRaw(CH_CHORUS);
  chorusDepth = (chorusDepthReading / (float) kAdcMax);

  /* Tremolo Readings - 2 switch controls */
  tremoloOnReading = tremoloOnSwitch.Read() ? 1 : 0;
  tremoloSpeedReading = tremoloSpeedSwitch.Read() ? 1 : 0;
  if(tremoloOnReading > 0)
  {
    tremoloOn = false;
    tremoloFreq = 0.0f;
    tremoloDepth = 0.0f;
    tremoloWidth = 0.0f;
  }
  else
  {
    tremoloOn = true;
    if(tremoloSpeedReading > 0)
    {
      tremoloFreq = kTremoloFreqLow;
      tremoloDepth = kTremoloDepthLow;
      tremoloWidth = kTremoloWidthLow;
    }
    else
    {
      tremoloFreq = kTremoloFreqHigh;
      tremoloDepth = kTremoloDepthHigh;
      tremoloWidth = kTremoloWidthHigh;
    }
  }

  /* Distortion Reading */
  distortionLevelReading = analogReadRaw(CH_DISTORTION);
  distortionLevel = (distortionLevelReading / (float) kAdcMax);

  /* Filter Readings */
  filterFreqReading = analogReadRaw(CH_FILTER_FREQ);
  filterFreq = pow(2.0, (filterFreqReading / (float) kAdcMax) * filterScalar) * kFilterFreqLow;

  filterResReading = analogReadRaw(CH_FILTER_RES);
  filterRes = (filterResReading / (float) kAdcMax) * kFilterResMax;

  /* Reverb Readings */
  reverbLevelReading = analogReadRaw(CH_REVERB);
  reverbLevel = reverbLevelReading / (float) kAdcMax;

  /* Noise Reading */
  noiseLevelReading = analogReadRaw(CH_NOISE);
  noiseLevel = noiseLevelReading / (float) kAdcMax;

  if(kDebug)
  {
    now = System::GetNow();
    if((now - lastPrintTime > 100))
    {
      lastPrintTime = now;
      hw.PrintLine("modulatorDepth:%.2f,distortionLevel:%.2f,chorusDepth:%.2f,"
                   "tremoloSpeedReading:%d,tremoloOnReading:%d,filterFreq:%.2f,"
                   "filterRes:%.2f,noiseLevel:%.2f,reverbLevel:%.2f,volumeLevel:%.2f,"
                   "carrierFreq:%.2f,gain:%.2f",
                   modulatorDepth, distortionLevel, chorusDepth,
                   tremoloSpeedReading, tremoloOnReading, filterFreq,
                   filterRes, noiseLevel, reverbLevel, volumeLevel,
                   carrierFreq, gain);
    }
  }
} // End loop()


int main(void)
{
  setup();
  while(1)
  {
    loop();
  }
}