# ChimeraMultiFX Schematic and PCB Review

**Date:** 2026-08-18  
**Design:** `hardware/boards/daisyBoard.kicad_sch` and `hardware/boards/daisyBoard.kicad_pcb`  
**Product intent:** Dual-channel 48 kHz Daisy Seed guitar-effects DSP with ESP32 Wi-Fi/control, browser UI, expression inputs, removable storage, and off-board audio jacks.

## Overview

The board combines a Daisy Seed audio engine, ESP32 network/control processor, dual analog audio conditioning, expression inputs, two SD interfaces, and off-board controls in a 100 mm x 100 mm four-layer design. The review prioritizes audio headroom and noise, reference stability, Wi-Fi/SD return paths, module integration, manufacturability, and repeatable sourcing.

## Verdict

**Not ready for fabrication yet, but materially improved.** `IC1`-`IC4` are now the 4.5 V-qualified `TL072HIDR`, and the broad analog via-in-pad issue is almost completely resolved. Remaining pre-fabrication concerns are floating unused amplifier channels, the missing ESP32 antenna keepout, two residual analog pad/via overlaps, long VREF routing, unqualified ferrites/bulk capacitors, absent structured MPN fields, and no current native DRC/ERC or Gerber release.

## Previous Review Delta

| Status | Item | Current result |
|---|---|---|
| Fixed | Local op-amp bypassing | All seven analog ICs now have a same-side 100 nF supply bypass about 2.2-3.1 mm from the package center. |
| Fixed | Layer order | `In1.Cu` is now the GND plane and `In2.Cu` is the power layer. The stored GND fill is one region with about 85.6% area coverage. |
| Fixed | TL072 supply range | `IC1`-`IC4` now specify `TL072HIDR`, whose 4.5 V minimum total supply includes the nominal 5 V rail. Pin 8 is VCC+ and pin 4 is GND in the selected SOIC-8 package. |
| Improved | Analog via-in-pad | Analog-related VP-001 findings fell from 23 to 2; all four TL072H supply-pad overlaps were removed. |
| Improved | Overall via-in-pad | VP-001 findings fell from 84 to 13. Most remaining cases are in the ESP power cluster; `C29:1` and `U3:8` are the two remaining analog cases. |
| New | Analog rail ferrite | `FB2` replaces `NT1` between +5 V and VCC_Daisy. Connectivity is correct, but its current rating and DCR are unspecified. |
| Improved | EMC static score | Analyzer score increased from 13/100 to 37/100. This is a relative risk metric, not a compliance prediction. |
| Open | Unused op-amp channels | `IC1B`-`IC4B` and `U3B` still have pins 5, 6, and 7 unconnected. |
| Open | ESP32 antenna implementation | ESP32 remains two 1x15 headers with no body, antenna outline, courtyard, or copper/routing keepout. |
| Open | VREF distribution | About 118.7 mm long, six layer changes, and about 52.7 mm routed on the inner power layer. |
| Open | Sourcing/verification | MPN coverage remains 0%; exact pinout, ratings, capacitor derating, and ferrite suitability cannot be fully verified. |
| New | LED current | `D1` uses `R32 = 27 ohm`; static analysis estimates about 41 mA, excessive for a 0201 indicator LED. |

## Critical Findings

| Severity | Issue | Action before fabrication |
|---|---|---|
| High | Five unused amplifier channels float | Configure each unused channel according to the selected op amp datasheet, normally as a follower biased at VREF. |
| High | No enforced ESP32 antenna keepout | Use the exact module/dev-board footprint and clear copper, tracks, vias, components, wiring, and enclosure metal around the antenna. Prefer antenna overhang. |
| High | No exact MPNs | Add MPN/manufacturer fields for ICs, modules, ferrites, LEDs, bulk capacitors, connectors, and SD sockets. |
| Warning | Two analog pad/via overlaps remain | Move the GND via out of `C29:1` and the supply via out of `U3:8`, or specify filled/capped via-in-pad. |
| Warning | `C18` and `C31` return vias are about 3.1/3.0 mm away | Bring each true bypass return via adjacent to the capacitor ground pad without putting the drill inside the solder land. |
| Warning | `FB2` is unqualified | Select an exact 0805 ferrite with sufficient current rating, low DCR, and impedance curve for the Daisy plus analog load. |
| Warning | VREF is long and changes layers six times | Keep it on an outer layer where practical, shorten the trunk, and place a GND return via beside each unavoidable transition. |
| Warning | Four nominal 100 uF capacitors use generic 1206 footprints | Select exact parts and validate package, polarity, voltage rating, effective capacitance under DC bias, leakage, and assembly orientation. |
| Warning | `D1` current is excessive | Recalculate for the chosen LED. A status LED usually needs roughly 1-5 mA, not about 41 mA. |
| Warning | Current Gerbers were not regenerated | Export a fresh fabrication set after all changes and inspect it in an independent CAM viewer. |

## Component Summary

The schematic analyzer found 113 non-power components: 7 ICs, 32 resistors, 34 capacitors, 14 connectors, 19 test points, 5 LEDs, and 2 ferrite beads across 174 schematic nets. The PCB contains 119 footprints, including intentional module/header and mechanical content. Exact sourcing data remain absent from structured MPN properties for all 35 unique BOM parts.

## Power Tree

```text
External +5 V
├── FB2, 600 ohm @ 100 MHz → VCC_Daisy
│   ├── Daisy Seed/module rails
│   ├── IC1-IC4 TL072HIDR audio op amps
│   └── U1-U2 expression buffers
├── U3 MCP6002
│   └── 100k/100k midpoint → U3A buffer → 100R → VREF
└── FB1 → VCC_ESP32
	└── ESP32 module input and local 3.3 V rail
```

The exact source current capability, ferrite drop, module regulator limits, and peak ESP32 demand remain unverified because source/module/ferrite MPNs are missing.

## Signal Analysis Review

Static analysis recognized the VREF divider/filter, analog RC networks, op-amp stages, SD/UART buses, and local decoupling. The op-amp configuration detector could not classify the stages reliably, so the critical analog conclusions below come from raw pin/net inspection rather than topology labels alone. SPICE confirmation is unavailable.

## VREF Review

The buffered-reference topology is directionally good:

- `R22 = 100 kohm` and `R23 = 100 kohm` create a nominal 2.5 V midpoint from 5 V.
- `C19 = 100 uF` and `C20 = 100 nF` filter the midpoint before `U3A`.
- `U3A` (MCP6002) is wired as the buffer.
- `R24 = 100 ohm` isolates the buffer output from the distributed VREF load and its parasitic capacitance.
- The four audio bias feeds use 1 Mohm resistors, so DC loading and drop across `R24` should be small.

With a 50 kohm Thevenin source and nominal 100 uF, the low-frequency pole is approximately:

$$
f_c = \frac{1}{2\pi(50\,\text{k}\Omega)(100\,\mu\text{F})} \approx 0.032\,\text{Hz}
$$

Do not add a large capacitor directly to the MCP6002 output by default. A capacitive load can reduce phase margin; the existing 100 ohm isolation resistor is the right kind of mitigation. If local VREF capacitors are added at remote stages, validate the total capacitance and isolation network against the exact op-amp datasheet or simulation.

### VREF layout observations

- Total routed length is about 118.7 mm: 51.4 mm on B.Cu, 14.9 mm on F.Cu, and 52.4 mm on `In2.Cu`.
- Six VREF transitions have no GND stitching via within 1 mm.
- Routing VREF on the power layer cuts and constrains power copper. Prefer a short outer-layer trunk over continuous `In1.Cu` GND.
- Keep VREF away from ESP32 antenna/current-burst paths and SD clocks.
- Probe VREF at `TP5` during Wi-Fi transmit and full-scale audio. Check DC level, startup settling, audio-band noise, and burst-correlated ripple.

## Local Decoupling

The new bypass network is a clear improvement and the net assignments are correct.

| IC | Bypass | Nets | Approx. distance | Assessment |
|---|---|---|---:|---|
| `IC1` | `C18`, 100 nF | VCC_Daisy-GND | 2.61 mm | Good |
| `IC2` | `C27`, 100 nF | VCC_Daisy-GND | 3.07 mm | Acceptable; move slightly closer if easy |
| `IC3` | `C26`, 100 nF | VCC_Daisy-GND | 2.31 mm | Good |
| `IC4` | `C28`, 100 nF | VCC_Daisy-GND | 2.51 mm | Good |
| `U1` | `C29`, 100 nF | VCC_Daisy-GND | 2.17 mm | Good |
| `U2` | `C30`, 100 nF | VCC_Daisy-GND | 2.78 mm | Good |
| `U3` | `C31`, 100 nF | +5V-GND | 2.17 mm | Good |

For the next layout pass:

- Prefer 0402 or 0603 for 100 nF high-frequency bypassing unless hand assembly dictates otherwise. A 1206 has higher connection inductance and is physically harder to place tight to SOIC supply pins.
- Route supply pin to capacitor first, then into the rail; connect the ground pad immediately to `In1.Cu` with an adjacent via.
- Avoid open vias centered in IC or capacitor lands. Tented opposite-side vias still can wick solder unless the fabricator fills/caps them.
- The revised layout clears this condition for nearly every analog IC and bypass capacitor. Move the remaining `C29:1` and `U3:8` vias as well.
- `C18` and `C31` are close to their ICs, but their nearest vias are now about 3.1 mm and 3.0 mm away. Put a GND via immediately beside each capacitor ground pad.
- Keep one local 1 uF per analog cluster and bulk capacitance at the rail entry.

## Op-Amp Suitability

### TL072HIDR

`IC1`-`IC4` now specify TI `TL072HIDR`. TI rates TL07xH for 4.5-40 V total supply, so nominal 5 V operation is valid. The selected `D` package is SOIC-8 and matches the existing footprint and pin map: pin 8 VCC+, pin 4 VCC-. The part is not rail-to-rail output, so clean input/output headroom around the 2.5 V reference still requires bench verification with worst-case boosted guitar levels.

The current schematic value identifies the exact part, but the structured MPN field remains empty and the Datasheet property incorrectly points to `tl074a`. Set Manufacturer=`Texas Instruments`, MPN=`TL072HIDR`, LCSC=`C4370389`, and Datasheet=`https://www.ti.com/lit/gpn/TL072H` on `IC1`-`IC4`.

### MCP6002

Microchip identifies MCP6002 as an in-production 1 MHz low-power op amp operating from 1.8 V. Its use at 5 V is plausible and the 100 ohm VREF output isolation is helpful. At about 28 nV/sqrt(Hz), it is not an especially low-noise audio amplifier, but it is reasonable for expression buffering and a low-current reference. Exact suffix/temperature/package MPNs are still required.

### Unused channels

`IC1B`-`IC4B` and `U3B` remain floating. For a common unity-gain-stable dual op amp, a typical treatment is non-inverting input to VREF and output tied to inverting input. Apply the selected manufacturer's guidance; do not ground an input if that violates common-mode range, and do not short an output to a rail.

## PCB Layout Analysis

### Positive findings

- Four-layer 100 mm x 100 mm board with `F.Cu / In1.Cu GND / In2.Cu power / B.Cu` ordering.
- Stored `In1.Cu` GND fill is one region and covers about 85.6% of the board area.
- ESP32 power remains isolated through `FB1` with local bulk and ceramic capacitance.
- Analog and digital regions are physically separated to a useful degree.
- Parser reports all routed nets complete.
- Analog-related via-in-pad findings dropped from 23 to 2; all TL072H solder lands are now clear of drilled vias.

### Remaining risks

- No keepout zones exist anywhere on the PCB. Copper remains under the probable ESP32 antenna area.
- VREF, audio, expression, and SD nets still change layers without nearby GND return vias.
- `/SD_CLK_DAISY` and `/SD_SCK` change layers without adjacent return vias and run on outer layers. Add source damping footprints if supported by the module interface and measured edge rate.
- The default net class permits 0.127 mm clearance and includes 0.127 mm routing. Use explicit classes for audio, power, SD clocks, and general signals instead of one permissive class.
- `Card2` is reported about 0.32 mm from the board edge. Verify the connector body, card insertion geometry, and fabricator copper-to-edge rules.
- No front-side fiducials were detected for 83 SMD parts. Add three global fiducials if using automated assembly.
- Test coverage is about 10%. Add accessible points for all rails, VREF, UART, SD clocks, and audio stage inputs/outputs.
- Two analog overlaps remain: `C29:1` and `U3:8`. Eleven additional VP-001 cases remain in the ESP power/reference cluster and should be reviewed before stencil assembly.

## EMC / Cross-Domain Analysis

The static FCC Class B-oriented risk score improved to 37/100 after the GND-first stack and local bypass changes. The remaining deterministic concerns are layer transitions without adjacent GND vias, outer-layer SD clocks, SD clock proximity to `J13`, and the absent ESP32 antenna keepout. The score cannot predict compliance or account for the final enclosure and cable harnesses; use it to prioritize bench and pre-compliance measurements.

### Analyzer overrides

- The reported 10 GND islands are not accepted literally. The raw stored zone has one filled `In1.Cu` GND region at about 85.8% fill. The connectivity graph appears to count isolated pads/tracks as islands.
- Many GP-001 findings are similarly inflated. The corrected GND-first stack is real; local void crossings still need KiCad DRC and visual inspection.
- The two remaining analog via-in-pad warnings are geometric and real. Most prior findings were resolved by moving the vias beside the pads.
- Six schematic jacks (`J6`-`J11`) missing from the PCB appear to be off-board hardware. Mark them excluded from board and document the harness so this is intentional rather than ambiguous.

## Audio/Product Insights

- Preserve approximately 1 Mohm input impedance for passive guitar pickups. Verify the complete input network at audio frequencies, including ESD parts and jack wiring.
- Design analog headroom from worst-case guitar/boost-pedal levels, not nominal pickup output. Measure clipping onset at every analog stage and at the Daisy ADC.
- Add input protection that does not add excessive capacitance or leakage: series resistance, rail clamps/low-capacitance protection, and defined behavior for hot-plug transients.
- Keep Wi-Fi burst current out of analog supply and ground paths. Validate with Wi-Fi disabled, idle, and continuous maximum-power transmit while monitoring audio FFT, VCC_Daisy, and VREF.
- Route off-board audio as shielded or twisted signal/return pairs. Avoid sharing return conductors with LED, SD, or ESP32 currents.
- Consider relay or analog-switch mute behavior during boot, reset, firmware update, and loss of Daisy clocks to avoid loud pops.
- Document maximum input/output level, input impedance, output impedance, latency, sample rate, bypass behavior, and expression-pedal electrical standard as product requirements.

## Manufacturing and Sourcing

- MPN coverage is 0%, so the lifecycle audit and full pin/rating verification are blocked.
- `IC1`-`IC4` contain the correct `TL072HIDR` text value but still need Manufacturer/MPN/LCSC properties and the correct TI datasheet URL.
- `C13`, `C15`, `C19`, and `C21` are nominal 100 uF polarized capacitors assigned to generic 1206 footprints. Select real parts before relying on those values.
- `FB1` needs an exact impedance curve, current rating, saturation behavior, and DCR suitable for ESP32 transmit peaks.
- `FB2` needs the same qualification for the complete Daisy/analog branch. Its connectivity from +5 V to VCC_Daisy is correct.
- Set the intended copper finish; the KiCad stack currently records `None`.
- Add assembly notes for socketed Daisy/ESP32 modules, SD sockets, off-board jacks, and polarity/orientation-sensitive parts.

## PDN Impedance

The idealized model sees 9 capacitors totaling 100.8 uF on VCC_Daisy, 3 totaling 100.2 uF on VCC_ESP32, 10.1 uF on each modeled 3.3 V rail, and only 100 nF directly on +5 V at U3. It predicts small anti-resonances near 5.0 MHz on VCC_Daisy and 7.9 MHz on VCC_ESP32. Treat these values as screening estimates: generic package parasitics, missing MPNs, MLCC DC bias, ferrite impedance, module capacitance, and PCB mounting inductance are not modeled accurately.

## Power Budget

The analyzer estimates about 120 mA for the six analog ICs on VCC_Daisy and 20 mA for U3 on +5 V. These are coarse defaults, not datasheet totals, and exclude Daisy/ESP32 peak demand, SD-card transients, LEDs, and external loads. Measure the complete board and size the 5 V source and FB1 for ESP32 transmit and SD write peaks with margin.

## Sleep Current Audit

The heuristic worst case is 69.84 mA and the state-aware estimate is about 0.59 mA, dominated by pull paths. This pedal is not presently optimized as a battery-sleep product, and module currents are not modeled. If battery operation is planned, define a power-state budget and add hardware rail shutdown rather than relying on MCU sleep alone.

## Bus Topology

The schematic uses UART between ESP32 and Daisy, SD/SPI-style buses for removable storage, two audio channels, and two expression channels. No graphical KiCad bus objects are used. Give SD clocks explicit source-series resistor footprints, keep each bus over continuous GND, and document which MCU owns each card and chip-select during reset.

## Test Coverage

Nineteen test points cover 13 nets, including +5 V, VCC_Daisy, VCC_ESP32, VREF, expression references, audio jack pads, and ground. Missing key coverage includes +3.3VA, +3.3VD, ESP-3.3V, VIN, UART, and SD SPI nets. Add compact accessible pads before fabrication.

## Assembly Complexity

The analyzer rates assembly complexity low at 26/100: 111 SMD and 16 THT placements, mostly 0805, 1206, and SOIC. Five 0201-class parts are the hard placements. The via moves substantially reduce assembly risk; the residual concerns are 13 pad/via overlaps, ambiguous module geometry, generic 100 uF footprints, missing fiducials, and absent exact assembly part numbers.

## BOM Optimization

The passive set is already modest: 10 resistor values, 7 capacitor values, and one single-use passive value. Do not consolidate values at the expense of filter response, LED current, VREF noise, or stability. The priority is exact MPN qualification and package consistency, not further line-count reduction.

## Thermal Analysis

The thermal analyzer ran at 40 C ambient but assessed zero parts because no component had both quantifiable dissipation and usable MPN/datasheet data. This is a review gap, not evidence of low temperature. Check the 5 V source path, FB1, module regulators, SD cards, and any enclosure hot spots during sustained Wi-Fi and DSP load.

## Gerber Verification

The only complete fabrication archive is dated 2026-08-10 and predates the present revision. It contains all four copper layers, mask, paste, silk, edge cuts, PTH, and NPTH files. Analyzer warnings about differing copper extents and 43% front-paste-to-front-copper flash count require CAM inspection but may be benign. Regenerate all outputs after the design is frozen.

## Analyzer Verification

### Analyses performed

- Current schematic analyzer: 113 components, 174 nets, 78 findings.
- Current full PCB analyzer with proximity: 119 footprints, 115 PCB nets, 620 track segments, 98 vias, 72 findings.
- Schematic/PCB cross-analysis: 8 findings.
- FCC Class B-oriented EMC risk analysis: 117 findings, static risk score 37/100.
- Thermal analyzer at 40 C ambient: ran but skipped modeling because no MPN/power data were available.
- Archived Gerber/drill analyzer: three warnings; package is dated 2026-08-10 and predates this revision.
- Raw schematic, PCB, project settings, Git delta, firmware architecture, and manufacturer product pages inspected.

### Not performed / limits

- Native KiCad ERC and DRC were not run because `kicad-cli` is unavailable. Run both in KiCad after refilling all zones.
- SPICE was not run because ngspice, LTspice, and Xyce are unavailable.
- No project datasheet cache exists and no exact MPNs are populated. Most pin-level and rating checks are consistency-only.
- Thermal analysis produced no component temperatures because dissipative part data are missing.
- Lifecycle analysis was not meaningful with 0% MPN coverage.
- Current Gerbers do not exist; the reviewed fabrication archive is stale.

## Priority Order

1. Terminate all unused op-amp channels correctly.
2. Create the exact ESP32 module footprint and all-layer antenna keepout/overhang.
3. Move the remaining `C29:1` and `U3:8` vias; place the `C18` and `C31` GND vias adjacent to, but outside, their pads.
4. Add exact Manufacturer/MPN/LCSC/datasheet properties for TL072HIDR and qualify `FB2` current/DCR.
5. Shorten VREF, keep it off the inner power layer where practical, and add adjacent GND return vias.
6. Correct `D1` current and select exact bulk capacitors/ferrite/LED/module MPNs.
7. Add explicit net classes, fiducials, test points, and off-board harness documentation.
8. Refill zones, run native ERC/DRC, regenerate Gerbers/CPL/BOM, and inspect in CAM.
9. Perform bench noise, clipping, startup-pop, Wi-Fi transmit, RSSI, and final-enclosure tests.

## Final Readiness Checklist

- [x] TL072H supply range and SOIC-8 pin mapping valid at nominal 5 V
- [ ] All unused amplifier channels safely biased
- [x] One local 100 nF bypass per analog IC
- [x] GND plane adjacent to F.Cu
- [ ] Exact ESP32 footprint and enforced antenna keepout
- [ ] No unapproved open via-in-pad construction (`C29:1` and `U3:8` still open in analog section)
- [ ] VREF transitions have adjacent return vias and reduced route length
- [ ] LED currents recalculated
- [ ] Exact MPNs and ratings populated
- [ ] Native ERC/DRC clean after zone refill
- [ ] Fresh Gerber, drill, BOM, and CPL files inspected
- [ ] Wi-Fi-correlated audio noise and final-enclosure RF tests passed