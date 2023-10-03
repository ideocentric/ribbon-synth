#include "DaisyDuino.h"
#include "math.h"
#include "tremor.h"

using namespace idfk;

const int kAdcBits = 12;
const long kAdcMax = 4095;

DaisyHardware hw;
size_t numChannels;

// A1 - A7
const float kCarrierFrequencyLow = 55.0;
const float kCarrierFrequencyHigh = 3520.0;

const int kTremoloFreqPin = A6;
const int kTremoloDepthPin = A2;
const int kTremoloWidthPin = A7;

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

const float kTremoloFreqLow = 1.0f;
const float kTremoloFreqHigh = 13.0f;

static Oscillator saw;
const float kOscillatorFreqLow = 55.0f;

static float left, right;

void MyCallback(float **in, float **out, size_t size) 
{
    float slewedTremoloFreq, slewedTremoloDepth, slewedTremoloWidth;
    float trem, treml, tremr;
    float sig, sigl, sigr;
    float osc;

   for (size_t i = 0; i < size; i++) 
   {
      slewedTremoloFreq = tremoloFreqPortamento.Process(tremoloFreq);
      slewedTremoloDepth = tremoloDepthPortamento.Process(tremoloDepth);
      slewedTremoloWidth = tremoloWidthPortamento.Process(tremoloWidth);
      tremolo.SetFreq(slewedTremoloFreq);
      tremolo.SetDepth(slewedTremoloDepth);
      tremolo.SetWidth(slewedTremoloWidth);
      trem = tremolo.Process();
      treml = tremolo.GetLeft();
      left = treml;
      tremr = tremolo.GetRight();
      right = tremr;
      saw.SetAmp(0.707f);
      saw.SetFreq(kOscillatorFreqLow);

      osc = saw.Process();
      sig = osc * trem;
      sigl = sig * treml;
      sigr = sig * tremr;

      out[0][i] = sigl;
      out[1][i] = sigr;
   }
} // end MyCallback
void setup() {
  float sampleRate;
  analogReadResolution(kAdcBits);
  Serial.begin(9600);

  // Initialize for Daisy pod at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  numChannels = hw.num_channels;
  sampleRate = DAISY.get_samplerate();


  tremoloScalar = log2(kTremoloFreqHigh / kTremoloFreqLow);
  tremolo.Init(sampleRate);
  tremolo.SetDepth(0.5f);
  tremolo.SetFreq(kTremoloFreqLow);
  tremolo.SetWidth(0.5f);
  tremoloFreqPortamento.Init(sampleRate, 0.01);
  tremoloDepthPortamento.Init(sampleRate, 0.01);
  tremoloWidthPortamento.Init(sampleRate, 0.01);

  saw.Init(sampleRate);
  saw.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw.SetAmp(0.707f);
  saw.SetFreq(kOscillatorFreqLow);

  DAISY.begin(MyCallback);
}

void loop() {
  unsigned long lastPrintTime = 0;
  unsigned long now;
  
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
    Serial.print("left:");
    Serial.print(left);
    Serial.print(",");
    Serial.print("right:");
    Serial.println(right);
  }

}
