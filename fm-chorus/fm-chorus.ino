#include "DaisyDuino.h"
#include "math.h"
#include "fmchorus.h"

using namespace idfk;

DaisyHardware hw;
size_t numChannels;

const int kAdcBits = 12;
const long kAdcMax = 4095;

// A1 - A7
const float kCarrierFrequencyLow = 55.0;
const float kCarrierFrequencyHigh = 3520.0;

const int kModulationDepthPin = A9;
const int kModulationRatioPin = A0;
const int kChorusDepthPin = A8;  // normally A8
const int kSw1Pin = D14;
const int kSw2Pin = D13;
const int kVolumePin = A1;
const int kForceSensorPin = A4;
const int kSoftPotPin = A5;

/* Envelope, Gain, Volume */
static Adsr envelope;

static Port gainPortamento;
static bool gate = false;
static long gainReading;
static float gain;

static long volumeReading;
static Port volumePortamento;
static float volumeLevel;

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

/* Sensor calibration settings for lower threshold */
const long kForceSensorThreshold = 20;
const float kGateThreshhold = 0.1;
const long kSoftPotThreshold = 44;

static float freq_scalar;
static float osc;

void MyCallback(float **in, float **out, size_t size) 
{
  float slewedVolumeLevel, slewedGain; 
  float slewedCarrierFreq, slewedModulatorDepth, slewedModulatorRatio;
  float slewedChorusDepth;
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

      slewedChorusDepth = chorusDepthPortamento.Process(chorusDepth);

      fmOsc.SetAmp(envelopeOut);
      fmOsc.SetRatio(slewedModulatorRatio);
      fmOsc.SetDepth(slewedModulatorDepth);
      fmOsc.SetFreq(slewedCarrierFreq);
      fmOsc.SetWidth(slewedChorusDepth);
      sig = fmOsc.Process();
      osc = sig * slewedVolumeLevel;

      out[0][i] = sig * slewedVolumeLevel;
      out[1][i] = sig * slewedVolumeLevel;
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

void setup() {
  float sampleRate;

  analogReadResolution(kAdcBits);
  pinMode(kSw1Pin, INPUT);
  pinMode(kSw2Pin, INPUT);

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

  chorusDepth = 0.0f;
  carrierPortamento.Init(sampleRate, 0.01f);
  modulatorDepthPortamento.Init(sampleRate, 0.01);
  modulatorRatioPortamento.Init(sampleRate, 0.01);
  chorusDepthPortamento.Init(sampleRate, 0.01);

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
} // end setup()

unsigned long lastPrintTime = 0;
unsigned long now = millis();

void loop() {
  // put your main code here, to run repeatedly:
  volumeReading = analogRead(kVolumePin);
  volumeLevel = volumeReading / (float) kAdcMax;

  gainReading = analogRead(kForceSensorPin);
  gain = normalizeForceSensor(gainReading);

  carrierFreqReading = analogRead(kSoftPotPin);
  float freq_scalar = normalizeSoftpot(carrierFreqReading);
  carrierFreq = pow(2.0, normalizeSoftpot(carrierFreqReading) * carrierScalar) * kCarrierFrequencyLow;

  modulatorDepthReading = analogRead(kModulationDepthPin);
  modulatorDepth = modulatorDepthReading  / (float) kAdcMax;

  modulatorRatioReading = analogRead(kModulationRatioPin); 
  modulatorRatio = (modulatorRatioReading / (float) kAdcMax) + 1.0f;

  chorusDepthReading = analogRead(kChorusDepthPin);
  chorusDepth = (chorusDepthReading / (float) kAdcMax);

  now = millis();
  if((now - lastPrintTime > 100))
  {
    lastPrintTime = now;
 //   Serial.print("carrierFreq:");
 //   Serial.print(carrierFreq);
 //   Serial.print(",");
    Serial.print("osc:");
    Serial.print(osc);
    Serial.print(",");
    Serial.print("volumeLevel:");
    Serial.print(volumeLevel);
    Serial.print(",");
    Serial.print("freq_scalar:");
    Serial.print(freq_scalar);
    Serial.print(",");
    Serial.print("gain:");
    Serial.print(gain);
    Serial.print(",");
    Serial.print("modulatorDepth:");
    Serial.print(modulatorDepth);
    Serial.print(",");
    Serial.print("modulatorRatio:");
    Serial.print(modulatorRatio);
    Serial.print(",");
    Serial.print("chorusDepth:");
    Serial.println(chorusDepth);
  }

} // end loop()
