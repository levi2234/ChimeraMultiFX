# ChimeraMultiFX KiCad Review

Review date: 2026-08-17  
Design files reviewed:

- `hardware/boards/daisyBoard.kicad_sch`
- `hardware/boards/daisyBoard.kicad_pcb`
- `hardware/boards/jlcpcb/production_files/GERBER-daisyBoard.zip`

## Verdict

The board architecture matches the intended purpose: a dual-MCU effects pedal platform where the Daisy Seed handles real-time audio DSP and the ESP32 handles UI/control. The schematic includes the expected functional blocks: audio input/output conditioning, expression inputs, Daisy/ESP32 headers, UART/control links, SD interfaces, VREF, and test access.

The current revision is **not ready for dependable production fabrication**. It is closer to an engineering prototype. The largest risks are analog op-amp suitability, unclear power entry, missing datasheet/MPN coverage, schematic-PCB sync gaps, split-plane return paths, and assembly/manufacturing issues.

## Analysis Performed

The following kicad-happy analyzers were run and saved in this directory:

- `schematic.json` from `analyze_schematic.py`
- `pcb.json` from `analyze_pcb.py --full`
- `cross_analysis.json` from `cross_analysis.py`
- `emc.json` from `analyze_emc.py`
- `thermal.json` from `analyze_thermal.py`
- `gerber.json` from `analyze_gerbers.py`

Not performed or limited:

- Native KiCad ERC/DRC was not run because `kicad-cli` was not found on PATH.
- SPICE simulation was not run because no `ngspice`, `ltspice`, or `xyce` executable was found.
- Datasheet sync and lifecycle audit were not run because no DigiKey credentials were set and the schematic has essentially no populated MPN coverage.
- Pin-level electrical claims are therefore consistency-only unless specifically backed by a local datasheet URL or raw file evidence.

## Summary Facts

Schematic analyzer:

- 106 total components
- 39 unique parts
- 174 nets
- 334 wires
- 25 no-connect markers
- 0/36 useful unique BOM parts with MPN coverage according to the sourcing audit

PCB analyzer:

- 100 mm x 100 mm board outline
- 4 copper layers
- 112 footprints
- 471 track segments
- 101 vias
- 3 copper zones
- 115 PCB nets
- Routing complete according to the analyzer

EMC analyzer:

- 115 findings
- 64 errors
- 34 warnings
- EMC risk score: 13/100

Gerber analyzer:

- Board dimensions match 100 mm x 100 mm
- 3 warnings: layer extent/alignment variation and low front paste aperture coverage

## Blockers

### 1. TL072 op-amps are still unsuitable for the shown 5 V single-supply audio path

The schematic still uses TL072 devices, for example `IC2` in `hardware/boards/daisyBoard.kicad_sch`. The design centers audio around a midrail reference and powers the analog section from the available low-voltage rails. A standard TL072 is not a safe choice for a nominal 5 V single-supply audio front end: input common-mode range, output swing, startup behavior, distortion, and stability are not guaranteed for this use.

Impact: clipping, nonlinear audio, unpredictable headroom, startup problems, and channel variation.

Required action: select a 5 V single-supply audio op-amp with input common-mode and output swing compatible with the Daisy codec/audio levels. Then recheck gain, headroom, noise, capacitive-load stability, VREF loading, and local decoupling.

### 2. Power input contract is contradictory and underprotected

The schematic contains text describing a USB-C 5 V input, but also text describing a 9 V or 12 V USB-C input stepped down by a buck/linear regulator combination. The implemented entry is a test point named `TP2` / `5V IN`, not a protected keyed production power entry.

Impact: applying the wrong external voltage can damage the ESP32, Daisy, SD cards, analog circuitry, or external equipment. A pedal-style power input also needs transient, reverse-polarity, and fault-current handling.

Required action: choose one explicit power contract:

- Regulated 5 V input, with voltage tolerance, current budget, connector pinout, fuse/PTC, reverse protection, TVS, and input capacitance.
- 9 V pedal input, with protected buck conversion to 5 V and noise filtering suitable for audio.

Update the schematic notes and silkscreen so there is only one interpretation.

### 3. Datasheet and MPN coverage is not sufficient for verification

The schematic analyzer reported `DS-001` and `SS-001`: no project datasheets directory and poor MPN coverage. Most components lack manufacturer part numbers and distributor fields.

Impact: this review cannot honestly verify pinouts, voltage ratings, op-amp limits, ferrite ratings, connector details, SD socket behavior, LED current limits, or protection-device suitability against manufacturer data.

Required action: populate MPNs for all ICs, connectors, ferrites, LEDs, protection parts, SD sockets, and critical passives. Then sync or add datasheets and rerun the review.

### 4. Schematic and PCB are out of sync

Cross-analysis reported that `J6`, `J7`, `J8`, `J9`, `J10`, and `J11` exist in the schematic but are missing from the PCB. The PCB also contains unsynchronized `REF**` mounting-hole footprints.

Impact: the manufactured board may omit intended connectors or retain undocumented mechanical/electrical objects. This is a pre-fabrication blocker.

Required action: run KiCad's update PCB from schematic flow, annotate the mounting holes properly, and rerun schematic/PCB cross-analysis.

### 5. Ground and power plane continuity is not acceptable yet

Cross-analysis reported:

- `GND` plane split into 42 islands with 63 signals crossing
- `VCC_Daisy` plane split into 13 islands with 38 signals crossing
- `+3.3VD` plane split into 8 islands with 6 signals crossing
- `+3.3VA` plane split into 3 islands with 4 signals crossing
- `+5V` plane split into 8 islands with 4 signals crossing
- Daisy SD clock `/SD_CLK_DAISY` crosses a GND plane gap
- SPI nets `/SD_MISO`, `/SD_MOSI`, and `/SD_SCK` also cross GND plane gaps

Impact: return currents can be forced through long detours, increasing loop area, EMI, digital noise coupling, and audio contamination. This is especially risky for a board combining Wi-Fi, SD clocks, and high-impedance audio.

Required action: refill zones in KiCad first to rule out stale fills, then rework the plane strategy. Prefer a mostly continuous ground reference with placement-based partitioning unless the split-ground boundary is fully controlled. Reroute SD clocks/data away from plane gaps, or provide deliberate adjacent return paths.

### 6. EMC risk is high

The EMC analyzer scored the design 13/100 and reported 64 errors. Most are ground/reference-plane gap issues, plus decoupling placement and clock-routing warnings.

Impact: high risk of SD/Wi-Fi/control noise coupling into audio, poor emissions behavior, and sensitivity to cable/external-jack events.

Required action: fix plane continuity first, then rerun EMC analysis. Add connector-edge ESD/RF protection and keep fast digital traces away from audio input/output routes.

## Additional Warnings

### LED D1 current is too high

Analyzer finding `LR-001` estimates LED `D1` at about 41 mA through `R32 = 27R` from a 3.3 V rail. That is too high for a typical indicator LED.

Required action: increase `R32`; the analyzer suggested about 110 ohms for roughly 10 mA using a 2.2 V LED assumption. Final value should use the exact LED datasheet and desired brightness.

### SD socket placement is too close to the board edge

PCB analyzer placement findings:

- `Card1`: 0.82 mm from board edge
- `Card2`: 0.32 mm from board edge, reported as an error

Required action: move both sockets to meet the chosen fabricator and assembly clearances. Treat `Card2` as a manufacturing blocker.

### Assembly readiness is incomplete

The PCB analyzer reported:

- No front-side fiducials despite 76 SMD components
- Test point coverage of only 11/111 nets, about 10 percent
- Many untented via-in-pad warnings
- Insufficient thermal vias under both SD socket footprints according to the analyzer

Required action: add three global fiducials, improve test coverage for rails/control/audio/boot/reset, avoid untented via-in-pad unless filled/capped, and review SD socket land pattern recommendations.

### Gerber package needs another export/check pass

Gerber analysis found:

- Width variation of 6.6 mm across copper/edge layers
- Height variation of 6.3 mm across copper/edge layers
- Front paste layer has 190 flashes vs 437 copper pads, about 43 percent coverage

Required action: regenerate Gerbers, drills, paste, BOM, and CPL from the cleaned KiCad design. Inspect with an independent Gerber viewer before ordering.

## What Looks Sound

- The functional architecture matches the project goal.
- The board has four copper layers, which is appropriate for mixed audio/digital routing if the planes are fixed.
- Routing is complete according to the analyzer.
- Audio I/O nets, expression nets, Daisy/ESP32 control paths, SD buses, and test structures are present.
- Mounting holes now exist, which improves over the previous review, but they need annotation/sync cleanup.
- A fabrication package exists, though it needs regeneration after fixes.

## Recommended Fix Order

1. Replace TL072 stages with a verified 5 V single-supply audio op-amp and update decoupling/stability calculations.
2. Define the power input contract and implement a protected power-entry stage.
3. Update PCB from schematic and resolve `J6`-`J11` plus `REF**` sync issues.
4. Rework ground/power planes and reroute SD clocks/data away from return-path gaps.
5. Add connector-edge ESD/RF protection for audio, expression, power, and exposed user-accessible connections.
6. Populate MPNs and datasheets, then rerun schematic, PCB, EMC, thermal, and lifecycle checks.
7. Fix assembly/DFM issues: SD edge clearance, fiducials, via-in-pad, test coverage, paste/gerber warnings.
8. Install or expose `kicad-cli` on PATH and run native ERC/DRC before ordering.

## Final Readiness Statement

Do not order this design as a final PCB. A limited prototype is reasonable only after the TL072, power-entry, schematic/PCB sync, and plane-continuity issues are fixed. As currently reviewed, the board is likely to be fragile for a real guitar/effects use case: it may work partly, but quiet audio, robust external-connector behavior, and repeatable production assembly are not yet supported by the design evidence.