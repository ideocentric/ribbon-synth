#include "DaisyDuino.h"
#include "math.h"
#include "fm.h"
#include "cubicnl.h"
#include "tremor.h"

using namespace idfk;

const int kAdcBits = 12;
const long kAdcMax = 4095;

// A1 - A7
const float kCarrierFrequencyLow = 55.0;
const float kCarrierFrequencyHigh = 3520.0;

const int kModulationDepthPin = A9;
const int kModulationRatioPin = A0;
const int kVolumePin = A1;
const int kForceSensorPin = A4;
const int kSoftPotPin = A5;
const int kDistortionLevelPin = A3;
const int kChorusDepthPin = A8;
/* -- original ports - hijacking for testing tremolo
const int kNoiseLevelPin = A6;
const int kFilterCutOffPin = A2;
const int kFilterResonancePin = A7;
*/
const int kTremoloFreqPin = A6;
const int kTremoloDepthPin = A2;
const int kTremoloWidthPin = A7;


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

static Fm fmOsc;
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

static Cubicnl distortionl, distortionr;
static long distortionLevelReading;
static Port distortionLevelPortamento;
static float distortionLevel;

static Tremor tremolo;
static Port tremoloDepthPortamento;
static long tremoloDepthReading;
static float tremoloDepth;
static Port tremoloWidthPortamento;
static long tremoloWidthReading;
static float tremoloWidth;
static Port tremoloFreqPortamento;
static long tremoloFreqReading;
static float tremoloFreq;
static float tremoloScalar;

const float kTremoloFreqLow = 1.0;
const float kTremoloFreqHigh = 13.0;

const float kChorusPanWidth = 0.5;  // values 0-1, 0 meaning no witdth, 1 meaning full width.
static Chorus chorus;
static Port chorusDepthPortamento;
static long chorusDepthReading;
static long chorusDepth;

DaisyHardware hw;
size_t numChannels;


void MyCallback(float **in, float **out, size_t size) 
{
  float slewedVolumeLevel, slewedGain; 
  float slewedCarrierFreq, slewedModulatorDepth, slewedModulatorRatio;
  float slewedDistortionLevel;
  float slewedChorusDepth;
  float slewedTremoloDepth, slewedTremoloWidth, slewedTremoloFreq;
  float gainOut, envelopeOut, modulator, sig, sigl, sigr;

   for (size_t i = 0; i < size; i++) 
   {
      slewedGain = gainPortamento.Process(gain);
      envelopeOut = envelope.Process(gate) * slewedGain;
      slewedVolumeLevel = volumePortamento.Process(volumeLevel);

      if(gate == true) 
      {
        previousCarrierFreq = carrierFreq;
        slewedCarrierFreq = carrierPortamento.Process(carrierFreq);
        previousCarrierFreq = slewedCarrierFreq;
      }
      else 
      {
        slewedCarrierFreq = carrierPortamento.Process(previousCarrierFreq);
      }
      slewedModulatorRatio = modulatorRatioPortamento.Process(modulatorRatio);
      slewedModulatorDepth = modulatorDepthPortamento.Process(modulatorDepth);

      fmOsc.SetAmp(envelopeOut);
      fmOsc.SetRatio(slewedModulatorRatio);
      fmOsc.SetDepth(slewedModulatorDepth);
      fmOsc.SetFreq(slewedCarrierFreq);
      sig = fmOsc.Process();

      slewedChorusDepth = chorusDepthPortamento.Process(chorusDepth);
      float depth = fmap(slewedChorusDepth, 0.0, 0.8);
      chorus.SetLfoDepth(depth);
      float feedback = fmap(slewedChorusDepth, 0.2, 0.9);
      chorus.SetFeedback(feedback);
      chorus.Process(sig);
      sigl = chorus.GetLeft() + sig * 0.707 * 0.5;
      sigr = chorus.GetRight() + sig * 0.707 * 0.5;

      slewedDistortionLevel = distortionLevelPortamento.Process(distortionLevel);
      distortionl.SetDrive(slewedDistortionLevel);
      distortionr.SetDrive(slewedDistortionLevel);
      float dryl, dryr, scaled;
      scaled = pow(slewedDistortionLevel, 0.2f);
      dryl = sigl * (1.0f - scaled);
      dryr = sigr * (1.0f - scaled);

      sigl = distortionl.Process(sigl) + dryl;
      sigr = distortionl.Process(sigr) + dryr;

      slewedTremoloFreq = tremoloFreqPortamento.Process(tremoloFreq);
      slewedTremoloDepth = tremoloDepthPortamento.Process(tremoloDepth);
      slewedTremoloWidth = tremoloWidthPortamento.Process(tremoloWidth);
      tremolo.SetFreq(slewedTremoloFreq);
      tremolo.SetDepth(slewedTremoloDepth);
      tremolo.SetWidth(slewedTremoloWidth);
      float trem = tremolo.Process();
      float treml = tremolo.GetLeft();
      float tremr = tremolo.GetRight();

      sigl = sigl * trem * treml;
      sigr = sigr * trem * tremr;

      out[0][i] = sigl * slewedVolumeLevel;
      out[1][i] = sigr * slewedVolumeLevel;
   }
} // end MyCallback


float normalizeSoftpot(long sensorValue) 
{
  long value;
  float normalizedValue;
  value = constrain(sensorValue, kSoftPotThreshold, kAdcMax);
  value = map(value, kSoftPotThreshold, kAdcMax, 0, kAdcMax);
  normalizedValue = (float)value/(float)kAdcMax;
  return normalizedValue;
}


float normalizeForceSensor(long sensorValue) 
{
  long value;
  float normalizedValue;
  value = constrain(sensorValue, kForceSensorThreshold, kAdcMax);
  value = map(value, kForceSensorThreshold, kAdcMax, 0, kAdcMax);
  normalizedValue = (float) value / (float) kAdcMax;
  if (normalizedValue > kGateThreshhold) {
    gate = true;
  }
  else {
    gate = false;
    //Serial.println("Gate Off");
  }
  return normalizedValue;
}


void setup() 
{
  float sampleRate;

  analogReadResolution(kAdcBits);
  Serial.begin(9600);

  // Initialize for Daisy pod at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  numChannels = hw.num_channels;
  sampleRate = DAISY.get_samplerate();

  carrierScalar = log2(kCarrierFrequencyHigh / kCarrierFrequencyLow);
  modulatorRatio = 1.0f;
  modulatorDepth = 0.0f;

  fmOsc.Init(sampleRate);
  fmOsc.SetFreq(kCarrierFrequencyLow);
  fmOsc.SetCarrierWaveform(Oscillator::WAVE_SAW);
  fmOsc.SetModulatorWaveform(Oscillator::WAVE_TRI);
  
  fmOsc.SetRatio(modulatorRatio);
  fmOsc.SetDepth(modulatorDepth);
  fmOsc.SetAmp(gain);

  carrierPortamento.Init(sampleRate, 0.01);
  modulatorDepthPortamento.Init(sampleRate, 0.01);
  modulatorRatioPortamento.Init(sampleRate, 0.01);

  distortionLevelPortamento.Init(sampleRate, 0.01);
  distortionl.Init(sampleRate);
  distortionl.SetDrive(0.0f);
  distortionl.SetOffset(0.0f);

  distortionr.Init(sampleRate);
  distortionr.SetDrive(0.0f);
  distortionr.SetOffset(0.0f);

  chorus.Init(sampleRate);
  chorus.SetPan(0.2f, 0.8f);
  chorus.SetLfoDepth(0.0f, 0.0f);
  chorus.SetLfoFreq(0.8f);
  chorus.SetDelayMs(3.5f, 3.5f);
  chorus.SetFeedback(0.4f, 0.4f);
  chorusDepthPortamento.Init(sampleRate, 0.01);
  
  tremoloScalar = log2(kTremoloFreqHigh / kTremoloFreqLow);
  tremolo.Init(sampleRate);
  tremolo.SetDepth(0.5f);
  tremolo.SetFreq(kTremoloFreqLow);
  tremolo.SetWidth(0.5f);
  tremoloFreqPortamento.Init(sampleRate, 0.01);
  tremoloDepthPortamento.Init(sampleRate, 0.01);
  tremoloWidthPortamento.Init(sampleRate, 0.01);

  envelope.Init(sampleRate);
  envelope.SetTime(ADSR_SEG_ATTACK,0.1);
  envelope.SetTime(ADSR_SEG_DECAY, 0.1);
  envelope.SetTime(ADSR_SEG_RELEASE, 0.01);
  envelope.SetSustainLevel(0.7);

  gate = false;
  gain = 0.0f;
  gainPortamento.Init(sampleRate, 0.01);

  volumePortamento.Init(sampleRate, 0.01);

  DAISY.begin(MyCallback);
}


void loop() {
  unsigned long lastPrintTime = 0;
  unsigned long now;

  // put your main code here, to run repeatedly:
  volumeReading = analogRead(kVolumePin);
  volumeLevel = volumeReading / (float) kAdcMax;

  gainReading = analogRead(kForceSensorPin);
  gain = normalizeForceSensor(gainReading);

  carrierFreqReading = analogRead(kSoftPotPin);
  carrierFreq = pow(2.0, normalizeSoftpot(carrierFreqReading) * carrierScalar) * kCarrierFrequencyLow;

  modulatorDepthReading = analogRead(kModulationDepthPin);
  modulatorDepth = modulatorDepthReading  / (float) kAdcMax;

  modulatorRatioReading = analogRead(kModulationRatioPin); 
  modulatorRatio = (modulatorRatioReading / (float) kAdcMax) + 1.0f;

  distortionLevelReading = analogRead(kDistortionLevelPin);
  distortionLevel = (distortionLevelReading / (float) kAdcMax);

  chorusDepthReading = analogRead(kChorusDepthPin);
  chorusDepth = (chorusDepthReading / (float) kAdcMax);

  tremoloFreqReading = analogRead(kTremoloFreqPin);
  tremoloFreq = pow(2.0, (tremoloFreqReading / (float) kAdcMax) * tremoloScalar) * kTremoloFreqLow;

  tremoloDepthReading = analogRead(kTremoloDepthPin);
  tremoloDepth = (tremoloDepthReading / (float) kAdcMax);

  tremoloWidthReading = analogRead(kTremoloWidthPin);
  tremoloWidth = (tremoloWidthReading / (float) kAdcMax);

  now = millis();
  if((now - lastPrintTime > 100))
  {
    lastPrintTime = now;
    Serial.print("volumeLevel:");
    Serial.print(volumeLevel);
    Serial.print(",");
    Serial.print("gain:");
    Serial.print(gain);
    Serial.print(",");
    Serial.print("distortionLevel:");
    Serial.print(distortionLevel);
    Serial.print(",");
    Serial.print("chorusDepth:");
    Serial.print(chorusDepth);
    Serial.print(",");
    Serial.print("tremoloFreq:");
    Serial.print(tremoloFreq);
    Serial.print(",");
    Serial.print("tremoloDepth:");
    Serial.print(tremoloDepth);
    Serial.print(",");
    Serial.print("tremoloWidth:");
    Serial.println(tremoloWidth);
  }
}
