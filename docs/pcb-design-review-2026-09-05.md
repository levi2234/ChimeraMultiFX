# ChimeraMultiFX Schematic and PCB Design Review

**Date:** 2026-09-05  
**Design:** `hardware/boards/daisyBoard.kicad_sch`, `hardware/boards/daisyBoard.kicad_pcb`, and `hardware/boards/daisyBoard.kicad_pro`  
**Revision basis:** Modified working tree derived from commit `bc405ad`  
**Intent:** Dual-channel Daisy Seed multi-effects processor with ESP32 control/Wi-Fi, two SD interfaces, expression inputs, and off-board audio jacks.

## Overview

The current design is a 100 mm x 100 mm, four-layer mixed-signal carrier with a Daisy Seed, ESP32, two microSD interfaces, dual audio input/output conditioning, and two expression-pedal interfaces. The PCB analyzer found 138 footprints, 116 PCB nets, 799 track segments, 127 through-vias, five zones, and zero unrouted nets.

This review was performed against the live modified KiCad sources, not the August fabrication package. Static schematic, PCB, cross-domain, EMC, thermal, stale-Gerber, and prior-run delta analyses were supplemented by raw-file checks and a manual review of the REF2033 manufacturer datasheet.

## Final Verdict

**No-go for fabrication in the current state.**

The routed board is a credible prototype design, and the audio signal topology is generally coherent. However, U4's floating enable can disable the new VBIAS source, the schematic and PCB do not encode the same ground-net strategy, four ordinary through-vias sit inside pasteable SMD pads, and the available fabrication outputs predate the redesign. Native KiCad ERC/DRC evidence is also unavailable.

After those release blockers are fixed, the principal performance risks are reference-rail noise pickup, incomplete high-speed return stitching, expression-interface electrical protection, and unqualified component choices.

### Readiness

| Target | Assessment |
|---|---|
| Bench prototype | **Not ready as drawn** because U4 may remain in shutdown |
| New PCB order | **No-go** until blockers are corrected and release outputs are regenerated |
| Repeatable product | **Not demonstrated**; MPN coverage is 0% and thermal/power margins are unqualified |
| FCC/CE release | **Not demonstrated**; the EMC score is a static risk screen, not a compliance result |

## Critical Findings

| Priority | Finding | Consequence | Required action | Evidence |
|---|---|---|---|---|
| Critical | U4 REF2033 EN pin 3 is explicitly unconnected | U4 can remain in shutdown, making VREF/VBIAS high impedance and disabling all VBIAS-referenced analog stages | Tie EN to a valid high level, normally U4 VIN for always-on operation | Datasheet-verified and raw-file checked |
| High | Schematic uses separate `GNDA` and `GNDD`; PCB has only `GND` and no net-tie | Design intent is not represented consistently; Update PCB from Schematic can split or rename ground connectivity | Use one schematic ground net for a single-plane design, or add an intentional net-tie and enforce the partition | Raw-file checked |
| High | Current fabrication outputs do not represent the live board | Ordering the existing package would fabricate an older design | Regenerate Gerbers, drills, BOM, and placement files after all fixes | Analyzer-derived and file-date checked |
| High | No current native KiCad ERC/DRC result | Static parsing cannot exclude native connectivity, clearance, courtyard, or zone errors | Refill all zones, update PCB from schematic, and archive clean ERC/DRC reports | Review gap |
| High | MPN coverage is 0/107 fitted BOM components | Package, pinout, ratings, lifecycle, derating, and assembly choices are not repeatable | Populate manufacturer, MPN, and datasheet fields, then rerun part-level review | Analyzer-derived |
| Warning | U4 has no local VIN, VREF, or VBIAS bypass capacitor | The long reference distribution is more susceptible to transient and RF noise | Place low-ESR 100 nF ceramics at VIN, VREF, and VBIAS near U4 | Datasheet-verified and raw-file checked |
| Warning | Four ordinary through-vias are inside SMD paste pads | Solder can wick into the via and leave weak or voided joints | Move vias out of pads or specify filled and capped via-in-pad construction | Raw-file checked |
| Warning | Expression ADC scaling reaches 3.333 V from a 5 V input | Nominal input already exceeds a 3.3 V ADC rail before tolerances; external tips lack explicit clamps | Reduce divider ratio and add input clamping/transient protection | Raw-file checked and calculated |
| Warning | SD, UART, audio, and VBIAS layer transitions lack nearby return vias | Larger return loops increase emission and susceptibility risk | Add adjacent GND vias or keep each route on one layer | Analyzer-derived and coordinate checked |

## Previous Review Delta

The change from the 2026-08-19 baseline is major: the schematic delta contains 141 changes and the PCB delta contains 71. The PCB gained approximately 20 footprints, 32 vias, and 12 nets relative to that review.

| Status | Item | Current state |
|---|---|---|
| Improved | Shared midpoint reference architecture | MCP6002 midpoint buffer was replaced by a purpose-built REF2033 with separate 3.3 V VREF and 1.65 V VBIAS outputs |
| Improved | Routing completeness | Current analyzer reports zero unrouted nets |
| Improved | Main ground plane | In1.Cu GND fill is broad at 93.7% of its zone outline |
| Regressed | Reference enable | New U4 EN is unconnected, creating a direct functional failure |
| Regressed | Reference bypass/layout | VBIAS is 90.37 mm with six vias; U4 lacks the manufacturer's recommended local input/output bypassing |
| Regressed | Via-in-pad | Four new ordinary through-via cases are present under pasteable SMD pads |
| Open | Part qualification | Structured MPN coverage remains 0% |
| Open | Release evidence | Existing Gerbers still predate the current board |

## Component Summary

The schematic analyzer reports 132 non-power components across 120 nets and 45 unique part groups.

| Type | Count |
|---|---:|
| ICs | 7 |
| Resistors | 47 |
| Capacitors | 38 |
| Connectors/modules | 14 |
| Test points | 19 |
| LEDs | 5 |
| Ferrite beads | 2 |

There are nine explicit no-connects. U4 EN is the only no-connect shown to be functionally unsafe by manufacturer documentation. Six off-board audio jack symbols, J6-J11, intentionally have no PCB footprints but should be marked with explicit exclusion/harness documentation.

## Power Tree

```text
External +5 V
|-- FB2 -> VCC_Daisy
|   |-- Daisy module and analog op amps
|   |-- U4 REF2033 VIN
|   |   |-- VREF = 3.3 V (currently unloaded except for its label)
|   |   `-- VBIAS = 1.65 V -> audio-stage non-inverting inputs
|   `-- expression ring-drive buffers
`-- FB1 -> VCC_ESP32
    `-- ESP32 module -> ESP-3.3V
```

The exact source current, ferrite impedance/DCR, module regulator behavior, bulk-capacitor derating, and simultaneous Wi-Fi/SD/DSP peak margin remain unknown because exact parts and measurements are unavailable.

## REF2033 Review

U4 was manually checked against TI document SBOS600E, revised February 2022.

- Pin mapping is correct: pin 1 VBIAS, pin 2 GND, pin 3 EN, pin 4 VIN, pin 5 VREF.
- REF2033 outputs are nominally 1.65 V and 3.3 V.
- Recommended VIN is at least VREF + 20 mV and no more than 5.5 V; the nominal 5 V VCC_Daisy supply is suitable.
- Both outputs can source or sink 20 mA, comfortably above the identified op-amp input-bias loading.
- Stable output capacitance is specified from 0 to 10 uF, so output capacitors are not required solely for stability.
- TI nevertheless recommends low-ESR 0.1 uF capacitors at VIN, VREF, and VBIAS, close placement, and short reference/bias traces to reduce noise pickup.
- EN must be pulled high for active mode and low for shutdown. The datasheet does not document floating EN as a valid active state.

The nearest 100 nF VCC_Daisy capacitor identified near U4 is C26 at approximately 20.4 mm. C32 is not U4 bypassing; it connects `+3.3VD` to ground and is approximately 50.7 mm away. Add dedicated local capacitors rather than relying on distributed rail capacitance.

VBIAS is 90.37 mm long, changes layers through six vias, and has nearest ground-via distances of approximately 2.30-6.81 mm at its transitions. This is legal copper connectivity but conflicts with the manufacturer's noise-sensitive layout guidance. VREF currently has no functional load; either remove it from the design or provide a local test/load purpose explicitly.

## Signal Analysis

### Audio Inputs

Each input follows this nominal chain:

```text
jack -> 10 uF AC coupling -> 1 Mohm VBIAS bias -> OPA1652 follower
     -> 3.3k/4.7k attenuation -> 10 uF AC coupling
     -> OPA1652 inverting stage -> 100 ohm/33 nF ADC filter
```

The nominal divider gain is:

$$
A_{divider} = \frac{4.7\text{k}}{3.3\text{k}+4.7\text{k}} = 0.5875
$$

The following inverting stage is approximately -1 V/V, giving an overall midband magnitude near 0.588 V/V. The 10 kOhm/330 pF feedback pole and 100 Ohm/33 nF ADC pole are both approximately 48.2 kHz. The interstage 10 uF/12 kOhm high-pass corner is approximately 1.33 Hz. These values are coherent for guitar-band operation.

Risks are connector-borne ESD/transients and unmeasured headroom. The OPA1652 stages run from 5 V while the signals are centered at 1.65 V, not at half of the op-amp supply. Confirm clipping onset at every analog and converter node.

### Audio Outputs

Each output uses a 15 kOhm input resistor and 33 kOhm feedback resistor, for nominal gain:

$$
A_v = -\frac{33\text{k}}{15\text{k}} = -2.2
$$

The 100 pF feedback pole is approximately 48.2 kHz. The 100 Ohm output resistor is useful for cable-capacitance isolation. The main concern is 2.2x gain headroom on a 5 V supply; measure maximum undistorted output into the intended load.

### Expression Inputs

Each expression channel uses an MCP6002 follower to drive the pedal ring from 5 V through 100 Ohm and another follower to buffer the tip. A 10 kOhm/20 kOhm divider scales the tip for the Daisy ADC:

$$
V_{ADC} = 5\text{ V}\frac{20\text{k}}{10\text{k}+20\text{k}} = 3.333\text{ V}
$$

This has no positive overvoltage margin against a nominal 3.3 V ADC rail. The effective 6.67 kOhm source resistance with 1 nF gives an approximately 23.9 kHz low-pass pole. Increase protection margin, add explicit clamps suitable for the ADC, and verify the source impedance against the Daisy ADC acquisition requirement.

A grounded or shorted expression ring requests 50 mA through the 100 Ohm series resistor. That likely forces the MCP6002 into current limiting. Increase the resistance or use a protected/current-limited ring supply if shorts during insertion are expected.

R47 and R48 appear to connect GNDA to GNDA and are electrically ineffective. Confirm whether they were intended as sleeve isolation, configurable links, or net ties.

## Grounding and Power Integrity

The physical PCB uses a unified ground strategy: In1.Cu has one broad GND zone with 93.7% fill, and the upper In2.Cu GND zone has 75.2% fill. This is a sound starting point for a mixed-signal audio board when return-current geometry is controlled.

The schematic instead contains distinct `GNDA`, `GNDD`, and `GND` global nets. No net-tie exists, while PCB pads that correspond to analog and digital ground are assigned to `GND`. This is not a cosmetic naming difference. The design must encode one deliberate strategy before the next schematic-to-PCB update.

Cross-domain connectivity analysis also reports two VCC_Daisy islands with five named SD signals crossing the affected region and three VCC_ESP32 islands. These findings may reflect stale stored zone fills or intentional local routed connections rather than broken DC power. Refill zones in KiCad and inspect the connectivity/ratsnest before changing copper; do not waive the finding without that check.

## PCB Layout and Signal Integrity

### Board and Rules

- Board: 100 mm x 100 mm with rounded corners.
- Copper layers: F.Cu, In1.Cu, In2.Cu, and B.Cu.
- Routing: 2,031.63 mm total track length, 799 segments, 127 vias, zero analyzer-unrouted nets.
- The project defines only the `Default` net class: 0.127 mm clearance, 0.254 mm track, and 0.686/0.330 mm via/drill.
- Fourteen nets contain 0.127 mm neck-down sections. These fit the configured minimum but spread a permissive rule to analog, SD, and power escape geometry.

Add explicit power, analog, SD/high-speed, and general-signal net classes so width and clearance intent is enforced rather than remembered.

### Layer Transitions and Returns

Measured nearest GND-via distances at representative transitions include:

| Net | Route length / vias | Worst measured GND-via distance | Assessment |
|---|---:|---:|---|
| VBIAS | 90.37 mm / 6 | 6.81 mm | Noise-sensitive reference; improve all transitions |
| `/SD_CLK_DAISY` | 41.70 mm / 2 | 7.40 mm | High priority; one transition is good at 0.87 mm |
| `/SD_SCK` | 17.87 mm / 2 | 2.86 mm | Improve one transition; other is 1.09 mm |
| `/SD_MOSI` | 29.41 mm / 2 | 11.25 mm | High priority |
| `/SD_MISO` | 20.95 mm / 2 | 5.05 mm | Improve |
| `/DAISY_TO_ESP` | 75.59 mm / 2 | 12.11 mm | Improve both UART transitions |
| `/ESP_TO_DAISY` | 84.26 mm / 2 | 9.96 mm | Improve both UART transitions |
| `/AUDIO_IN_2` | 61.82 mm / 2 | 8.05 mm | Improve channel 2 transition |
| `/AUDIO_OUT_2` | 67.97 mm / 2 | 3.57 mm | Improve channel 2 transition |

Place a GND via close to each signal via, preferably within approximately 1 mm, or keep the signal on one layer. The EMC analyzer's clock and return-path categories are directionally useful; its 28/100 score is not a compliance probability.

The UART traces also run close enough in parallel to trigger one crosstalk warning. Their data rate may make this tolerable, but additional spacing or a grounded guard is inexpensive if routing permits.

### Via-in-Pad

The following are ordinary 0.686/0.330 mm through-vias inside SMD paste pads:

| Location | Geometry | Disposition |
|---|---|---|
| J2 pad 10 | Via 0.38 mm from center, inside 1.9 mm x 1.0 mm pad | Move or fill/cap |
| R46 pad 1 | Via centered in pad | Move or fill/cap |
| R50 pad 1 | Via approximately 0.065 mm from center | Move or fill/cap |
| R8 pad 2 | Via centered in pad | Move or fill/cap |

Tenting alone is not a reliable substitute beneath an open paste aperture.

### Placement and Assembly

- No front-side global fiducials were recognized for 110 SMD footprints. Add three or obtain an explicit assembler waiver/panel-fiducial plan.
- Card1 is approximately 0.82 mm from the edge and J15 approximately 0.58 mm. These can be intentional connector placements, but body overhang, panel rails, and depaneling clearance need mechanical confirmation.
- Test points cover 13 of 112 signal nets, approximately 12%. Add accessible UART, both SD clocks, representative SD data/chip-select, U4 EN, VBIAS, and key rails based on the intended production-test strategy.

## EMC and Cross-Domain Review

The EMC analyzer returned 92 findings and a 28/100 risk score: 21 errors, 48 warnings, and 23 informational items. Most are heuristic geometry observations rather than independent faults.

The useful priorities are:

1. Correct the missing return stitching listed above, especially SD clocks/data, UART, and VBIAS.
2. Resolve or disprove the stored VCC_Daisy plane split after a zone refill.
3. Keep SD clocks away from unrelated connector routes where practical.
4. Add connector-level ESD/transient protection to external expression and audio paths based on the product's test target.
5. Preserve the ESP32 antenna keepout and validate it against the exact selected module and enclosure.

Thirty-five GP-001 plane-coverage findings are not treated as 35 independent ground failures because the raw board contains a broad In1.Cu GND fill. Fourteen DC-003 findings also include signal/filter capacitors that are not supply bypasses. U4's DC-001 warning is accepted because direct coordinate and net checks show no local bypass capacitor.

## Manufacturing, Sourcing, and Test

The stale fabrication package is complete enough to analyze but not current enough to order. It contains 11 Gerbers, two drill files, and 242 holes, but represents only 92 components and 84 vias versus the live PCB's 138 footprints and 127 vias. Two reported layer-extent warnings can arise from naturally different copper occupancy and are not blockers by themselves; the age and design mismatch are decisive.

Exact ferrites, bulk capacitors, LEDs, sockets, connectors, modules, and protection parts remain unspecified. Populate MPNs before attempting BOM pricing, lifecycle review, package verification, voltage/temperature derating, or production assembly validation.

## Thermal and Power Limits

The thermal analyzer ran at 40 C but assessed zero components because no component had both quantifiable dissipation and usable part data. Zero findings do not mean zero thermal risk.

The current design has no complete source-current budget for simultaneous Daisy processing, ESP32 transmit, both SD interfaces, LEDs, and expression-ring loads. Measure rail droop and temperature during that worst-case combination. Particular targets are the external 5 V source, FB1/FB2, module regulators, SD cards, and op amps under cable faults.

## Analyzer Verification

### Performed

- `analyze_schematic.py`: 132 components, 120 nets, 75 findings.
- `analyze_pcb.py --full --proximity`: 138 footprints, 116 nets, 799 segments, 127 vias, zero unrouted nets.
- `cross_analysis.py`: five findings.
- `analyze_emc.py`: 92 findings, 28/100 risk score.
- `analyze_thermal.py` at 40 C: ran, zero components assessable.
- `analyze_gerbers.py`: run against the available stale production package.
- Schematic, PCB, and EMC delta analyses against the 2026-08-19 baseline.
- Raw-file checks: counts, dimensions, layer stack, U4 connectivity and placement, ground-net naming, via-in-pad geometry, zone fills, critical route lengths, and return-via distances.
- Manufacturer PDF check: TI REF2025/REF2030/REF2033/REF2041 datasheet SBOS600E for U4 pinout, enable behavior, supply range, loading, capacitance, and layout guidance.

### Evidence Basis

- **Datasheet-verified:** U4 REF2033 only.
- **Raw-file checked:** U4 and C32 connectivity, ground-net mismatch, via-in-pad geometry, rules, dimensions, layers, and selected analog values/topologies.
- **Analyzer-derived and reviewed:** counts, route lengths, return-via distances, fill ratios, DFM findings, and deltas.
- **Inference-only:** absolute noise, headroom, rail stability, thermal margin, ESD immunity, EMC compliance, and product reliability.
- **Unverified:** all other component-specific pinouts and electrical ratings because structured MPN coverage is 0%.

## Reviewer Overrides and False Positives

- J6-J11 are treated as intentional off-board jacks, not missing board parts, contingent on explicit BOM/harness exclusion.
- The extra PCB `REF**` item appears non-schematic/mechanical and is not treated as a circuit defect.
- Card1/Card2 shell tabs are not thermal exposed pads; the four TV-001 findings are dismissed as classification false positives.
- GNDA-to-PCB-GND mappings are not reported as 11 independent pin swaps. They are one architecture/synchronization issue.
- The 35 GP-001 findings are condensed into local return-path concerns because a broad In1.Cu GND plane exists.
- Power-island findings remain unresolved pending a native zone refill; they are not accepted as proof of open DC rails.
- Edge-placement findings for Card1/J15 require mechanical confirmation rather than automatic relocation.

## Not Performed / Review Limits

- Native KiCad ERC/DRC was not run because `kicad-cli` is unavailable and no saved current report exists.
- SPICE simulation was not run because ngspice, LTspice, and Xyce are unavailable.
- Current-release Gerber validation was not possible because only stale fabrication outputs exist.
- Broad datasheet extraction and component lifecycle audit were blocked by 0% MPN coverage and no project datasheet cache.
- No deep per-IC datasheet review was possible beyond U4.
- Thermal analysis had no usable part models.
- Static review cannot determine absolute noise, THD+N, clipping, firmware callback margin, enclosure coupling, hot-plug behavior, or regulatory compliance.

## Required Closeout Order

1. Tie U4 EN high and add local 100 nF VIN, VREF, and VBIAS capacitors.
2. Make the schematic and PCB ground strategy consistent; then update PCB from schematic.
3. Reduce expression ADC full-scale voltage, add suitable clamps, and address expression-ring short current.
4. Move J2:10, R46:1, R50:1, and R8:2 vias out of paste pads or specify filled/capped vias.
5. Refill zones and resolve the VCC_Daisy/VCC_ESP32 island and SD-clock crossing reports.
6. Add return vias at critical transitions and shorten VBIAS where practical.
7. Add net classes, global fiducials, and the required production/bring-up test points.
8. Populate exact MPN/manufacturer/datasheet fields and complete part-level pinout, rating, lifecycle, and derating checks.
9. Run and archive native clean ERC and DRC results.
10. Regenerate and independently inspect Gerber, drill, BOM, and placement outputs.
11. Bench-test rail droop, U4 outputs, noise/FFT, clipping, Wi-Fi plus dual-SD interference, hot-plug faults, and temperatures before release.