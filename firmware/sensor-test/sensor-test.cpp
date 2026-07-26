// sensor-test — reads every panel input and streams normalized values over USB
// serial. Flash this immediately after assembly to confirm each pot, switch,
// force sensor, and soft-pot is wired to the right pin before loading the main
// firmware. View with a serial terminal or the Arduino Serial Plotter.
//
// Ported from the DaisyDuino sketch to bare-metal libDaisy. Pin labels are the
// physical wire colors on the panel harness (see docs/firmware-architecture.md).

#include "daisy_seed.h"

using namespace daisy;
using namespace daisy::seed;

static DaisySeed hw;

// ADC channels, indexed by wire color. Order here defines the GetFloat() index.
enum AdcChannel
{
    CH_WHITE = 0, // A0
    CH_PURPLE,    // A1
    CH_GREEN,     // A2
    CH_ORANGE,    // A3
    CH_YELLOW,    // A4
    CH_BLUE,      // A5
    CH_GRAY,      // A6
    CH_BLACK,     // A7
    CH_FORCE,     // A8 — force-sensitive resistor
    CH_SOFTPOT,   // A9 — ribbon soft-pot
    ADC_CHANNELS
};

int main(void)
{
    hw.Init();

    // Configure all ten analog inputs.
    AdcChannelConfig adc[ADC_CHANNELS];
    adc[CH_WHITE].InitSingle(A0);
    adc[CH_PURPLE].InitSingle(A1);
    adc[CH_GREEN].InitSingle(A2);
    adc[CH_ORANGE].InitSingle(A3);
    adc[CH_YELLOW].InitSingle(A4);
    adc[CH_BLUE].InitSingle(A5);
    adc[CH_GRAY].InitSingle(A6);
    adc[CH_BLACK].InitSingle(A7);
    adc[CH_FORCE].InitSingle(A8);
    adc[CH_SOFTPOT].InitSingle(A9);
    hw.adc.Init(adc, ADC_CHANNELS);
    hw.adc.Start();

    // Two tremolo toggle switches (digital inputs, matching the original sketch's
    // floating INPUT mode).
    GPIO sw1, sw2;
    sw1.Init(D13, GPIO::Mode::INPUT, GPIO::Pull::NOPULL); // SW2 (tremolo speed)
    sw2.Init(D14, GPIO::Mode::INPUT, GPIO::Pull::NOPULL); // SW1 (tremolo on/off)

    hw.StartLog(false); // USB serial; do not block waiting for a host

    while(1)
    {
        hw.PrintLine(
            "gray:%.2f,white:%.2f,black:%.2f,sw1:%d,sw2:%d,purple:%.2f,"
            "blue:%.2f,green:%.2f,yellow:%.2f,orange:%.2f,force:%.2f,softpot:%.2f",
            hw.adc.GetFloat(CH_GRAY),
            hw.adc.GetFloat(CH_WHITE),
            hw.adc.GetFloat(CH_BLACK),
            sw1.Read() ? 1 : 0,
            sw2.Read() ? 1 : 0,
            hw.adc.GetFloat(CH_PURPLE),
            hw.adc.GetFloat(CH_BLUE),
            hw.adc.GetFloat(CH_GREEN),
            hw.adc.GetFloat(CH_YELLOW),
            hw.adc.GetFloat(CH_ORANGE),
            hw.adc.GetFloat(CH_FORCE),
            hw.adc.GetFloat(CH_SOFTPOT));
        System::Delay(100);
    }
}