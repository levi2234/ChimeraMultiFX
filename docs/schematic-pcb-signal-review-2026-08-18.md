# ChimeraMultiFX Schematic, PCB, and Guitar Signal Review

**Date:** 2026-08-18  
**Design:** `hardware/boards/daisyBoard.kicad_sch` and `hardware/boards/daisyBoard.kicad_pcb`  
**Revision:** `d1bcdf6`  
**Scope:** Current schematic/PCB review, delta from the August 17 and August 18 reviews, and end-to-end risk of audible contamination of the guitar signal.

**2026-08-19 return-path recheck:** The current PCB has a GND via 0.875 mm from the `/SD_CLK_DAISY` transition and a GND via 1.09 mm from the `/SD_SCK` transition. Both provide a suitably short return path. `/SD_CLK_DAISY` now clears the analyzer; `/SD_SCK` remains reported only because the rule uses a hard 1.0 mm center-distance threshold and is treated as a false positive. No additional SD-clock return vias are required.

## Overview

ChimeraMultiFX is a dual-channel 48 kHz guitar-effects platform built around a Daisy Seed audio DSP and an ESP32 control/Wi-Fi processor. The carrier includes two buffered guitar inputs, two buffered audio outputs, a shared 2.5 V analog reference, expression inputs, two SD interfaces, socketed modules, and off-board audio jacks on a 100 mm x 100 mm four-layer PCB.

## Verdict

The analog signal path is now electrically plausible and should not strongly color the guitar band by itself. The 1 Mohm input bias preserves passive-pickup loading, the input RF poles are about 159 kHz, the output low-pass poles are about 21.9 kHz, and the coupling poles are below 2 Hz. The TL072H devices are valid at a nominal 5 V supply, and all unused op-amp channels are now biased as followers.

The design still has a **moderate, credible risk of audible Wi-Fi- or SD-correlated noise** in a prototype and a **significant unresolved risk for repeatable production or formal EMC testing**. The most likely symptoms are periodic ticks, buzz, discrete FFT spurs, startup pops, or a raised noise floor during ESP32 transmit and SD access. Gross loss of guitar tone is less likely than burst-correlated contamination or clipping on unusually hot inputs.

The static EMC score remains 37/100. This is a conservative layout-risk index, not a 37% pass probability. Sixty-nine of 118 EMC findings are heuristic reference-plane findings, so the score overstates the number of independent faults. The specific VREF, channel-2 audio, SD-clock, antenna, and cable-return mechanisms below remain physically credible.

## Critical Findings

| Priority | Finding | Guitar-signal impact | Required action |
|---|---|---|---|
| High | ESP32 is still represented by two headers with no exact body or antenna keepout | RF current can couple into the carrier, VREF, audio wiring, and enclosure; antenna behavior is not repeatable | Use the exact module/dev-board footprint and vendor keepout. Prefer antenna overhang and keep copper, traces, vias, wiring, and enclosure metal out of the specified volume. |
| High | VREF is 134.2 mm long, uses three layers and eight vias, with 51.3 mm on `In2.Cu` | Shared reference pickup appears at every analog stage and can become common-mode or output noise | Shorten the trunk, keep it over continuous GND, avoid the power layer, and add a GND return via beside each unavoidable transition. |
| Warning | `R24`, the 100 ohm VREF-buffer isolation resistor, was removed in the latest revision | Direct MCP6002 drive of a distributed capacitive net may reduce stability margin; exact load is unverified | Restore a small series isolation resistor close to U3 unless bench testing or a part-specific stability analysis proves direct drive is quiet and stable. Do not add large VREF capacitors downstream without validating the isolation network. |
| Warning | `/AUDIO_IN_2` and `/AUDIO_OUT_2` each change layers without nearby return stitching | RF return current detours can increase pickup and rectification in channel 2 | Keep each net on one outer layer or place a GND via next to the signal via. |
| Warning | `/SD_CLK_DAISY` and `/SD_SCK` change layers and remain on outer layers | Their adjacent GND vias provide short return paths, but clock ringing can still inject conducted or radiated energy | Keep the existing return vias and add source-series resistor footprints, nominally 22-47 ohm and selected from scope measurements. |
| High | Exact MPN coverage is 0% and no project datasheet cache exists | Ferrite current/DCR, bulk-cap effective value, module pinout, LED rating, and analog headroom are not reproducible | Populate Manufacturer/MPN/datasheet fields for all ICs, modules, ferrites, bulk capacitors, LEDs, SD sockets, and connectors. |
| Warning | Eleven via-in-pad findings remain in the ESP32 power/reference cluster | Open vias can wick solder and make power filtering inconsistent between assemblies | Move the vias outside lands or specify filled/capped via-in-pad fabrication. |
| Warning | `D1` with `R32 = 27 ohm` is estimated near 41 mA | Excess LED current can add supply/ground disturbance and threatens the LED | Select the LED and recalculate for about 1-5 mA unless high brightness is explicitly required. |

## Component Summary

The fresh schematic analysis found 112 non-power components across 163 nets: seven ICs, 31 resistors, 34 capacitors, 14 connectors, 19 test points, five LEDs, and two ferrite beads. The PCB contains 118 footprints and 104 physical nets. MPN coverage is 0/35 unique BOM parts, so component ratings and lifecycle status are not production-verified.

## Power Tree

```text
External +5 V
|-- FB2 -> VCC_Daisy
|   |-- Daisy Seed/module rails
|   |-- IC1-IC4 TL072H audio buffers
|   `-- U1-U2 MCP6002 expression buffers
|-- U3 MCP6002
|   `-- 100k/100k filtered midpoint -> U3A -> VREF
`-- FB1 -> VCC_ESP32
  `-- ESP32 module input and local 3.3 V regulation
```

The complete source current, module regulator limits, ferrite drop, and ESP32/SD peak demand cannot be checked without exact module and ferrite MPNs.

## Previous Review Delta

| Status | Item | Current result |
|---|---|---|
| Fixed | Standard TL072 below its minimum supply | `IC1`-`IC4` are `TL072HIDR`, whose family supports a 4.5 V minimum total supply. |
| Fixed | Floating unused TL072 channels | Pins 5 are on VREF and pins 6/7 form local followers on all four devices. |
| Fixed | Floating unused U3B channel | U3B is now a follower referenced to the filtered 2.5 V divider node. |
| Fixed | Residual analog via-in-pad at `C29:1` and `U3:8` | Neither is present in the fresh PCB findings. |
| Improved | Schematic/PCB consistency | No-connect markers fell from 25 to 10; cross-analysis findings fell from eight to seven. |
| Regressed | VREF isolation | `R24 = 100 ohm` was removed and U3A now drives VREF directly. |
| Regressed | VREF route length | The route increased from about 118.7 mm to 134.2 mm and now has eight vias. |
| Open | ESP32 antenna implementation | No keepout or rule area exists; only the two 1x15 header rows define the module. |
| Improved | SD/audio return transitions | Both SD clocks have acceptable adjacent GND vias. `/AUDIO_IN_2` now has a GND via 1.30 mm away; `/AUDIO_OUT_2` has no explicit GND via closer than 5.22 mm. |
| Open | Exact sourcing and ratings | Structured MPN coverage remains 0%. |
| Open | Native release evidence | No saved ERC/DRC report and no current Gerber release were found. |

## Signal Analysis Review

## End-to-End Audio Path

### Input channels

Each guitar input follows this topology:

```text
Jack tip
  -> 100 pF shunt RF capacitor
  -> 1 uF AC-coupling capacitor
  -> 1 kohm series resistor
  -> 1 nF shunt capacitor + 1 Mohm bias to VREF
  -> TL072H unity-gain buffer
  -> Daisy audio input
```

The nominal input coupling pole is:

$$
f_{HP,in} = \frac{1}{2\pi(1\,\text{M}\Omega)(1\,\mu\text{F})} \approx 0.16\,\text{Hz}
$$

The 1 kohm/1 nF RF pole is:

$$
f_{LP,in} = \frac{1}{2\pi(1\,\text{k}\Omega)(1\,\text{nF})} \approx 159\,\text{kHz}
$$

This is a good nominal guitar input impedance and should not remove audible bass or treble. The high-impedance node is nevertheless sensitive to RF, jack wiring, flux contamination, and VREF noise. Keep the off-board run shielded or use a twisted signal/return pair.

### Output channels

Each Daisy output follows this topology:

```text
Daisy audio output
  -> 3.3 kohm series resistor + 2.2 nF shunt capacitor
  -> 1 uF AC-coupling capacitor + 1 Mohm bias to VREF
  -> TL072H unity-gain buffer
  -> 100 ohm output resistor
  -> 1 uF output coupling capacitor
  -> 100 kohm pulldown
  -> output jack
```

The nominal reconstruction pole is:

$$
f_{LP,out} = \frac{1}{2\pi(3.3\,\text{k}\Omega)(2.2\,\text{nF})} \approx 21.9\,\text{kHz}
$$

The final 1 uF/100 kohm coupling pole is about 1.6 Hz. The 21.9 kHz first-order pole is approximately 0.8 dB down at 10 kHz and 2.6 dB down at 20 kHz. That is mild ultrasonic filtering, not severe guitar-band coloration, but capacitor tolerance should be included in an audio sweep.

### Route geometry

| Net | Length | Layers | Vias | Assessment |
|---|---:|---|---:|---|
| `/AUDIO_IN_1` | 70.35 mm | F.Cu | 0 | Long but single-layer; lower risk than channel 2 if continuously referenced to GND. |
| `/AUDIO_IN_2` | 63.49 mm | F.Cu/B.Cu | 1 | Add an adjacent return via. |
| `/AUDIO_OUT_1` | 55.09 mm | F.Cu | 0 | Single-layer and relatively low impedance. |
| `/AUDIO_OUT_2` | 46.58 mm | F.Cu/B.Cu | 1 | Add an adjacent return via. |
| `VREF` | 134.24 mm | F.Cu/B.Cu/In2.Cu | 8 | Highest shared analog coupling concern. |

No direct trace-proximity pairs were reported between audio/VREF and SD, UART, or ESP32 nets at the analyzer threshold. This reduces direct crosstalk risk. It does not cover far-field 2.4 GHz coupling, shared plane impedance, enclosure/cable coupling, or demodulation at op-amp inputs.

## Power and Ground

The four-layer order is `F.Cu / In1.Cu GND / In2.Cu power+GND / B.Cu`. The stored `In1.Cu` GND zone is filled at about 85.8% of board area and is the correct adjacent reference for top-side signals. The schematic calls the analog return `GNDA`, while the PCB intentionally implements it as the single `GND` net; there is not a physical split analog plane.

The connectivity analyzer reports seven GND islands, but most analog pads and 48 GND vias belong to one common island and the stored `In1.Cu` fill is one broad region. The isolated entries are concentrated around ESP power capacitors and SD socket pads and are likely union/zone-model artifacts. Do not accept or dismiss them solely from this analyzer: refill all zones and require native KiCad DRC to show zero unconnected items.

`FB1` isolates the ESP branch and `FB2` isolates the Daisy/analog branch, which is directionally good. Their impedance labels alone do not establish current capacity, DCR, DC bias behavior, or useful attenuation with the actual load-side capacitors. ESP32 current bursts and SD writes must be measured at `VCC_ESP32`, `VCC_Daisy`, and VREF simultaneously.

## PDN Impedance

The idealized analyzer model still indicates small anti-resonance regions near 5 MHz on `VCC_Daisy` and 7.9 MHz on `VCC_ESP32`. These are screening estimates only: generic capacitor parasitics, unknown DC-bias derating, unspecified ferrites, module-local capacitance, and mounting inductance prevent a verified impedance prediction.

## Power Budget

The schematic model estimates roughly 120 mA for the six analog ICs on `VCC_Daisy` and about 20 mA for U3 on +5 V. These coarse defaults exclude Daisy processing load, ESP32 transmit peaks, both SD cards, LEDs, and external loads. Size and measure the 5 V source and ferrites for simultaneous DSP, Wi-Fi transmit, and SD write activity.

## Sleep Current Audit

The design is not presently optimized as a battery-sleep product. Module current is not modeled, and the analyzer's state-based resistor-path estimate is not meaningful for product battery life without defined rail shutdown states. If battery operation is intended, add a hardware power-state budget and controllable rail isolation.

## Bus Topology

The board uses UART between ESP32 and Daisy, two independent SD/SPI-style interfaces, two audio input/output channels, and two expression ADC channels. The SD clocks are the highest-risk digital aggressors because both change layers without adjacent return vias; add source damping footprints and keep the interfaces over continuous GND.

## PCB Layout Analysis

- Board: 100 mm x 100 mm, four copper layers, 118 footprints, 104 PCB nets, 634 track segments, and 98 vias.
- Routing is represented as complete by the parser.
- Cross-analysis still reports `/SD_CLK_DAISY` crossing a `+3.3VD` plane boundary and audio nets crossing `+3.3VA` power-region boundaries. These are not proof of a broken GND return because `In1.Cu` is GND, but they indicate layer-3 power fragmentation and should be visually inspected.
- No enforceable antenna keepout exists anywhere on the PCB.
- `Card2` is reported only 0.32 mm from the board edge. Verify socket body, card insertion, copper clearance, and assembly panelization.
- No front-side fiducials were detected for 82 SMD components.
- Test-point coverage is about 11%. Add UART and both SD clocks; the existing audio, rail, VREF, and GND probes are useful.
- The remaining 11 via-in-pad cases are `C21`, `C22`, `C23`, `FB1`, `R22`, `R23`, and `TP21`, concentrated around ESP power and the reference-divider area.
- The existing fabrication archive predates this revision. It must not be ordered as the current board.

## EMC / Cross-Domain Analysis

The original FCC Class B-oriented static score was 37/100 from 118 findings across seven useful categories. After the return-path edits, a current rerun scores 46/100 with 117 findings. The credible subset is the missing return vias on VREF and `/AUDIO_OUT_2`, outer-layer clock routing, `/SD_CLK_DAISY` proximity to `J13`, and the absent ESP32 antenna keepout. `/AUDIO_IN_2` has a practical return via at 1.30 mm. The remaining `/SD_SCK` return warning is a threshold false positive because its GND via is 1.09 mm away. The repeated `GP-001` plane-coverage findings are heuristic and require visual/native DRC confirmation rather than literal counting.

## Thermal Analysis

The thermal analyzer ran at 40 C ambient but assessed zero components because no part had both quantifiable dissipation and usable MPN/datasheet data. This is a verification gap, not evidence that the ESP regulator, ferrites, SD cards, or analog devices will remain cool under sustained load.

## Test Coverage

Nineteen schematic test points cover about 11% of physical PCB nets, including useful audio, VREF, rail, and ground probes. Add accessible pads for UART, both SD clocks, and any module rail needed to correlate digital activity with audio artifacts.

## Assembly Complexity

The board has 82 front-side SMD components and no detected global fiducials. The main assembly risks are the 11 open via-in-pad cases, generic 100 uF footprints, unspecified socket/module geometry, and the SD socket edge placement rather than raw placement count.

## BOM Optimization

Further passive-value consolidation is not a priority. Preserve the intentional filter, bias, LED, and isolation values; focus BOM work on exact MPN qualification, capacitor technology/derating, ferrite current/DCR, and package consistency.

## How Real Is the Audible-Risk?

| Mechanism | Current risk | Why |
|---|---|---|
| Passive pickup loading / bass loss | Low | 1 Mohm input bias and approximately 0.16 Hz coupling pole. |
| Intended high-frequency rolloff | Low to moderate | About 0.8 dB at 10 kHz and 2.6 dB at 20 kHz from the output RC stage; verify component tolerance. |
| Normal op-amp hiss | Low to moderate | TL072H is appropriate for audio, but total noise depends on source impedance, resistor noise, gain staging, and Daisy converter noise. |
| Clipping with boosted/hot guitar levels | Moderate, unquantified | TL072H operates at 5 V, but input common-mode and output swing margin must be measured at the real load and VREF. |
| Wi-Fi burst ticking/buzz | Moderate | Physical separation and ferrite branches help; absent antenna keepout, long VREF, cable pickup, and shared return paths keep the mechanism credible. |
| SD-correlated tones | Moderate | Both clocks use layer transitions without adjacent return vias; `/SD_CLK_DAISY` also runs near `J13`. |
| Channel-to-channel mismatch | Low to moderate | Topologies match, but channel 2 has two additional signal transitions and different layer usage. |
| Assembly-induced noise/intermittence | Moderate | Eleven open via-in-pad findings and unspecified bulk/ferrite parts can create board-to-board variation. |

For a one-off enclosed prototype, a clean result is plausible and the board is not doomed to be noisy. For a repeatable product, the remaining risks are too dependent on exact module placement, cable routing, component selection, and enclosure bonding to call the audio path clean without measurement.

## Bench Acceptance Plan

1. Terminate the input with a pickup-like source impedance and record 20 Hz-20 kHz FFT/noise with Wi-Fi disabled, associated-idle, continuous transmit, and maximum configured transmit power.
2. Repeat while continuously reading and writing each SD interface. Compare channels separately to expose the channel-2 transition penalty.
3. Drive 100 Hz, 1 kHz, and 10 kHz tones; sweep input amplitude to find clipping at the input buffer, Daisy ADC/DAC, and output buffer. Record THD+N and maximum clean peak-to-peak level.
4. Sweep 20 Hz-24 kHz and compare measured response against the 21.9 kHz nominal output pole.
5. Probe VREF, `VCC_Daisy`, and `VCC_ESP32` during Wi-Fi and SD bursts. Look for correlated ripple, ringing, or sustained oscillation after the removal of `R24`.
6. Test boot, reset, firmware update, cable hot-plug, and Daisy clock loss for pops.
7. Repeat with the final shielded/twisted jack harness and final enclosure. Compare lid open/closed and antenna inside/overhanging the enclosure.
8. Use a near-field probe around the ESP power branch, antenna region, SD clocks, VREF buffer/trunk, and both input filters.

## Analyzer Verification

Fresh artifacts are under `analysis/2026-08-18_signal_review/`.

- Schematic analyzer: 112 components, 163 nets, 82 findings.
- Full PCB analyzer with proximity: 118 footprints, 104 nets, 634 segments, 98 vias, 69 findings.
- Schematic/PCB cross-analysis: seven findings.
- FCC Class B-oriented EMC analysis: 118 findings, 37/100 static score, seven useful categories.
- Thermal analyzer at 40 C: ran but assessed zero components because no usable MPN/dissipation data exist.
- Raw schematic, PCB, current Git delta, prior reports, and current zone/route metrics were inspected.

### Reviewer overrides

- The 69 `GP-001` findings are not 69 independent faults. The broad `In1.Cu` GND fill is real; local return discontinuities still require visual/native DRC inspection.
- The seven reported GND islands are not accepted literally because the connectivity model appears to separate pad/via/zone constructs in the ESP cluster. Native DRC is the deciding check.
- Several `DC-003` findings associate signal capacitors with unrelated IC decoupling. The dedicated local 100 nF capacitors remain the valid bypass evidence.
- Six schematic jacks absent from the PCB are treated as intentional off-board hardware. Mark them excluded from board and document the harness.

## Not Performed / Review Limits

- Native KiCad ERC/DRC was not independently run because `kicad-cli` is unavailable. The latest commit says DRC was checked, but no report is stored for review.
- SPICE was not run because ngspice, LTspice, and Xyce are unavailable.
- Datasheet extraction and lifecycle audit were not possible because no `datasheets/` cache exists and structured MPN coverage is 0%. Except for the previously established TL072H supply-family check, pin, rating, ferrite, and capacitor conclusions are consistency or plausibility checks.
- Thermal analysis had no modelable components; zero findings does not mean zero thermal risk.
- Current Gerbers were not available, so fabrication-layer and paste/drill checks apply only to the stale archive.
- Static analysis cannot predict absolute audible noise or regulatory margin. Final cables, enclosure, antenna placement, firmware activity, and source/load impedances require measurement.

## Recommended Order

1. Implement the exact ESP32 module footprint and antenna keepout/overhang.
2. Shorten VREF, remove it from `In2.Cu`, add transition return vias, and restore or validate output isolation at U3.
3. Keep the existing SD-clock return vias, add a closer return via for `/AUDIO_OUT_2`, and add source damping footprints for the clocks.
4. Qualify exact ferrites, bulk capacitors, modules, LEDs, sockets, and analog IC suffixes with structured MPN fields.
5. Correct `D1` current and resolve the remaining ESP-cluster via-in-pad construction.
6. Refill zones and save clean native ERC/DRC reports.
7. Regenerate and independently inspect Gerber, drill, BOM, and CPL outputs.
8. Pass the Wi-Fi/SD audio test matrix in the final enclosure before calling the signal path production-clean.

## Readiness

**Prototype bring-up:** reasonable after the antenna/VREF/return-via changes, with mandatory audio-noise and clipping tests.  
**Another fabrication order:** not recommended yet.  
**Production or FCC/CE readiness:** not demonstrated.