#include "DaisyDuino.h"
#include "math.h"
#include "fmchorus.h"
#include "cubicnl.h"
#include "tremor.h"

using namespace idfk;

/* Hardware Setup */
DaisyHardware hw;
size_t numChannels;

/* Debugging Settings */
const bool kDebug = true;

/* Analog Reading Settings */
const int kAdcBits = 12;
const long kAdcMax = 4095;

/* Fequency Range: A1-A7 */
const float kCarrierFrequencyLow = 55.0;
const float kCarrierFrequencyHigh = 3520.0;

/* Pin Mappings */
const int kModulationDepthPin = A9;
const int kDistortionLevelPin = A0;
const int kChorusDepthPin = A8;
const int kTremoloSpeedPin = D14;
const int kTremoloOnPin = D13;

const int kFilterFreqPin = A3;
const int kFilterResonancePin = A6;
const int kNoiseLevelPin = A2;
const int kReverbLevelPin = A7;
const int kVolumePin = A1;

const int kForceSensorPin = A4;
const int kSoftPotPin = A5;

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
static boolean tremoloOn;
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
static MoogLadder filter;
static float filterFreq;
static long filterFreqReading;
static Port filterFreqPortamento;
static float filterRes;
static long filterResReading;
static Port filterResPortamento;
static float filterScalar;

const float kFilterFreqLow = 20.0f;
const float kFilterFreqHigh = 24000.0f;

/* Reverb Settings */
static ReverbSc reverb;
static Port reverbLevelPortamento;
static float reverbLevel;
static long reverbLevelReading;
static CrossFade crossfadel, crossfader;

/* Noise Settings */
static WhiteNoise noise;
static Port noiseLevelPortamento;
static float noiseLevel;
static long noiseLevelReading;

void MyCallback(float **in, float **out, size_t size) 
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
} // End MyCallback(float **in, float **out, size_t size)


float normalizeSoftpot(long sensorValue) 
{
  long value;
  float normalizedValue;
  value = constrain(sensorValue, kSoftPotThreshold, kAdcMax);
  value = map(value, kSoftPotThreshold, kAdcMax, 0, kAdcMax);
  normalizedValue = (float)value/(float)kAdcMax;
  return normalizedValue;
} // End normalizeSoftpot(long sensorValue)


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
} // End normalizeForceSensor(long sensorValue)


void setup() {
  float sampleRate;
  /* harware initialization */
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  numChannels = hw.num_channels;
  sampleRate = DAISY.get_samplerate();

  analogReadResolution(kAdcBits);
  pinMode(kTremoloSpeedPin, INPUT);
  pinMode(kTremoloOnPin, INPUT);

  if(kDebug)
  {
    Serial.begin(9600);
  }

  /* FM Sythesis Initialization */
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

  /* Tremolo Intialization */
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

  DAISY.begin(MyCallback);
}  // End setup()


void loop() {
  unsigned long lastPrintTime = 0;
  unsigned long now;

  /* Oscillator Gain and Volume */
  volumeReading = analogRead(kVolumePin);
  volumeLevel = volumeReading / (float) kAdcMax;

  gainReading = analogRead(kForceSensorPin);
  gain = normalizeForceSensor(gainReading);

  carrierFreqReading = analogRead(kSoftPotPin);
  carrierFreq = pow(2.0, normalizeSoftpot(carrierFreqReading) * carrierScalar) * kCarrierFrequencyLow;

  modulatorDepthReading = analogRead(kModulationDepthPin);
  modulatorDepth = modulatorDepthReading  / (float) kAdcMax;

  //modulatorRatioReading = analogRead(kModulationRatioPin); 
  //modulatorRatio = (modulatorRatioReading / (float) kAdcMax) + 1.0f;

  chorusDepthReading = analogRead(kChorusDepthPin);
  chorusDepth = (chorusDepthReading / (float) kAdcMax);

  /* Tremolo Readings - 2 switch controls */
  tremoloOnReading = digitalRead(kTremoloOnPin);
  tremoloSpeedReading = digitalRead(kTremoloSpeedPin);
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
  distortionLevelReading = analogRead(kDistortionLevelPin);
  distortionLevel = (distortionLevelReading / (float) kAdcMax);


/* Filter Readings */

  filterFreqReading = analogRead(kFilterFreqPin);
  filterFreq = pow(2.0, (filterFreqReading / (float) kAdcMax) * filterScalar) * kFilterFreqLow;
  
  filterResReading = analogRead(kFilterResonancePin);
  filterRes = (filterResReading / (float) kAdcMax);

/* Reverb Readings */
  reverbLevelReading = analogRead(kReverbLevelPin);
  reverbLevel = reverbLevelReading / (float) kAdcMax;

  /* Noise Reading */
  noiseLevelReading = analogRead(kNoiseLevelPin);
  noiseLevel = noiseLevelReading / (float) kAdcMax;

  if(kDebug)
  {
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
      
      Serial.print("modulatorDepth:");
      Serial.print(modulatorDepth);
      Serial.print(",");
      
      Serial.print("modulatorRatio:");
      Serial.print(modulatorRatio);
      Serial.print(",");
      
      Serial.print("chorusDepth:");
      Serial.print(chorusDepth);
      Serial.print(",");
      
      Serial.print("tremoloOnReading:");
      Serial.print(tremoloOnReading);
      Serial.print(",");
      
      Serial.print("tremoloSpeedReading:");
      Serial.print(tremoloSpeedReading);
      Serial.print(",");
      
      Serial.print("filterFreq:");
      Serial.print(filterFreq);
      Serial.print(",");

      Serial.print("distortionLevel:");
      Serial.print(distortionLevel);
      Serial.print(",");

      Serial.print("reverbLevel:");
      Serial.print(reverbLevel);
      Serial.print(",");

      Serial.print("noiseLevel:");
      Serial.print(noiseLevel);
      Serial.print(",");

      Serial.print("filterFreq:");
      Serial.print(filterFreq);
      Serial.print(",");

      Serial.print("carrierFreq:");
      Serial.print(carrierFreq);
      Serial.println("");
    }
  }

} // End loop()
