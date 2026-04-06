# Design Notes

System has 3 potentiometers and 2 toggle switches in the top row.  Second row contains 5 potentiometers.  The third row has a a pressure sensor with a soft potentiometer layered on top of pressure sensor.  Layout of pots, switches and strips are as follows:

Row 1:

| Modulation | Distortion | Chorus Width |  Tremolo Speed| Tremolo On |
|:--|:--|:--|:--|:--|
| 10 - Black | 9 - White | 8 - Gray | SW1 | SW2 |
| A9 | A0 | A8 | D14 | D13 |

Row 2:

| Filter Cutoff | Filter Resonance | Noise | Reverb | Volume |
|:--|:--|:--|:--|:--|
| 3 - Orange | 4 - Yellow | 5 - Green | 6 - Blue | 7 - Purple |
| A3 | A6 | A2 | A7 | A1 |

Touch Sensors:

| Frequency | Pressure |
|:--|:--|
| Soft Pot | Gain |
| A5 | A4 |

Tremolo switches function like a Leslie speaker.  When enabled the speed of the tremolo ramps up to the current speed selection.  The target frequency, depth and stereo width is indicated in the table below:

| Description | Rate | Depth | Width |
|:--|:--|:--|:--|
| Low | 1.0 Hz | 0.5 | 0.66 |
| High | 13 Hz | 0.75 | 0.83 |

Stereo panning is mapped at half the frequency of the tremolo resulting in alternating direction in panning.  The effect is not meant to directly mimic a Leslie speaker, but to simply provide a textural similarity.

Daisy Seed Pin Mappings:

| Pin | Name | Type | Mapping | Notes |
|:-- |:--|:--|:--|:--|
|1 | D0 |  |  |
|2 | D1 |  |  |
|3 | D2 |  |  |
|4 | D3 |  |  |
|5 | D4 |  |  |
|6 | D5 |  |  |
|7 | D6 |  |  |
|8 | D7 |  |  |
|9 | D8 |  |  |
|10 | D9 |  |  |
|11 | D10 |  |  |
|12 | D11 |  |  |
|13 | D12 |  |  |
|14 | D13 | | SW2 |  |
|15 | D14 | | SW1 |  |
|16 | in[0][SIZE] |  |  |
|17 | in[1][SIZE] |  |  |
|18 | out[0][SIZE] |  |  |
|19 | out[1][SIZE] |  |  |
|20 | AGND |  |  |
|21 | 3v3 Analog |  |  |
|22 | A0/D15 | ADC0 | White |  |
|23 | A1/D16 | ADC1 | Purple |  |
|24 | A2/D17 | ADC2 | Green |  |
|25 | A3/D18 | ADC3 | Orange |  |
|26 | A4/D19 | ADC4 | Pressure | 3k Ohm Pulldown |
|27 | A5/D20 | ADC5 | Ribbon | 10k Ohm Pulldown |
|28 | A6/D21 | ADC6 | Yellow |  |
|29 | A7/D22 | ADC7/DAC OUT 2 | Blue |  |
|30 | A8/D23 | ADC8/DAC OUT 1 | Gray |  |
|31 | A9/D24 | ADC9 | Black |  |
|32 | A10/D25 | ADC10 |  |  |
|33 | D26 |  |  |  |
|34 | D27 |  |  |  |
|35 | A11/D28 |  |  |  |
|36 | D29 |  |  |  |
|37 | D30 |  |  |  |
|38 | 3v3 Digital |  |  |  |
|39 | VIN |  | +4v - +17v |  |
|40 | DGND |  |  |   | 


Some of the components were challenging to find or were too large for the desired space.  The following list of items are some of the key components required to make this work.  The particular potentiometers were chosen due to their low profile.  The are soldered to a 10 wire ribbon cable.  The colors refer to the matching cable color.

The pressure sensors and soft potentiometers do not match in the available sizes.  In this design I needed a 150mm Soft pot.  Since the Pressure Sensors can be cut down, text available size was 200mm which is cut down to match the usable length of the Soft Pot. 


| Description | Link |
|:--|:--|
| TWTADE 10k Ω Potentiometer | https://www.amazon.com/TWTADE-Amplifier-Potentiometers-Knurled-RK097N-3-10K/dp/B07XQ1Q9RN?th=1 |
| ElectroCookie Solderable Breadboard  | https://www.amazon.com/dp/B07YBYZCTN?ref=ppx_yo2ov_dt_b_product_details&th=1 |
| Ancable 5-Pack 1/8" 3.5mm TRS Stereo Panel Mount Audio AUX Jack | https://www.amazon.com/dp/B07JNC4P7Y?psc=1&ref=ppx_yo2ov_dt_b_product_details |
| uxcell 2 Position 3P SPDT Panel Mount Micro Slide Switch  | https://www.amazon.com/dp/B01NCWDJ18?psc=1&ref=ppx_yo2ov_dt_b_product_details |
| CESS 5.5 x 2.1 mm 5A DC Power Jack Socket  | https://www.amazon.com/dp/B07FK6MMGH?psc=1&ref=ppx_yo2ov_dt_b_product_details |
| Bud Industries Aluminum Enclosure 7.39” L x4.7” W x 1.5” H | https://www.amazon.com/dp/B005T7R82W?psc=1&ref=ppx_yo2ov_dt_b_product_details |
| LS18-P 3-Channel NoBounce IC – PDIP | https://logiswitch.com/shop/nobounce-ics/ls18-p/ |
| Force Sensor FSR Model 408 (200mm length) | https://buyinterlinkelectronics.com/collections/new-standard-force-sensors/products/fsr-model-408-200mm-length |
| Soft Pot (150mm) | https://store.spectrasymbol.com/products/softpot-linear-potentiometer?variant=40669007904883 |



The general design of the instrument is based on a simple FM synthesis.  The module uses 3 carriers and modulator pairs.  The carrier wave form is a sawtooth wave while the modulator is a triangle wave.  The synthesis module is designed to have the ration of the modulator to carrier as configurable, however, in this design it is set to an equal-tempered tritone.  The depth extends the modulation from 0 - the carrier frequency.  The “Chorus Width” is essentially a 3 voice detuning from no detuning  to a full equal-tempered semi-tone.  The upper and lower voices are at a slightly lower amplitude to retain the center frequencies dominance.  The detuned components are a full FM carrier/modulator pair.

Distortion is implemented using Julius O. Smith’s cubicnl.dsp algorithm from Faust ported to the Daisy Seed here.

The Moog Ladder Filter, White Noise and the SCReverb are standard Daisy Seed modules are used here without modification.

One of the initial challenges was to evaluate and map the Soft Pot and Force Sensors.  The Soft Pot is a bit noisy when idle and the full reading needs to be mapped to a usable range.  Additionally, the gate release locks the last set frequency  in order to prevent a fall off to the lowest frequency.  This allows for extending the release time of an envelope.