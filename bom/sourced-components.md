# Sourced Components — Absonus

Components not included in the JLCPCB PCB assembly BOM. These are sourced separately and installed by hand.

| Description | Source / Link |
|:--|:--|
| TWTADE 10kΩ Potentiometer (RK097N-3) | [Amazon](https://www.amazon.com/TWTADE-Amplifier-Potentiometers-Knurled-RK097N-3-10K/dp/B07XQ1Q9RN?th=1) |
| ElectroCookie Solderable Breadboard | [Amazon](https://www.amazon.com/dp/B07YBYZCTN) |
| Ancable 1/8" 3.5mm TRS Stereo Panel Mount Audio Jack (5-pack) | [Amazon](https://www.amazon.com/dp/B07JNC4P7Y) |
| uxcell 2-Position 3P SPDT Panel Mount Micro Slide Switch | [Amazon](https://www.amazon.com/dp/B01NCWDJ18) |
| CESS 5.5×2.1mm 5A DC Power Jack Socket | [Amazon](https://www.amazon.com/dp/B07FK6MMGH) |
| Bud Industries Aluminum Enclosure 7.39"L × 4.7"W × 1.5"H | [Amazon](https://www.amazon.com/dp/B005T7R82W) |
| LS18-P 3-Channel NoBounce IC (PDIP) | [LogiSwitch](https://logiswitch.com/shop/nobounce-ics/ls18-p/) |
| Force Sensor FSR Model 408 (200mm length) | [Interlink Electronics](https://buyinterlinkelectronics.com/collections/new-standard-force-sensors/products/fsr-model-408-200mm-length) |
| SoftPot Linear Potentiometer (150mm) | [Spectra Symbol](https://store.spectrasymbol.com/products/softpot-linear-potentiometer?variant=40669007904883) |

## Notes

- The 10kΩ potentiometers were chosen for their low profile; they are soldered to a 10-wire ribbon cable with color-coded conductors matching the panel layout (see `docs/design-notes.md`).
- The FSR is 200mm and is cut down to match the usable 150mm length of the SoftPot.
- A 3kΩ pulldown resistor is required on the FSR input (A4/D19) and a 10kΩ pulldown on the SoftPot input (A5/D20).
