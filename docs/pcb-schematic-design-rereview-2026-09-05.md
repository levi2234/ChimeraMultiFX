# ChimeraMultiFX PCB and Schematic Design Rereview

**Date:** 2026-09-05  
**Design:** `hardware/boards/daisyBoard.kicad_sch`, `hardware/boards/daisyBoard.kicad_pcb`, and `hardware/boards/daisyBoard.kicad_pro`  
**Scope:** Electrical schematic, PCB layout, return paths, EMC risk, and Daisy Seed integration. BOM, sourcing, Gerbers, and release-output checks are intentionally excluded.

## Verdict

**No-go in the current state.** The Daisy Seed signal mapping is coherent and the newly added ground stitching fixes the previous UART return-path concern. However, the new REF2033 capacitors are wired in series rather than from each rail to ground. This leaves U4 IN/EN without a DC supply and isolates both reference outputs from VREF and VBIAS. The analog bias system therefore cannot operate as drawn.

After U4 is corrected, the main remaining design risks are the schematic/PCB ground-domain mismatch, expression ADC over-range margin, unsynchronized C40/C41 values, and distant return vias on several SD and expression ADC transitions. UART length is not a blocker at 115200 baud.

## Priority Findings

| Priority | Finding | Consequence | Required action | Evidence |
|---|---|---|---|---|
| **Urgent** | C43 is in series between VCC_Daisy and tied U4 pins 3/4 | U4 has no DC input supply and cannot be reliably enabled | Connect pin 4 directly to VCC_Daisy; connect pin 3 to a defined high level for always-on use; connect C43 from VIN to GND | Raw schematic and PCB |
| **Urgent** | C31 and C42 are in series with U4 VBIAS and VREF outputs | Both output rails are DC-isolated from their loads | Connect pin 1 directly to VBIAS and pin 5 directly to VREF; connect C31 and C42 from their respective outputs to GND | Raw schematic and PCB |
| **High** | Schematic uses GND, GNDA, and GNDD while the PCB contains only GND | The intended return architecture is not encoded consistently and a future schematic-to-PCB update can alter connectivity | Use one GND net throughout for the present continuous-plane layout, or add a deliberate net tie if separation is actually required | Raw-file comparison |
| **High** | Expression divider produces 3.333 V from a 5 V input | Nominal voltage already exceeds a nominal 3.3 V ADC rail before tolerances and transients | Lower the divider ratio and add suitable input clamps/transient protection | Raw schematic and calculation |
| **High** | C40/C41 are 10 nF in the schematic but 1 nF on the PCB | The intended expression ADC filter is ambiguous by a factor of ten | Select the intended value and update the PCB from the schematic | Deterministic cross-analysis |
| **Medium** | Several SD and expression ADC layer transitions lack a nearby GND return via | Enlarged return loops can increase susceptibility, emissions, and edge ringing | Add a GND via near each listed transition or keep the route on one layer | PCB coordinate analysis |
| **Medium** | VBIAS is 91.54 mm long and uses six vias | A noise-sensitive analog reference is distributed over a long, multi-layer route | After fixing U4, shorten where practical and add local return support at every transition | PCB route analysis |
| **Low** | R8 pad 2 still contains an untented ordinary through-via | Solder wicking can weaken the pad joint | Move the via outside the pad or specify an appropriate filled/capped process | Raw PCB and analyzer |

## Changes Since the Prior Review

The live schematic gained C31, C42, and C43, all 100 nF. The PCB gained those three footprints, nine track segments, nine vias, and about 10.04 mm of routing.

| Change | Assessment |
|---|---|
| Three U4 capacitors added | Placement is close to U4, but all three are wired in series and must be rewired as shunt capacitors to GND |
| Nine vias added | UART return stitching is now good: all four signal transitions are about 0.81 mm from a GND via |
| C40/C41 changed from 1 nF to 10 nF in schematic | PCB values were not updated and still read 1 nF |
| Via-in-pad cleanup | J2, R46, and R50 cases are gone; only R8 pad 2 remains reported |
| Routing | Board remains fully routed with zero analyzer-unrouted nets |

## REF2033 Circuit

The prior review identified missing local bypassing, and the new capacitors appear intended to address it. Their placement is reasonable: C31, C42, and C43 are approximately 1.56 mm, 1.94 mm, and 2.64 mm from U4. Their connectivity is not.

### Present Connectivity

```text
VCC_Daisy -- C43 -- U4 pin 4 IN
                       |
                       `-- U4 pin 3 EN

U4 pin 1 VBIAS -- C31 -- VBIAS
U4 pin 5 VREF  -- C42 -- VREF
U4 pin 2 GND ----------- GND
```

A capacitor in each series path blocks DC. The correct always-on arrangement is:

```text
VCC_Daisy -------- U4 pin 4 IN
     |              U4 pin 3 EN
    C43
     |
    GND

U4 pin 1 VBIAS ---- VBIAS
     |
    C31
     |
    GND

U4 pin 5 VREF ----- VREF
     |
    C42
     |
    GND
```

This correction must be made in the schematic first, followed by Update PCB from Schematic and rerouting. The REF2033 behavior cited here is based on the TI SBOS600E manual review from the preceding design pass; the raw live files establish the present wiring fault.

## Daisy Seed Integration

The Daisy mapping was checked against the local official-derived Rev4 pinout CSV, the local Daisy symbol, and the current manufacturer documentation links. The provided PDF itself was not independently ingested in this pass.

| Daisy physical pins | Function | Live net | Assessment |
|---|---|---|---|
| 2-7 | SDMMC1 D3, D2, D1, D0, CMD, CK | SD_DAT_3_DAISY through SD_CLK_DAISY | Correct |
| 14 | D13 / PB6 / USART1_TX | DAISY_TO_ESP | Correct |
| 15 | D14 / PB7 / USART1_RX | ESP_TO_DAISY | Correct |
| 16-19 | Audio input L/R and output L/R | AUDIO_IN_1/2 and AUDIO_OUT_1/2 | Correct |
| 20 | AGND | GNDA in schematic, GND on PCB | Electrically unified, naming should be made explicit |
| 21 | 3V3_A | +3.3VA | Correct |
| 22-23 | D15/A0 and D16/A1 | ADC_EXP_IN_2 and ADC_EXP_IN_1 | Correct |
| 38 | 3V3_D | +3.3VD | Correct |
| 39 | VIN | VCC_Daisy | Correct |
| 40 | GND/DGND | GNDA in schematic, GND on PCB | Electrically unified, but pin function is mislabeled by the schematic net choice |

J15 and J16 use logical breakout numbering for the two 20-pin sides. Their apparent reversal relative to the physical J13/J14 footprint orientation is intentional. No UART direction swap was found.

The official documentation identifies the Seed as a 96 kHz/24-bit stereo audio platform with native SDMMC1 on pins 2-7, twelve 16-bit ADC channels, and two 12-bit DAC channels. The chosen interfaces use valid native functions.

## Analog Design

### Audio Paths

The existing audio topology remains broadly coherent:

- Input midband attenuation is approximately $4.7/(3.3+4.7)=0.5875$ before the following nominal -1 V/V stage.
- The 10 kOhm/330 pF feedback pole and 100 Ohm/33 nF ADC pole are each approximately 48.2 kHz.
- Output gain is approximately $-33/15=-2.2$ V/V.

These are plausible values for a guitar-band interface, but clipping margin is not established. The op amps use 5 V while audio is centered on 1.65 V, and the output stage applies 2.2x gain. Measure maximum undistorted input and output levels into the intended loads after VBIAS is restored. External audio connectors also remain inference-only ESD/transient risks because exact protection requirements were not provided.

### Expression Inputs

The 10 kOhm/20 kOhm divider gives:

$$
V_{ADC}=5\text{ V}\frac{20\text{k}}{10\text{k}+20\text{k}}=3.333\text{ V}
$$

This should be reduced below the ADC rail with tolerance margin. C40/C41 are shunt capacitors after a Thevenin resistance of approximately 6.67 kOhm. The two possible filter corners are:

$$
f_c(1\text{ nF})\approx23.9\text{ kHz},\qquad
f_c(10\text{ nF})\approx2.39\text{ kHz}
$$

For an expression pedal, 10 nF is the more useful of those two noise-filtering choices, but the final value should follow the intended control bandwidth and ADC acquisition behavior. Also address the 50 mA short demand created by a grounded 5 V expression ring through 100 Ohm; raise the series resistance or use a current-limited drive if insertion shorts are expected.

## PCB Layout and Return Paths

The live PCB is a 100 mm x 100 mm four-layer board with 141 footprints, 118 PCB nets, 808 track segments, 136 vias, five zones, 2,041.67 mm total track length, and zero analyzer-unrouted nets. In1.Cu contains a broad continuous GND plane with approximately 93.7% fill.

### Critical Route Metrics

| Interface | Route lengths | Transition assessment |
|---|---:|---|
| UART | 76.14 mm and 84.48 mm | Good; all four transitions have a GND via about 0.81 mm away |
| Daisy SDMMC CLK/CMD | 41.70 mm / 44.62 mm | One CLK transition is 7.40 mm from GND; one CMD transition is 11.26 mm |
| Daisy SDMMC D0-D3 | 34.47 / 32.22 / 40.12 / 34.23 mm | Worst transition distances are about 11.23, 11.16, 8.06, and 6.46 mm |
| ESP SD SPI SCK | 17.87 mm | Good at 0.81/1.09 mm |
| ESP SD SPI MOSI | 29.41 mm | One transition is about 11.25 mm from GND |
| ESP SD SPI MISO/CS | 20.95 / 27.28 mm | Approximately 1.28/0.81 mm and 0.81/1.06 mm |
| Expression ADC 1 | 72.60 mm | One transition is about 20.90 mm from GND |
| Expression ADC 2 | 51.64 mm | One transition is about 17.71 mm from GND |
| Audio input/output channel 2 | 61.82 / 67.97 mm | Worst distances are about 3.23 and 2.15 mm |
| VBIAS | 91.54 mm, six vias | Long and multi-layer; improve locally after U4 is fixed |

The UART trace length is acceptable at 115200 baud. Do not reroute it solely to make it shorter; preserve the newly added transition stitching. Optional source-series damping footprints may be useful only if bench measurements show edge ringing.

Several SD nets use 0.127 mm neck-downs. This is not automatically an electrical failure, but 0.20-0.254 mm is preferable where escape geometry permits. The project still defines only a permissive Default net class, so create explicit SD, analog, power, and ordinary-signal classes to preserve routing intent.

### Plane Interpretation

Cross-analysis reports two VCC_Daisy islands and labels SD signals as crossing a VCC_Daisy plane gap. This does not prove a broken GND return path:

- In1.Cu has one board-spanning GND region.
- The VCC_Daisy zone is on In2.Cu and reports one filled region despite the connectivity-island warning.
- The PCB has zero analyzer-unrouted nets.

The electrical power-island report is therefore **unconfirmed** pending a native zone refill and DRC. It should not be treated as a proven open circuit. There is still a local return-path concern for B.Cu traces because In2.Cu is their adjacent plane; route over continuous copper where practical and add the return vias listed above.

## Ground Architecture

A continuous physical GND plane is appropriate for this mixed-signal board if placement and return-current geometry are controlled. The problem is the schematic representation, not the choice of a unified plane. The schematic contains GND, GNDA, and GNDD power symbols, while the PCB has no GNDA or GNDD nets.

Recommended correction:

1. Rename all grounds to GND if the board is intended to use one plane.
2. Preserve the Daisy pin descriptions as AGND for physical pin 20 and GND/DGND for pin 40 in documentation.
3. If separate domains are truly required, add an explicit net tie and verify all return crossings; do not rely on implicit PCB net merging.

## Reviewer Overrides

- J6-J11 are intentional schematic-only off-board jack representations. They explicitly use `on_board=no`, `in_bom=no`, and blank footprints; the cross-domain missing-component warning is dismissed.
- Card1/Card2 shell tabs were misclassified as thermal pads. Their thermal-via warnings are dismissed.
- The 89 EMC findings and 31/100 score are a heuristic risk screen, not a compliance verdict. Plane-coverage duplicates are condensed into the measured transition findings above.
- Generic decoupling-to-via warnings associate several filter capacitors with unrelated ICs and are not accepted individually.
- Outer-layer SD clocks are not automatically defective. Continuous reference copper and transition stitching are the actionable criteria.
- Card1 and J15 edge-clearance warnings require enclosure and connector-overhang confirmation; they are not automatically electrical faults.

## Analysis Basis and Limits

### Performed

- Fresh `analyze_schematic.py` run: 135 components, 122 nets, and 445 wires.
- Fresh `analyze_pcb.py --full --proximity` run: 141 footprints, 118 nets, 808 segments, 136 vias, and zero unrouted nets.
- Fresh cross-domain, EMC, and thermal analyzer runs.
- Schematic and PCB delta analysis against the earlier 2026-09-05 run.
- Raw-file checks of U4 and C31/C42/C43 connectivity, Daisy mapping, ground naming, project rules, critical route lengths, transition coordinates, and GND-via distances.
- Daisy mapping comparison against the local official-derived Rev4 pinout, local symbol, and current manufacturer documentation links.

### Not Performed / Review Limits

- BOM, sourcing, lifecycle, Gerber, drill, placement-file, and release-output analysis were excluded as requested.
- Native KiCad ERC and DRC were not run because `kicad-cli` is unavailable. Refill zones and run both in KiCad after the schematic correction.
- SPICE was not run because ngspice, LTspice, and Xyce are unavailable.
- Thermal analysis ran but assessed no devices because quantifiable dissipation/package data is unavailable; zero thermal findings is not a thermal validation.
- Broad per-component datasheet verification was not possible. Daisy integration uses official pinout evidence; REF2033 details rely on the prior TI datasheet review; other electrical ratings remain consistency or plausibility checks.
- Static review cannot establish THD+N, clipping, ADC acquisition settling, rail droop, hot-plug behavior, enclosure coupling, or regulatory compliance.

## Simple Todo List

1. **URGENT:** Rewire U4 and C31/C42/C43 as direct rail connections with 100 nF shunt capacitors to GND.
2. **HIGH:** Make the schematic ground nets match the intended single PCB GND plane, then update the PCB from the schematic.
3. **HIGH:** Reduce expression ADC full-scale voltage, add input protection, and synchronize C40/C41 values.
4. **MEDIUM:** Add nearby GND vias at the distant SD, MOSI, expression ADC, and VBIAS layer transitions; keep the UART routing as-is.
5. **MEDIUM:** Refill all zones and run clean native ERC/DRC; inspect the reported VCC_Daisy islands in KiCad.
6. **LOW:** Move or properly process the R8 pad-2 via, add explicit net classes, and mechanically confirm Card1/J15 edge placement.