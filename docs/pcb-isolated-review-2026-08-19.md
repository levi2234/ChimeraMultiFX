# ChimeraMultiFX Isolated PCB Review

**Date:** 2026-08-19  
**Scope:** Current `daisyBoard.kicad_sch`, `daisyBoard.kicad_pcb`, firmware hardware contract, and current fabrication outputs only  
**Isolation rule:** No prior review, prior analyzer output, backup, history, or review delta was opened or used  
**Verdict:** **Not ready to release for fabrication/assembly.** The routed PCB is electrically connected and native KiCad DRC is nearly clean, but the available manufacturing package is stale and several circuit, robustness, and verification items remain open.

## Overview

This review treats the current KiCad source as a new design and evaluates it against the requirements of a dual-MCU, Wi-Fi-enabled audio multi-effects pedal. It covers schematic topology, PCB connectivity/layout, signal integrity, EMC risk, fabrication outputs, assembly, testability, and mechanical integration. Findings from existing reviews or historical analyzer runs were deliberately excluded.

## Critical Findings

| Priority | Finding | Evidence | Required action |
|---|---|---|---|
| BLOCKER | Manufacturing package does not represent the current design | Current schematic/PCB were modified Aug 19; release ZIP is Aug 16. Gerber contains 92 unique component refs while current PCB analysis finds 109 schematic-linked footprints. Release BOM still names J5/J12 while the PCB uses Card1/Card2, and contains obsolete values including C14=0.1uF versus current 10uF and R13-R17=10K versus current 47K. | Regenerate BOM, CPL, Gerber, drill, and assembly drawings from the final source. Review a release diff before ordering. |
| HIGH | VREF startup is excessively slow for a pedal | R22/R23 are 100k/100k. C15 and C19 are each 100uF and C20 is 100nF on the divider midpoint. The Thevenin time constant is approximately `(100k || 100k) * 200.1uF = 10 s`; 90% settling is about 23 s. | Reduce divider impedance/capacitance, or add a deliberate audio mute until VREF settles. Verify startup and power-down pop behavior on hardware. |
| HIGH | D1 is likely overdriven | D1 is a generic green 0201 LED from ESP-3.3V through R32=27 ohm. With an assumed 2.2 V forward drop, current is about 41 mA. No exact LED MPN/rating is present. | Select an exact LED and resistor from its rated current/brightness. About 110 ohm gives roughly 10 mA at 2.2 V; modern indicators often need less. Review D2/D3 at 150 ohm from 5 V as well. |
| HIGH | No explicit ESD protection is present on off-board pedal interfaces | J6-J11 are off-board audio/expression jacks with no PCB footprint. No TVS/ESD devices were found on audio, expression, or panel-entry nets. | Add low-capacitance clamps and suitable series impedance at the board entry, with a short clamp return. Validate against cable-discharge and IEC 61000-4-2 targets appropriate to the enclosure. |
| HIGH | Component identity is insufficient for a production release | MPN coverage is 0/36 unique BOM groups. Ferrite beads, LEDs, connectors, and passives are generic. Manufacturer pinout, ratings, ferrite current/DCR, and derating cannot be established from source properties. | Populate manufacturer and MPN properties, attach datasheets, then rerun pinout, electrical, thermal, and lifecycle checks. |
| WARNING | SD clock/data return paths are weak | Both SD clocks are outer-layer traces. `/SD_SCK` is 17.9 mm with one layer transition; `/SD_CLK_DAISY` is 39.2 mm with one transition. No nearby ground stitching via was found at either transition; several SD data nets show the same condition. | Add a GND return via close to every SD signal transition, especially clocks. Consider source-series damping after measuring edge rate/ringing. Keep clocks over continuous GND. |
| WARNING | Current PCB has one dangling GND via | KiCad DRC: GND via at (118.6, 140.2) is connected on only one layer. | Connect or remove the via, refill zones, rerun DRC. |
| WARNING | Placed TF-01A footprints differ from the current library copy | KiCad DRC reports Card1 and Card2 footprint-library mismatches. Pad geometry in the placed copies is consistent with the checked local footprint, but library metadata/formats have drifted. | Diff and reconcile the footprint intentionally; verify C91145 land pattern against the manufacturer drawing before accepting any library update. |
| WARNING | Assembly/test provisions are incomplete | No board fiducials were detected for 83 SMD parts; signal-net test coverage is 11/101 (11%). | Add three global fiducials or obtain written confirmation that panel fiducials satisfy the assembler. Add production test access for UART, SD, boot/reset, 3.3 V rails, and critical analog nodes. |
| WARNING | Module/enclosure fit is not proven | Card2 lies between the ESP32 headers near the antenna end; an M3 hole at (134.21, 132.45) lies under the Daisy module envelope. Module 3D models and enclosure stack-up are absent. | Check insertion/removal access for Card2, ESP antenna clearance, screw-head/standoff clearance under Daisy, and all panel-jack harness paths in the enclosure CAD or a physical stack-up. |

## Use-Case Assessment

This is a 100 mm x 100 mm, four-layer dual-MCU multi-effects pedal board. The Daisy Seed handles 48 kHz stereo audio; the ESP32 provides Wi-Fi/UI control and communicates with the Daisy over 115200-baud UART. The board also carries two microSD interfaces and two expression-pedal channels.

For this use case, the design priorities are low audio noise, quiet startup/shutdown, robust cable-connected I/O, predictable RF performance, and reliable removable-media operation. The current design is promising in partitioning and connectivity, but VREF startup, ESD, manufacturing release control, and SD return paths need attention before it is a durable stage product.

## Component Summary

| Type | Schematic count |
|---|---:|
| ICs | 7 |
| Resistors | 32 |
| Capacitors | 34 |
| Connectors, including off-board jacks | 14 |
| Test points | 19 |
| LEDs | 5 |
| Ferrite beads | 2 |
| **Total** | **113** |

The PCB contains 119 footprints, including board-only mounting/logo items, 83 SMD footprints, 8 through-hole component footprints, 105 nets, and 97 vias. The schematic has 39 unique part groupings and no DNP entries. Controlled MPN coverage is 0%, which is the dominant component-verification gap.

## Signal Analysis Review

### Power Tree

```text
External +5V
  +-- FB2 --> VCC_Daisy --> Daisy Seed + TL072 IC1-IC4 + MCP6002 U1/U2
  +-- FB1 --> VCC_ESP32 --> ESP32 module input

Daisy module outputs
  +-- +3.3VA --> expression reference buffers U1A/U2A
  +-- +3.3VD --> Daisy microSD + LEDs D4/D5

ESP32 module output
  +-- ESP-3.3V --> ESP microSD + LED D1

+5V --> R22/R23 midpoint --> U3A buffer --> R24 --> VREF
```

The topology is internally coherent. Bulk plus 100 nF bypassing exists on the major rails. A single continuous ground plane is preferable for this mixed-signal board, and the PCB does use a broad In1 GND plane. The schematic mixes `GND` and `GNDA`; KiCad reports both names on the same items and resolves the net to GND. Normalize the naming so a future edit cannot accidentally create a real split.

The supply source must handle simultaneous Daisy processing and ESP32 Wi-Fi current peaks. Because FB1/FB2 have no exact MPNs, their DC resistance, rated current, and impedance curve are unknown. Select beads with adequate peak-current margin and verify droop at the module pins during Wi-Fi transmit plus worst-case DSP load. The external 5 V source and connector should be budgeted for both modules, LEDs, SD cards, and analog stages with margin.

### Audio Path

The two audio channels are symmetric and VREF-biased. The detected passive values are consistent with full-band audio:

- Input AC coupling uses 1 uF with 1 Mohm bias, giving a sub-hertz high-pass corner.
- The 1 kohm/1 nF input network gives an ultrasonic low-pass corner near 159 kHz, not 159 Hz.
- Output filtering uses 3.3 kohm with 2.2 nF, approximately 21.9 kHz.
- Output AC coupling uses 1 uF with 100 kohm discharge/bias resistance, approximately 1.6 Hz.

The automated RC detector reported several misleading pairwise corners; the topology-level interpretation above supersedes those detections. TL072HIDR operation on a single 5 V rail is plausible but has limited input/output headroom compared with rail-to-rail parts. Since no manufacturer extraction was available, verify common-mode range, output swing into the Daisy input load, and maximum unclipped guitar/line-level signal from the exact TI datasheet and on the bench.

The 135 mm VREF route uses eight vias and all four routing/reference contexts (F.Cu, B.Cu, and In2.Cu segments). For a shared analog bias, that is unnecessarily distributed. Keep the buffered reference low impedance, route it away from SD clocks/Wi-Fi current paths, and place a local 100 nF return at every analog stage. The analyzer's cap-to-via warnings (typically 3-5 mm) are credible enough to improve during the next layout pass, especially around IC1-IC4.

### Expression Inputs

U1/U2 buffer the Daisy 3.3 V analog reference and the two expression ADC paths. The functional topology is sensible, but these are cable-connected, high-impedance interfaces. Add ESD clamps and input series resistance/RC filtering at the physical board entry. Confirm that a miswired TRS cable, powered expression pedal, or contact with 5 V cannot exceed Daisy ADC limits.

### Digital Interfaces

The UART mapping agrees with firmware:

- ESP GPIO17 TX -> `/ESP_TO_DAISY` -> Daisy D14/RX.
- Daisy D13/TX -> `/DAISY_TO_ESP` -> ESP GPIO16 RX.
- Both traces are about 79-85 mm, with no vias. At 115200 baud this is electrically relaxed; preserve ground reference and separation from clocks/audio inputs.

The SD buses are more sensitive. Add transition return vias, preserve continuous GND under the routes, and avoid running clock traces near module/header pins that can become antennas. If high SD clock rates are required, measure at the socket and tune source-series resistance rather than relying only on nominal trace impedance.

### Bus Topology

No I2C or differential bus is used on this carrier. The manually confirmed buses are one 115200-baud point-to-point UART between ESP32 and Daisy, one four-wire ESP32 SPI microSD interface, and one Daisy four-bit SD interface. Pull-ups are 47 kohm on the relevant SD command/data nets. The analyzer did not produce a consolidated bus-topology object, so this section is raw-net and firmware derived.

## PCB Layout Analysis

- **Native connectivity:** KiCad DRC found 0 unconnected items; the analyzer's reported `+3.3VA`, `+5V`, `VCC_Daisy`, and `VCC_ESP32` split islands conflict with native connectivity and are treated as false positives.
- **Routing:** 658 segments, 97 vias, 1.96 m total trace length, 105 PCB nets, 0 unrouted nets.
- **Stackup:** F.Cu / In1.Cu / In2.Cu / B.Cu, 1.6 mm board, approximately 0.48 mm dielectric spacing between copper layers. In1 is the primary GND plane; In2 is partitioned among power/GND zones.
- **Antenna:** An explicit all-copper-layer ESP32 antenna keepout exists at x=66.044-87.945 mm, y=44.1-54.5 mm. This is a good control, but its dimensions and the adjacent Card2 metal shell must be checked against the exact ESP32 module reference design.
- **Card1 edge:** The Daisy microSD socket is 0.82 mm from the board edge. This can be intentional for card access, but confirm panelization/depaneling and enclosure clearance.
- **Back silkscreen:** The large bottom graphic crosses many pad/hole areas and will be heavily clipped by solder-mask clearances. This is cosmetic, but review the final Gerber to ensure no essential markings are lost and no assembler rejects the artwork density.
- **Mounting:** Four unannotated `REF**` mounting footprints are intentional mechanical items, not stale schematic parts. Annotate or mark them board-only to remove parity noise. Validate the asymmetric mounting pattern and the under-Daisy screw location with actual hardware.

## ERC and DRC

### KiCad DRC

Native KiCad 10.0.3 DRC with zone refill and schematic parity found:

- 3 board violations: one dangling GND via and Card1/Card2 library mismatches.
- 0 unconnected items.
- 7 parity notices, all caused by four board-only mounting holes sharing `REF**`.

No clearance, short, edge-cut, annular-ring, or unrouted-item error was reported under the project's active rule settings. Several checks are configured as ignored, including missing courtyards, footprint type mismatch, and silkscreen over copper; that limits the meaning of a “clean” DRC for assembly quality.

### KiCad ERC

Native ERC found 69 items:

- 54 dangling labels, largely connector pin-description labels placed as documentation rather than electrically attached.
- 8 “power pin not driven” errors caused by module-sourced rails/passive ferrite paths and missing PWR_FLAG declarations.
- 3 MCP6002 library-symbol mismatch warnings for U3 units.
- 2 dangling no-connect flags.
- 1 zero-length/unconnected wire endpoint at Card2 VDD.
- 1 GND/GNDA multiple-name warning.

The power rails were manually traced and are present, so the eight source warnings are ERC-modeling debt rather than proven opens. Clean all 69 items before release: use graphic text for pin documentation, add PWR_FLAGs at real sources, remove zero-length wires/orphan no-connects, normalize ground naming, and reconcile the U3 symbol.

## Manufacturing and Test

### Gerber Verification

The available Gerber archive has all four copper layers, masks, paste, silks, Edge.Cuts, PTH drill, and NPTH drill. Its 100 mm x 100 mm outline is coherent. The Gerber analyzer's layer-extents alignment warnings are false positives caused by legitimate differences in copper coverage, and its 43% F.Paste-to-F.Cu flash ratio is not by itself evidence of missing paste because through-hole, via, and no-paste copper inflate F.Cu counts.

The archive must still not be ordered: it predates the current board and BOM. Generate the complete manufacturing set atomically from one tagged source state. Then verify:

1. Current Card1/Card2 TF-01A sockets and their rotations appear in BOM/CPL.
2. Current LED/resistor additions and all changed passive values appear in BOM.
3. CPL coordinates and rotations match the assembly render.
4. Paste apertures exist for all SMD signal and shell pads intended for reflow.
5. Board-level or panel-level fiducials satisfy the selected assembler.
6. A first-article test covers 5 V input, both filtered module rails, both 3.3 V outputs, VREF settling, audio loopback/noise, expression range, UART, both SD buses, and Wi-Fi operation.

### Test Coverage

Nineteen test-point symbols cover 13 unique nets, including +5V, both filtered 5 V module rails, VREF, expression references, audio/jack nodes, and ground. Automated PCB coverage is 11/101 signal nets (11%). Missing high-value access includes +3.3VA, +3.3VD, ESP-3.3V, UART TX/RX, and the ESP SD SPI bus. This is adequate for exploratory bring-up but not for efficient production fault isolation.

### Assembly Complexity

The schematic estimator rates assembly complexity as low (26/100): mostly 0805, 1206, and SOIC parts, with 16 through-hole items. Five 0201 LEDs are the notable hand-assembly/rework challenge. The lack of fiducials, stale CPL, and unverified microSD footprint revisions are more important assembly risks than package density itself.

### BOM Optimization

The passive set uses 10 resistor values and 7 capacitor values, with only two single-use passive values. That is already reasonably consolidated. Do not optimize further until exact voltage rating, dielectric, tolerance, LED brightness, ferrite current/DCR, and audio-noise requirements are assigned; replacing controlled specifications with value-only substitutions would increase risk.

## PDN Impedance

The analytical PDN model found no extracted component-specific impedance data because capacitor and ferrite MPNs are absent. Its generic MLCC model places the highest modeled capacitor SRF near 15 MHz and suggests adding smaller values for higher-frequency coverage on the module rails. Treat that as a prompt for measurement, not a parts recommendation: the ESP32 and Daisy modules include local decoupling, and actual PDN behavior depends on their layouts, the selected ferrites, capacitor package/dielectric/DC bias, and source impedance. Measure rail droop and impedance/noise at TP6 and TP21 under simultaneous Wi-Fi, SD, LED, and DSP activity.

## Power Budget

The automated budget includes only carrier-board op-amps: approximately 120 mA on VCC_Daisy and 20 mA directly on +5 V. It omits the Daisy Seed, ESP32 module, both SD cards, LEDs, and transient load, so it is not a valid system budget. Create a manual worst-case budget from exact module/card data, include ESP32 RF peaks and SD write current, then require margin in the 5 V source, connector, planes, and FB1/FB2 current ratings.

## Sleep Current Audit

The resistor-only audit estimates 0.59 mA realistic quiescent current: about 0.56 mA worst-case through SD pull-ups plus 25 uA through the VREF divider, with GPIO-controlled LEDs assumed off. This excludes both modules and analog IC quiescent current. Since a pedal is typically externally powered, sleep current is secondary to noise and thermal behavior, but the result is not suitable for battery-runtime claims.

## Thermal and EMC

Automated thermal analysis was skipped because no component had MPN-backed power/thermal data. Manual risk is modest for the discrete analog stages; the dominant unknowns are module power, ferrite heating/droop, and LED current. Verify ESP32 peak-current behavior and closed-enclosure temperature.

The EMC analyzer scored 46/100, but most of its 119 findings are repeated reference-plane coverage alerts on unused header-breakout nets. That raw score overstates product risk. Retained concerns are:

- SD clocks/data crossing layers without local return vias.
- Long outer-layer SD clocks near module/header structures.
- Analog/audio layer transitions without nearby return vias on channel 2 and expression channels.
- No ESD clamps at cable-connected interfaces.
- Decoupling return vias 3-5 mm from several analog capacitors.

This is a pre-compliance risk assessment, not an FCC/CISPR prediction. Wi-Fi module certification does not cover host-board cable radiation or ESD behavior. Perform near-field scanning around both SD sockets, the ESP32 module, and the Daisy headers, then test conducted/radiated emissions with representative audio, expression, USB/power, and Wi-Fi activity.

## Positive Findings

- Native DRC reports no opens or shorts and routing is complete.
- Four-layer construction provides a broad dedicated GND reference plane.
- Audio channel component values are symmetric and implement sensible audio-band filtering.
- Major module rails have bulk and high-frequency decoupling plus ferrite isolation.
- ESP32 antenna keepout is explicit across all copper layers.
- UART direction and pin mapping match both firmware implementations.
- Critical analog, power, jack, and expression nodes have useful board test pads.
- The visual layout is clearly partitioned by function and connectors are accessible at the board perimeter.

## False Positives and Overrides

- J6-J11 absent from PCB: intentional off-board panel jacks; all have blank footprint properties.
- `REF**` extra/duplicate footprints: intentional board-only mounting holes, though they should be annotated or excluded cleanly.
- Power-plane island reports: rejected because fresh zone refill did not remove them but native KiCad DRC found 0 unconnected items; the analyzer's copper union model is not authoritative here.
- Card1/Card2 “thermal pad via” warnings: rejected. Pads 10-13 are microSD shield/mount tabs, not exposed thermal pads.
- 0.127 mm “narrow signal” warnings: not current-capacity defects for audio/high-impedance logic. They may still be widened for fabrication margin.
- Gerber extent and paste-ratio warnings: not accepted as defects after layer/content review; stale release age remains the actual blocker.

## Analyzer Verification

Critical analyzer conclusions were checked against native KiCad ERC/DRC, raw net maps, raw footprints/zones, current firmware pin assignments, a scratch-board zone refill, fabrication archive contents, and fresh 3D renders. Where native KiCad contradicted the geometry analyzer, native connectivity controlled the verdict. Every retained issue states whether it is raw-file/native-tool based or an engineering inference.

## Not Performed / Review Limits

Fresh checks run:

- `analyze_schematic.py`
- `analyze_pcb.py --full --proximity`
- `cross_analysis.py`
- `cross_verify.py`
- `analyze_thermal.py` (ran, but skipped assessments due missing MPN data)
- `analyze_emc.py --market us`
- `analyze_gerbers.py --full`
- KiCad 10.0.3 native ERC
- KiCad 10.0.3 native DRC with zone refill and schematic parity
- Fresh top/bottom 3D renders
- Raw schematic/PCB net, footprint, zone, stackup, BOM, and fabrication-package checks

Not performed or limited:

- **Previous-review delta:** intentionally not performed to preserve clean-room isolation.
- **Datasheet-complete verification:** not possible because BOM MPN coverage is 0%; active-device value strings and a few URL fields are not a controlled BOM.
- **SPICE:** no supported simulator was installed. Passive filter and VREF startup calculations are analytical.
- **Quantitative thermal:** skipped by analyzer due missing MPN/power data.
- **Lifecycle/stock audit:** not meaningful without controlled MPNs.
- **Enclosure/module collision check:** enclosure and module 3D models were not available in the render.
- **Lab validation:** audio noise/THD, clipping, startup pop, RF/EMC, ESD, SD margin, and peak-current droop require physical measurement.

## Final Verdict / Readiness

Do not order the current package. Minimum release gate:

1. Resolve the VREF startup behavior and D1 current.
2. Decide and implement cable-interface ESD protection.
3. Fix the dangling GND via and clean ERC to an intentionally reviewed state.
4. Add SD transition return vias and review clock damping/plane coverage.
5. Reconcile Card1/Card2 library footprints against the C91145 drawing.
6. Verify ESP/Card2/Daisy/screw mechanical stack-up and antenna clearance.
7. Populate controlled MPNs and verify ratings, pinouts, ferrite current/DCR, and derating.
8. Regenerate BOM/CPL/Gerber/drill outputs from the final source and run a release-package diff.
9. Build one first article and complete the power, audio, control, SD, Wi-Fi, thermal, ESD, and EMC bring-up plan.