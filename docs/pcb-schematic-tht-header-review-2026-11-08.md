# PCB and Schematic Review — THT Header Conversion

**Date:** 2026-11-08  
**Design:** `hardware/boards/daisyBoard.kicad_sch`, `hardware/boards/daisyBoard.kicad_pcb`  
**Scope:** J1–J4 and J13–J16 conversion from SMD to through-hole headers/sockets, schematic wiring, signal integrity, return paths, and route lengths.

## Verdict

**The THT conversion itself passes the electrical-connectivity review, but the complete design remains a conditional no-go for fabrication.**

All eight converted headers retain their previous pad-to-net assignments, their schematic and PCB footprints agree, and native KiCad DRC reports zero unconnected items. The new THT routing also improves the Daisy SDMMC and expression-ADC layer transitions substantially.

Before fabrication:

1. Resolve the three native ERC errors and the 0.0381 mm dangling wire stub.
2. Reduce the nominal 5 V expression-divider output below 3.3 V with tolerance and transient margin.
3. Fix or intentionally connect the dangling GND via at `(118.12, 132.18)`.
4. Review the remaining ESP SD MOSI return transition and B.Cu reference changes.
5. Assign exact JLCPCB/LCSC orderable parts and verify pin length, body height, drill fit, and assembly orientation. No MPN or LCSC number is presently stored.

## THT Conversion Assessment

| Check | Result | Evidence |
|---|---|---|
| J1–J4, J13–J16 schematic footprints changed to standard vertical THT footprints | Pass | Raw schematic and analyzer delta |
| Per-reference pad-to-net assignment before versus after conversion | Pass; all eight unchanged | Deterministic PCB analyzer comparison |
| Schematic footprint versus PCB footprint | Pass for all eight headers | Analyzer comparison |
| Routed connectivity | Pass | KiCad 10.0.3 DRC: zero unconnected items |
| THT courtyard within 100 mm × 100 mm board | Pass | All eight courtyard bounds remain inside `(50,50)`–`(150,150)` |
| Module row spacing | Plausible | ESP socket rows J1/J2 are 25.40 mm apart; Daisy socket rows J13/J14 are 15.24 mm apart |
| Exact purchased-part mechanical fit | Not verified | No MPN/C-number or supplier drawing |

The PCB visual-QA heuristic reports J2, J4, J14, and J15 as off-board. This is a rotation/bounding-box false positive: their actual courtyards remain inside the board, and native DRC reports no courtyard issue. Nevertheless, the exact connector body and mating height must be checked against the selected supplier parts.

THT barrels add a 1.6 mm plated stub and a larger pad discontinuity compared with SMD pads. At the present SD/UART route lengths and expected low-to-moderate clock rates, this is not by itself a signal-integrity blocker. It would need renewed review if these pins are driven with substantially faster interfaces or unusually fast edge rates.

## Priority Findings

| Priority | Finding | Impact | Required action | Confidence / source |
|---|---|---|---|---|
| **High** | Expression divider remains 10 kΩ / 20 kΩ from 5 V | Nominal ADC input is 3.333 V before resistor tolerance, rail variation, ESD, or hot-plug transients | Lower the divider ratio and add suitable clamp/transient protection | High; raw schematic and calculation |
| **High** | ERC has three power-output conflicts | Schematic cannot pass its configured electrical rules; power flags on U4 outputs are unnecessary/misleading | Remove or correct `#FLG03`, `#FLG09`, and the `#FLG08`/`#FLG010` duplicate pair; rerun ERC | High; native KiCad ERC |
| **Medium** | ESP SD MOSI has a layer transition about 11.25 mm from the nearest GND via | Enlarged return loop and greater edge ringing/EMI risk | Keep MOSI on F.Cu over In1 GND or add appropriate return/reference stitching near the transition | High; PCB coordinates |
| **Medium** | VBIAS remains 90.614 mm long with six vias | A noise-sensitive analog reference is distributed over three copper layers | Shorten where practical, keep it over continuous reference copper, and retain local bypassing at each load | High; PCB analyzer |
| **Medium** | 0.127 mm neck-downs remain on Daisy SD CMD/D0–D3 and VBIAS | High and uncontrolled local impedance; reduced fabrication margin | Widen to 0.20–0.254 mm where escape geometry allows and define explicit net classes | High; PCB analyzer |
| **Medium** | GND and GNDA names are joined implicitly | KiCad chooses GND in the netlist; schematic intent is ambiguous | Use one GND name, or use an explicit net tie if domains are intentionally separate | High; native ERC |
| **Medium** | GND via at `(118.12, 132.18)` is connected on only one layer | It does not provide the intended outer-layer stitching path in its present state | Connect it intentionally to GND copper/track on the relevant layer or remove it | High; native DRC |
| **Low** | Only the permissive Default net class exists | Routing intent and minimum widths are not protected | Add SD, analog/reference, power, and ordinary-signal net classes | High; project rules |
| **Low** | Card1/Card2 differ from their footprint-library copies | Later library updates can alter them unexpectedly | Update from library or document/lock the intentional local edits | High; native DRC |

## Trace-Length Review

Positive delta means the current route is longer than the September baseline.

| Net | Current length | Delta | Vias | Layers | Assessment |
|---|---:|---:|---:|---|---|
| `/SD_CLK_DAISY` | 38.001 mm | -3.694 mm | 1 | F/B | Improved; transition has GND via at ~0.87 mm |
| `/SD_CMD_DAISY` | 37.754 mm | -6.861 mm | 1 | F/B | Improved; transition has GND via at ~0.81 mm |
| `/SD_DAT_0_DAISY` | 30.892 mm | -3.574 mm | 1 | F/B | Improved; transition has GND via at ~0.81 mm |
| `/SD_DAT_1_DAISY` | 26.027 mm | -6.197 mm | 1 | F/B | Improved; transition has GND via at ~0.90 mm |
| `/SD_DAT_2_DAISY` | 39.550 mm | -0.570 mm | 1 | F/B | Improved; transition has GND via at ~0.81 mm |
| `/SD_DAT_3_DAISY` | 28.911 mm | -5.319 mm | 1 | F/B | Improved; transition has GND via at ~0.81 mm |
| `/SD_SCK` | 18.115 mm | +0.250 mm | 2 | F/B | Length is good; worst transition-to-GND-via distance is ~2.07 mm |
| `/SD_MISO` | 17.597 mm | -3.352 mm | 0 | F | Good |
| `/SD_MOSI` | 30.167 mm | +0.761 mm | 2 | F/B | Length is acceptable; one transition remains ~11.25 mm from GND |
| `/SD_CS` | 23.402 mm | -3.880 mm | 2 | F/B | Acceptable; worst transition-to-GND-via distance is ~3.48 mm |
| `/ESP_TO_DAISY` | 78.998 mm | -5.477 mm | 0 | F/B via THT pad | Acceptable for UART; no need to length-match |
| `/DAISY_TO_ESP` | 76.171 mm | +0.029 mm | 1 | F/B | Acceptable for UART |
| `/ADC_EXP_IN_1` | 72.788 mm | +0.193 mm | 1 | F/B | Transition improved from ~20.90 mm to ~2.76 mm from GND |
| `/ADC_EXP_IN_2` | 54.934 mm | +3.294 mm | 1 | F/B | Transition improved from ~17.71 mm to ~3.86 mm from GND |
| `/AUDIO_IN_1` | 67.411 mm | +2.824 mm | 0 | F | Acceptable; keep away from digital clocks |
| `/AUDIO_IN_2` | 62.237 mm | +0.419 mm | 1 | F/B | Acceptable |
| `/AUDIO_OUT_1` | 65.512 mm | +0.727 mm | 0 | F | Acceptable |
| `/AUDIO_OUT_2` | 68.433 mm | +0.467 mm | 1 | F/B | Acceptable |
| `VBIAS` | 90.614 mm | -0.927 mm | 6 | F/B/In2 | Still long and transition-heavy |
| `VREF` | 1.737 mm | New direct route | 0 | F | Good; prior series-capacitor fault is fixed |

### SDMMC Length Matching

The Daisy SDMMC group ranges from 26.027 mm to 39.550 mm, a maximum spread of 13.523 mm. CLK and CMD differ by only 0.247 mm. Using an approximate FR-4 propagation delay of 6–7 ps/mm, the full bus spread is roughly 81–95 ps. This is small compared with a 20 ns period at 50 MHz and is not a length-matching blocker.

The more important issue is reference continuity, not absolute matching. Every Daisy SD signal now uses only one ordinary routing via instead of two, and the remaining transitions are close to GND vias. This is a substantial improvement over the September layout.

### Reference Planes and Layer Changes

The stackup is F.Cu / In1.Cu / In2.Cu / B.Cu with approximately 0.48 mm dielectric spacing. In1.Cu is the broad GND reference; In2.Cu contains power zones. F.Cu signals therefore have the preferred adjacent GND reference, while B.Cu signals reference the power plane and rely on nearby power-to-ground decoupling when changing reference.

Automated cross-analysis again reports two VCC_Daisy islands and claims that the SD signals cross a VCC_Daisy gap. This is not evidence of an electrical open:

- native DRC reports zero unconnected items;
- the relevant high-speed F.Cu reference is the In1 GND plane, not VCC_Daisy on In2;
- the same heuristic was previously identified as over-aggressive.

However, B.Cu SD sections should still be routed over continuous In2 copper, and each F/B transition should have a short return path through nearby decoupling between the GND and power reference planes. A GND via alone does not bridge two different reference-plane voltages.

### Crosstalk Screen

No digital SD/UART proximity pair was flagged. The geometry screen found two analog pairs:

- `/ADC_EXP_IN_1` alongside `/ADC_EXP_IN_2` for approximately 4.0 mm on B.Cu;
- `/ADC_EXP_IN_1` alongside `/AUDIO_OUT_2` for approximately 1.5 mm on B.Cu.

These are heuristic coupling lengths, not measured crosstalk. Preserve spacing and continuous reference copper; increasing separation from the audio route is preferable if it can be done without creating extra layer transitions.

## Schematic Wiring Review

### Confirmed Correct or Improved

- Each converted connector keeps exactly the same pin number to net mapping as before the footprint conversion.
- Daisy Seed SDMMC, UART, audio, and ADC signal assignments remain coherent with the previous reviewed mapping.
- The prior REF2033 blocker is fixed:
  - U4 pin 4 IN and pin 3 EN connect directly to `VCC_Daisy`;
  - C43 is a shunt capacitor from `VCC_Daisy` to GNDA;
  - U4 pin 1 connects directly to `VBIAS`, with C31 from VBIAS to GNDA;
  - U4 pin 5 connects directly to `VREF`, with C42 from VREF to GNDA.
- C40 and C41 now both read 10 nF in the schematic and PCB; the previous 10 nF/1 nF mismatch is fixed.
- J6–J11 are intentionally schematic-only off-board jack representations; their cross-analysis warning is dismissed.
- `G***` and `REF**` are non-schematic graphic/placeholder footprints, not missing electrical components.

### Remaining Schematic Problems

Native KiCad ERC reports five violations:

- **Error:** U4 VREF output connected to power-output flag `#FLG09`.
- **Error:** U4 VBIAS output connected to power-output flag `#FLG03`.
- **Error:** power-output flags `#FLG08` and `#FLG010` connected together.
- **Warning:** GND and GNDA attached to the same items; GND is used in the netlist.
- **Warning:** one 0.0381 mm unconnected wire endpoint near the VREF/power-flag area.

The power-flag errors appear to be rule-modeling errors rather than proof that the REF2033 circuit is electrically disconnected. They should still be cleaned up so ERC can detect real future faults.

The expression input remains a functional concern:

\[
V_{ADC}=5\,\text{V}\frac{20\,\text{k}\Omega}{10\,\text{k}\Omega+20\,\text{k}\Omega}=3.333\,\text{V}
\]

This already exceeds a nominal 3.3 V rail and has no tolerance or transient margin.

## Native KiCad Validation

### DRC — KiCad 10.0.3

- Zero unconnected items.
- Zero courtyard violations.
- Warning: dangling/single-layer GND via at `(118.12, 132.18)`.
- Warning: Card1 and Card2 differ from their footprint-library copies.

The project currently ignores several DRC categories, including silkscreen clearance, silkscreen-over-copper, footprint-type mismatch, and track-endpoint centering. These ignored checks reduce sign-off coverage.

### ERC — KiCad 10.0.3

- Three errors.
- Two warnings.
- ERC does not pass.

## Delta from the September Review

| Previous issue | Current state |
|---|---|
| REF2033 capacitors wired in series | **Fixed**; direct rails plus shunt capacitors confirmed |
| C40/C41 schematic-PCB value mismatch | **Fixed**; both are 10 nF |
| Distant Daisy SD return vias | **Improved substantially**; one transition per net, ~0.81–0.90 mm from GND vias |
| Distant expression-ADC return vias | **Improved substantially**; ~2.76 mm and ~3.86 mm |
| UART return transitions | Still acceptable; lengths 79.0/76.2 mm |
| VBIAS long multi-layer route | **Still open**; 90.614 mm and six vias |
| Expression ADC over-range | **Still open** |
| GND/GNDA ambiguity | **Still open** and reported by native ERC |
| R8 via in pad | **Appears fixed**; nearest GND via is now outside the pad area |
| Explicit signal/power net classes | **Still open** |

## Fabrication and Review Limits

- Existing Gerbers under `hardware/boards/jlcpcb/gerber` predate this THT conversion and must not be used for ordering.
- No MPNs or LCSC numbers are stored, so JLCPCB stock, Basic/Extended status, drill-to-lead fit, body dimensions, and assembly orientation were not verified.
- No component datasheet cache was available. Electrical conclusions other than previously established Daisy/REF2033 behavior are raw-file consistency checks or engineering inferences.
- No SPICE simulator was available.
- Thermal analysis was skipped because no components had quantifiable power/datasheet data.
- EMC output is a risk screen, not a compliance prediction. The raw 46/100 score is dominated by duplicated plane-coverage heuristics and should not be treated as a lab pass/fail result.
- Static analysis cannot establish edge slew, ringing amplitude, ADC acquisition settling, THD+N, hot-plug stress, enclosure coupling, or regulatory compliance.

## Recommended Sign-Off Sequence

1. Correct the three ERC power-flag errors, GND/GNDA naming, and the tiny dangling wire; obtain a clean ERC.
2. Correct the expression-divider ratio and protection strategy.
3. Connect or remove the dangling GND via at `(118.12, 132.18)`.
4. Improve the MOSI transition return path and widen 0.127 mm SD neck-downs where possible.
5. Add explicit net classes and rerun zone fill, ERC, and DRC.
6. Assign exact JLCPCB/LCSC parts and verify connector drawings against drills, row spacing, body clearance, mating height, and pin-1 orientation.
7. Regenerate Gerbers, drill files, BOM, and CPL from the current sources; validate the generated fabrication set rather than the stale files.
8. On first hardware, inspect SD clocks with a short-ground-spring probe and test SDMMC/SPI at maximum intended clock while exercising audio and expression ADC inputs.
