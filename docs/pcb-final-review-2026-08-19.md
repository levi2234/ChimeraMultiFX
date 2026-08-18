# ChimeraMultiFX Final PCB and Multi-Effects Review

**Date:** 2026-08-19  
**Design:** `hardware/boards/daisyBoard.kicad_sch` and `hardware/boards/daisyBoard.kicad_pcb`  
**PCB revision analyzed:** working tree derived from commit `06b2de2`, source SHA-256 `89c725a2147554c6311b7c6661d85c96c0677566b35e35479af3f27390f780ca`  
**Intent:** Dual-channel 48 kHz Daisy Seed multi-effects processor with ESP32 control/Wi-Fi, two SD interfaces, expression inputs, and off-board audio jacks.

## Overview

This is a 100 mm x 100 mm, four-layer carrier for a Daisy Seed audio DSP and ESP32 network/control processor. It provides two buffered guitar inputs, two buffered outputs, four-lane firmware routing, expression inputs, independent Daisy and ESP32 SD interfaces, a shared buffered 2.5 V reference, and off-board user controls and audio jacks.

## Final Verdict

**The architecture is well suited to the intended multi-effects use case and the current PCB is credible for a carefully tested prototype. It is not ready for a final fabrication release or production order.**

The audio topology should preserve normal guitar bandwidth and pickup loading. The four-layer stack, local analog bypassing, separate ferrite-fed ESP32 and Daisy branches, real four-layer antenna keepout, and physical analog/digital separation are all good foundations. The latest edit also removes the final reported open via-in-pad cases from the VREF divider.

The remaining performance risk is not gross loss of tone or lack of DSP routing capability. It is audible contamination or instability under combined Wi-Fi, SD, and heavy DSP activity: VREF is still long and changes layers eight times, the MCP6002 VREF buffer directly drives that distributed net without the former 100 ohm isolation resistor, channel 2 audio has imperfect transition returns, and exact ferrite/capacitor/module behavior is unknown. Analog clipping margin at hot pedal levels is also not established.

### Readiness

| Target | Assessment |
|---|---|
| Bench prototype | **Good candidate**, after correcting D1 current and with mandatory noise/headroom tests |
| Another PCB order | **Not recommended yet** without native clean DRC/ERC and current Gerbers |
| Repeatable product | **Not demonstrated** because MPN coverage is 0% and thermal/power behavior is unqualified |
| FCC/CE release | **Not demonstrated**; static EMC analysis is only a risk screen |

## Critical Findings

| Priority | Finding | Multi-effects consequence | Required action |
|---|---|---|---|
| High | MPN coverage is 0/35 unique parts and no project datasheet cache exists | Ferrite loss, capacitor derating, module pinout/antenna geometry, LED rating, and analog limits are not repeatable | Populate Manufacturer/MPN/datasheet fields and rerun the review with extracted datasheets |
| High | No current native DRC/ERC report or current Gerber release exists | A structurally plausible PCB can still contain a release-breaking native rule or output error | Refill zones, save zero-unconnected DRC/ERC reports, regenerate Gerbers/drills, and inspect in an independent CAM viewer |
| Warning | VREF is 136.87 mm long, uses F.Cu/B.Cu/In2.Cu, and has eight vias | Noise or burst energy on VREF is shared by all analog stages and can become ticks, buzz, offset movement, or distortion | Shorten it, remove it from In2.Cu where practical, and add adjacent GND return vias at its transitions |
| Warning | U3 drives distributed VREF directly; the former R24 isolation resistor is absent | Distributed capacitance can reduce MCP6002 phase margin or make VREF respond poorly to burst loads | Restore a small series resistor at U3 or prove stability with load-step, FFT, and oscilloscope testing |
| Warning | `/AUDIO_OUT_2` changes layers with its nearest GND via 5.22 mm away | Channel 2 has greater RF pickup/return-loop risk than channel 1 | Add a GND via beside the signal via or keep the route on one layer |
| Warning | D1 uses R32 = 27 ohm; heuristic current is about 41 mA | Excess LED current adds ESP rail disturbance and may damage or age the LED | Select the exact LED and target roughly 1-5 mA unless higher current is intentional |
| Warning | Exact ferrites and 100 uF 1206 capacitors are unspecified | Wi-Fi and SD load steps may modulate VCC_ESP32, VCC_Daisy, or VREF; nominal capacitance may not exist in production | Select parts by current, DCR, impedance curve, voltage bias, leakage, polarity, and package |
| Warning | Card1 is 0.82 mm from the board edge | Assembly/depaneling tolerance may damage or shift the socket | Confirm the exact socket body, intended overhang, panel rails, and assembler clearance |
| Warning | No global front fiducials for 82 front-side SMD parts | Placement repeatability is reduced for automated assembly | Add three global fiducials or obtain assembler approval for the panel strategy |

## Component Summary

The schematic contains 112 non-power components across 163 nets: seven ICs, 31 resistors, 34 capacitors, 14 connectors, 19 test points, five LEDs, and two ferrite beads. The PCB has 118 footprints because it also contains intentional module/header and mechanical content. The unique BOM has 35 part groups, none with a structured MPN.

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
	`-- ESP32 module input -> ESP-3.3V
```

The exact 5 V source capacity, module regulator limits, ferrite drop, and simultaneous ESP32/SD peak-current margin are not established without exact parts and measurement.

## Previous Review Delta

| Status | Item | Final state |
|---|---|---|
| Fixed | ESP32 antenna protection | A 226.66 mm2 keepout now blocks tracks, vias, pads, and copper pours on all four copper layers. |
| Fixed | ESP32 SD placement near antenna | Card2 moved to `(76.9, 64.475)`; the antenna keepout ends at `y = 53.6`, leaving physical separation. |
| Fixed | Card2 edge warning | The previous approximately 0.32 mm edge warning is gone. |
| Fixed | VREF-divider via-in-pad | The latest working-tree edit moved the R22/R23 vias; no VP-001 warnings remain. |
| Preserved | SD clock returns | `/SD_CLK_DAISY` has a GND via 0.875 mm from its transition. `/SD_SCK` has one 1.09 mm away. |
| Open | VREF distribution | Route increased to 136.87 mm and still has eight transitions, none within the analyzer's 1 mm GND-via threshold. |
| Open | VREF output isolation | R24 remains absent from the current schematic and PCB. |
| Open | Part qualification | Structured MPN coverage remains 0%. |
| Open | Release evidence | Available Gerbers are dated 2026-08-10 and predate the current PCB. |

## Signal Analysis Review

The sections below assess how the complete analog path, board routing, and current firmware configuration support the multi-effects use case.

### Multi-Effects Performance Assessment

### DSP and routing capability

The firmware configures stereo audio at 48 kHz. It supports four lanes with up to eight slots per lane, giving a maximum routing model of 32 instantiated effect positions. The current firmware does not override libDaisy's default 48-sample audio block, so the callback cadence is approximately:

$$
t_{block} = \frac{48}{48000} = 1\ \text{ms}
$$

This is an appropriate real-time control cadence for a pedal. It is not the complete input-to-output latency: codec group delay, buffering, and individual effects add latency. CPU headroom also depends on the chosen effects, especially convolution, pitch shifting, long stereo delays, and reverbs. Those are firmware/runtime limits rather than PCB limits.

### Frequency response and pickup loading

Each input uses a nominal 1 Mohm bias and 1 uF coupling capacitor:

$$
f_{HP,in} \approx \frac{1}{2\pi(1\ \text{M}\Omega)(1\ \mu\text{F})} = 0.16\ \text{Hz}
$$

The 1 kohm/1 nF input RF network is approximately 159 kHz. It should not remove guitar treble. The output 3.3 kohm/2.2 nF pole is approximately 21.9 kHz, about 0.8 dB down at 10 kHz and 2.6 dB down at 20 kHz. That is reasonable anti-ultrasonic shaping for guitar, but it should be included in the measured product response.

**Expected guitar-band fidelity: good.** Passive-pickup loading and bass loss are low-risk. Mild top-octave rolloff is intentional and unlikely to be objectionable for guitar.

### Noise and interference

**Expected noise performance: potentially good, but not yet proven under digital load.** The key strengths are a continuous In1.Cu GND fill with one stored region and about 85.8% area coverage, local 100 nF bypass capacitors roughly 2.2-3.1 mm from the seven analog ICs, ferrite-separated ESP32 and Daisy branches, and no reported direct proximity violation between audio/VREF and digital clocks.

The dominant weaknesses are the shared VREF route and cable-coupled RF. Wi-Fi transmit or SD access can produce current bursts that enter the analog path through supply impedance, VREF, connector harnesses, or op-amp input rectification. The most likely audible symptoms are periodic ticks, buzz, discrete FFT spurs, startup pops, or a raised noise floor rather than broad tonal degradation.

### Channel matching

| Net | Length | Layers/vias | Assessment |
|---|---:|---|---|
| `/AUDIO_IN_1` | 70.35 mm | F.Cu, 0 vias | Long but continuously referenced; lower transition risk |
| `/AUDIO_IN_2` | 63.49 mm | F.Cu/B.Cu, 1 via | Nearest GND via is 1.30 mm away; acceptable but improvable |
| `/AUDIO_OUT_1` | 55.09 mm | F.Cu, 0 vias | Lower-risk single-layer route |
| `/AUDIO_OUT_2` | 46.58 mm | F.Cu/B.Cu, 1 via | Nearest GND via is 5.22 mm away; fix before release |

Channel 1 has the cleaner routing geometry. Channel 2 should still function, but it has a modestly higher susceptibility risk and should be compared independently during FFT, THD+N, and Wi-Fi/SD tests.

### Headroom and clipping

The TL072H choice is plausible at a nominal 5 V total supply, but the board has no demonstrated margin for boosted guitar, synth, or line-level signals. The part is not rail-to-rail, the Daisy converter range and analog-stage swing are not established here, and component tolerances are unqualified.

**Expected ordinary guitar headroom: plausible. Expected hot-pedal/line-level headroom: moderate risk until measured.** Find clipping onset at each input buffer, converter input/output, and final output buffer with the real load.

### Overall use-case scorecard

| Attribute | Assessment |
|---|---|
| Stereo/dual-channel architecture | Strong |
| Guitar input loading | Strong |
| Nominal guitar-band response | Strong |
| Routing flexibility | Strong |
| Base callback latency | Strong at approximately 1 ms per block |
| Heavy-effect CPU capacity | Firmware-dependent; not established by PCB review |
| Analog headroom | Plausible but unquantified |
| Wi-Fi/SD immunity | Moderate risk |
| Channel-to-channel consistency | Good topology, channel 2 layout slightly weaker |
| Production repeatability | Weak until exact parts and release outputs are controlled |

## PCB Layout Analysis

The final analyzer reports 118 footprints, 104 PCB nets, 656 track segments, 95 vias, four copper layers, and zero unrouted nets. The board is 100 mm x 100 mm. Generic DFM screening found no minimum-width, spacing, drill, or annular-ring violation for its standard tier.

Ten nets include a 0.127 mm segment, including both audio inputs and five Daisy SD signals. These appear to be short escape/neck-down sections rather than current-carrying limitations. They are manufacturable under the modeled rules, but explicit audio, clock, and general-signal net classes would make intent clearer and prevent accidental spread of the permissive width.

The four TV-001 findings on Card1/Card2 pads 11 and 12 are treated as false positives. Those pads are grounded microSD socket shell tabs, not IC thermal exposed pads requiring five thermal vias. Ground stitching around each socket remains useful for EMC, but five vias per shell land are not a thermal requirement.

Test-point coverage is 11%. Existing rail, audio, VREF, and GND access is useful. Add UART, both SD clocks, and representative SD data/chip-select points if repeatable bring-up and correlation of digital activity to audio artifacts matter.

## Power and Reference Integrity

The architecture separates `VCC_ESP32` and `VCC_Daisy` from +5 V through FB1 and FB2. This is directionally appropriate for keeping ESP32 current bursts out of the analog/Daisy branch. The exact filter behavior is unknown because the ferrites, load-side capacitors, module-local capacitance, and source impedance are unspecified.

VREF is generated from a filtered 100 kohm/100 kohm midpoint and buffered by U3A. The low-frequency divider filter is strong, but direct drive of a long distributed net is the unresolved stability issue. Do not add arbitrary large capacitors to remote VREF points without validating the buffer and any series isolation.

Analyzer rail-source warnings mostly reflect module-generated or externally supplied rails without PWR_FLAG/power-output declarations. They are ERC-modeling gaps, not accepted evidence that all eleven rails are physically unsourced. Add explicit source declarations so native ERC can distinguish intent from a real wiring error.

## EMC / Cross-Domain Analysis

The FCC Class B-oriented static risk score is 46/100 from 120 findings across seven useful categories. This is not a pass probability. Sixty-nine findings are heuristic GP-001 plane-coverage reports, while the raw PCB stores one broad 85.8%-filled In1.Cu GND region. They are not treated as 69 independent faults.

The two clock transitions are better than the raw finding list implies:

- `/SD_CLK_DAISY`: nearest GND via 0.875 mm; RP-001 clears.
- `/SD_SCK`: nearest GND via 1.09 mm; the error-level RP-001 is a 0.09 mm threshold miss and is downgraded to a minor improvement, not a blocker.

Other SD data/control vias are 1.28-7.33 mm from the nearest GND via. Their edge rates can still matter even when their names do not contain `CLK`. Add local return vias where practical, with first priority on MOSI, CS, and the Daisy SD data group. Optional source-series resistor footprints near each clock driver, typically evaluated in the 22-47 ohm range, would make edge-rate tuning possible.

The all-layer antenna keepout is a major improvement. Its final validity still depends on the exact ESP32 module/dev-board antenna geometry, orientation, enclosure metal, and wiring. Keep the antenna region away from metal and preferably overhang it at the enclosure/PCB edge according to the selected module guidance.

## Manufacturing and Sourcing

- Four nominal 100 uF capacitors use generic 1206 footprints. Exact technology, polarity, voltage rating, effective capacitance, and leakage are unresolved.
- FB1 and FB2 require exact current rating, DCR, and impedance-versus-frequency curves.
- D1 and all other LEDs require exact forward-voltage/current qualification.
- SD sockets and socketed module headers need exact mechanical parts and assembly notes.
- Six schematic jacks (`J6`-`J11`) are absent from the PCB and treated as intentional off-board hardware. Mark exclusion and harness intent explicitly.
- The available Gerber archives contain files dated 2026-08-10 and must not be ordered as this revision.

## Thermal Analysis

The thermal analyzer ran at 40 C but assessed zero components because no part has both quantifiable dissipation and usable MPN/datasheet data. This is a review gap, not evidence of low temperature. During sustained Wi-Fi transmission, dual-SD access, and heavy DSP load, measure the ESP32 and Daisy module regulators, FB1/FB2, SD cards, and enclosure hot spots.

## PDN Impedance

The analytical model remains useful only as a screen because capacitor ESR/ESL, DC-bias derating, ferrite curves, module-local capacitance, and source impedance are unknown. Prior runs indicated small possible anti-resonance regions around 5 MHz on VCC_Daisy and 7.9 MHz on VCC_ESP32. These frequencies are not accepted as part-specific predictions until the exact network is modeled or measured.

## Power Budget

The schematic analyzer's generic model estimates roughly 120 mA for the six analog ICs on VCC_Daisy and about 20 mA for U3 on +5 V. It excludes the Daisy processor, ESP32 transmit peaks, SD write transients, LEDs, and external loads, so it is not a complete supply budget. Size the 5 V input and both ferrites from measured simultaneous peak load with margin.

## Sleep Current Audit

The resistor/indicator heuristic reports a 69.84 mA worst case and approximately 0.59 mA state-aware estimate, excluding realistic module sleep behavior. The design is not presently a battery-optimized power architecture. If battery operation matters, define hardware rail-off states and measure them rather than relying on this estimate.

## Bus Topology

The board uses UART between ESP32 and Daisy, one native-width Daisy SD interface, one ESP32 SPI-mode SD interface, two audio input/output channels, and two expression ADC channels. The clocks are the primary digital aggressors, but the relocated ESP32 SD data/control transitions also need local return-path attention because fast edges are not limited to clock-labelled nets.

## Test Coverage

Nineteen schematic test points map to approximately 11% of physical PCB nets. Audio, VREF, rail, and GND access is useful. Add accessible UART, both SD clocks, and representative SD data/chip-select pads for production bring-up and noise correlation.

## Assembly Complexity

Raw placement complexity is moderate: 82 front-side SMD components, eight detected THT footprints, socketed modules, and two microSD sockets. The latest via moves eliminate reported VP-001 cases. The main repeatability risks are missing global fiducials, unqualified package choices, Card1 edge clearance, module/socket mechanics, and stale production outputs.

## BOM Optimization

Passive-value reduction is not a priority. Preserve the intentional filter, bias, and isolation values. The useful BOM work is exact part qualification, consistent capacitor packages/technology, realistic bulk capacitance, and ferrite selection rather than reducing line count.

## Bench Acceptance Plan

1. Terminate each input with a pickup-like source impedance. Record 20 Hz-20 kHz FFT/noise with Wi-Fi off, associated-idle, continuous transmit, and maximum configured transmit power.
2. Repeat while continuously reading and writing each SD interface, then both together. Compare channels independently.
3. Probe VREF, VCC_Daisy, VCC_ESP32, and +5 V during the same tests. Look for burst-correlated ripple, ringing, or sustained VREF oscillation.
4. Drive 100 Hz, 1 kHz, and 10 kHz. Sweep input amplitude to clipping and record maximum clean peak-to-peak level and THD+N at each analog/converter stage.
5. Sweep 20 Hz-24 kHz and compare against the nominal 21.9 kHz output pole.
6. Load realistic worst-case effect chains and log audio callback overruns/CPU use while Wi-Fi and both SD interfaces are active.
7. Test boot, reset, firmware update, SD insertion/removal, cable hot-plug, and Daisy clock loss for pops.
8. Repeat in the final enclosure with the final shielded or twisted signal/return harness. Compare lid open/closed and antenna near/far from metal.

## Analyzer Verification

### Performed

- `analyze_schematic.py`: 112 components, 163 nets, 82 findings.
- `analyze_pcb.py --full --proximity`: 118 footprints, 104 nets, 656 segments, 95 vias, zero unrouted nets.
- `cross_analysis.py`: four findings.
- `analyze_emc.py`: 120 findings, 46/100 risk score.
- `analyze_thermal.py` at 40 C: ran, but assessed zero components because no quantifiable part data were available.
- Raw schematic/PCB checks: antenna keepout, critical route lengths, transition-to-GND-via distances, VREF isolation absence, LED value, module/socket placement, and current working-tree hash.
- Prior-run delta: compared against `analysis/2026-08-19_return_check/`.

### Evidence basis

- **Raw-file checked:** antenna keepout restrictions/geometry, R24 absence, R32 value, layer stack, current routing geometry, board dimensions, and the latest R22/R23 via moves.
- **Analyzer-derived and reviewed:** route lengths, return-via distances, fill ratio, local bypass distances, connectivity, DFM, and warning counts.
- **Inference-only:** absolute noise floor, clipping level, ferrite attenuation, thermal margin, EMC compliance margin, and final product latency.
- **No manufacturer-level verification:** DS-001 applies because MPN coverage and the project datasheet cache are both absent.

## Reviewer Overrides

- The 69 GP-001 findings are not accepted literally as independent plane failures; local crossings still require native DRC and visual inspection.
- `/SD_SCK` at 1.09 mm from a GND via is a threshold miss, not an error-level release blocker.
- Card1/Card2 shell tabs are not thermal exposed pads; TV-001 is dismissed as a classification false positive.
- Many DC-003 findings associate signal/filter capacitors with IC decoupling. The dedicated 100 nF supply bypass capacitors 2.2-3.1 mm from the analog ICs are the relevant evidence. C31's approximately 3.3 mm GND-via path remains worth improving.
- The six missing jack footprints are treated as intentional off-board jacks, contingent on explicit exclusion/harness documentation.
- The reported +3.3VA plane split is on the power layer; In1.Cu remains the primary signal reference. It still warrants visual inspection but is not by itself proof of a broken audio return.

## Not Performed / Review Limits

- Native KiCad ERC/DRC was not run because `kicad-cli` is unavailable and no saved report exists.
- SPICE was not run because ngspice, LTspice, and Xyce are unavailable.
- Current Gerber analysis was not run because the present PCB has no current fabrication export; existing archives predate the revision.
- Datasheet extraction and lifecycle audit were blocked by 0% structured MPN coverage and no project datasheet cache.
- Thermal analysis produced no model because component dissipation and part thermal data are unavailable; zero thermal findings does not mean zero thermal risk.
- Static analysis cannot determine absolute noise, THD+N, clipping, callback CPU margin, enclosure coupling, or regulatory compliance.

## Required Closeout Order

1. Correct D1/R32 and select exact LEDs.
2. Restore/validate VREF output isolation, shorten VREF, and add VREF plus `/AUDIO_OUT_2` return vias.
3. Add optional SD clock source damping and improve return vias for the relocated ESP32 SD bus.
4. Populate exact MPN/manufacturer/datasheet fields, especially modules, ferrites, bulk capacitors, LEDs, and sockets.
5. Add fiducials and confirm Card1 mechanical/panel clearance.
6. Refill zones and save clean native ERC/DRC evidence.
7. Generate and inspect current Gerber, drill, BOM, and CPL outputs.
8. Pass the full audio/Wi-Fi/SD/headroom test matrix before calling the platform performance-complete.