# ChimeraMultiFX PCB Review

Review date: 2026-08-09  
Design reviewed: `hardware/boards/daisyBoard.kicad_pcb`  
KiCad version used for checks: 10.0.3

## Scope

This review covers placement, routing, power distribution, grounding, mixed-signal return paths, RF coexistence, external-interface protection, mechanics, manufacturability, and initial bring-up for the Daisy Seed plus ESP32 audio-effects carrier board.

## Executive decision

**Status: electrically routed, but not ready for production fabrication.**

KiCad reports zero unconnected items and zero PCB/schematic parity mismatches. The board uses a sensible four-layer concept and the analog circuitry is physically grouped. Those are strong foundations.

Release is blocked by unresolved DRC warnings, no mounting provisions, absent connector-level protection, an unsafe/unclear power-entry implementation, weak decoupling coverage, a split-ground strategy that needs return-path verification, and incomplete fabrication outputs. The TL072 supply incompatibility identified in the schematic review must be corrected before layout sign-off because replacement parts may change decoupling and stability requirements.

## Board facts

| Item | Observed value |
| --- | ---: |
| Outline | 100 mm × 100 mm |
| Nominal thickness | 1.6 mm |
| Copper layers | 4 |
| Footprints | 91 |
| Pads | 351 |
| Routed segments | 431 |
| Vias | 98 |
| Copper zones | 5 |
| Unconnected items | 0 |
| Schematic parity issues | 0 |
| DRC violations | 96 warnings |

Stackup intent:

1. `F.Cu`: components and signals.
2. `In1.Cu`: power regions for Daisy, ESP32, and 3.3 V domains.
3. `In2.Cu`: split GNDA/GNDD reference plane.
4. `B.Cu`: signals and components.

Track widths present are 0.127, 0.254, 0.3048, 0.508, and 0.762 mm. The narrowest 0.127 mm tracks should be justified by the fabricator capability and limited to low-current signals.

## What is already sound

- Four layers provide much better return-path and power-distribution options than a two-layer carrier.
- The analog input/output circuitry is grouped in two compact, mirrored channel blocks.
- ESP32, Daisy, expression, power, and analog blocks are visually separated and clearly labeled.
- `NT1` makes the sole intended GNDA/GNDD connection explicit.
- Power branches have local bulk/high-frequency capacitors and ferrite beads.
- Trace widths are varied, with wider tracks used for some power routes.
- Test points provide useful access during bring-up.
- Native DRC finds no opens, and schematic parity is exact.

## Priority findings

### P0-1: Correct the analog amplifier selection before final placement approval

The board places four TL072 packages in circuits powered from a 5 V single supply. The schematic review explains why this is not a valid operating condition. A replacement amplifier can have different bypass requirements, input protection behavior, output stability, thermal pad, and footprint.

**Required action:** select the final op-amp first, then re-run placement and stability review. Place one 100 nF decoupler at each package supply pin pair, preferably on the same side with a direct ground via adjacent to the capacitor pad.

### P0-2: Add a real protected power-entry area

The board exposes `TP2` as the functional 5 V input and `TP1` as ground. They are test-point footprints, not a keyed, strain-relieved production connector. There is no visible fuse, reverse-polarity device, TVS, or implemented 9/12 V conversion stage.

**Required action:** reserve a connector-side power-entry zone containing the chosen connector, fuse/PTC, reverse-polarity or ideal-diode element, TVS, input bulk capacitor, and regulator if the input is not already regulated 5 V. Keep surge currents out of GNDA and route their return directly to the entry ground/chassis strategy.

### P0-3: Add ESD and RF protection at the physical board edge

Audio, expression, and power signals leave the board through THT test pads and wires. The existing analog filters are placed inside the functional blocks, but there is no dedicated edge protection. Long wires between a jack and protection parts behave as antennas and carry ESD current across the board.

**Required action:** use keyed connectors or clearly arranged solder pads at the edge. Put ESD suppressors and RF components immediately at those entries. Keep the unprotected trace length minimal and provide a short, low-inductance discharge path to chassis/entry ground.

### P0-4: Rework the split-ground implementation around signal return paths

`In2.Cu` is divided between GNDA and GNDD and joined through `NT1`. A split plane can work, but only when every signal remains above its own reference region and no fast trace crosses the split. Otherwise the return current detours through the net tie, increasing loop area and coupling digital noise into audio.

The board routes 52 segments on `In1.Cu` and five segments on `In2.Cu`; using plane layers for ordinary traces fragments their reference and power areas. Long routes also run between controller headers and SD sockets.

**Required action:**

- Keep `In2.Cu` as uninterrupted as possible; avoid signal tracks on it.
- Do not route UART, SD clock/data, ESP32 GPIO, or other fast edges across the GNDA/GNDD boundary.
- Place `NT1` at the actual current-conversion boundary, preferably near the codec/Daisy analog-ground relationship, not merely where routing is convenient.
- Ensure any signal that crosses domains has an adjacent return path across the same boundary. Add stitching/return capacitors only when their current path and EMC purpose are understood.
- Consider one continuous ground plane with placement-based partitioning. For many mixed-signal audio boards, this produces lower impedance and fewer accidental return detours than a physically split plane.

The preferred choice should be validated by measuring output noise during Wi-Fi transmit and SD activity.

### P0-5: Clear all DRC warnings and restore silkscreen legibility

KiCad reports 96 `silk_over_copper` warnings. These are not electrical shorts, but they indicate extensive silkscreen clipping over pads and solder mask, especially around headers and board graphics. Assembly labels can disappear or become misleading after fabrication.

**Required action:** move reference/value text, clip or relocate graphics, and keep polarity, pin 1, connector purpose, and voltage labels outside solder-mask openings. Run DRC again with zero unexplained warnings.

### P1-1: Improve local decoupling placement and via geometry

The analog devices need one local 100 nF capacitor per package. Existing capacitors are grouped into power islands but do not cover all seven op-amp packages. Bulk capacitors cannot replace a short high-frequency current loop.

For each IC, the desired loop is supply pin → capacitor → ground via/plane → IC return, with minimal enclosed area. Do not route a thin trace from a shared remote capacitor. Add local 1-10 µF per analog cluster as required by measurement.

For the ESP32 and both SD sockets, provide local low-ESR bulk capacitance for Wi-Fi/card current steps. Verify ferrite current rating, DC resistance, and impedance under DC bias.

### P1-2: Protect the ESP32 antenna keepout in all layers and mechanics

The ESP32 module is represented by headers, so the PCB does not intrinsically enforce the module antenna location or keepout. Copper, traces, ground planes, SD metalwork, wires, displays, batteries, and a metal enclosure near the antenna can reduce range and increase RF current in the audio ground.

**Required action:** add an explicit antenna outline and all-layer copper keepout tied to the exact ESP32 dev-kit drawing. Put the antenna at a board/enclosure edge with no copper or component beneath it and no cable crossing it. Validate conducted and radiated behavior in the final enclosure while monitoring the audio noise floor.

### P1-3: Tighten SD routing and damping

Both microSD sockets are remote from their controller headers and use long parallel routes. SD clocks have fast edges even at moderate clock frequencies. Long, unterminated routes can ring and inject broadband noise into the codec and analog inputs.

**Required action:**

- Place each socket closer to its controller or shorten/straighten the bus.
- Route clock with a continuous reference plane and avoid plane splits.
- Add source-series resistor footprints at the driver, especially for clock.
- Keep bus traces together in reference environment without unnecessarily tight coupling.
- Place card decoupling at the socket power pins.
- Keep the SD buses out of the analog input/output blocks.

### P1-4: Add mechanical mounting and cable strain relief

The 100 mm square board has no mounting-hole footprints. The controller modules, SD card insertion, and off-board jack wires all apply mechanical load. Header solder joints and test-point pads should not carry enclosure stress.

**Required action:** add at least three, preferably four, grounded or isolated mounting holes with keepouts appropriate to the chosen standoffs. Add keyed connectors or dedicated strain-relief holes for off-board wires. Check tool access for SD cards, USB connectors, boot/reset controls, and module removal.

### P1-5: Replace test-point footprints used as production connectors

The jack and power interfaces use 2.5 mm THT test-point pads. This makes assembly polarity and channel identity dependent on documentation and creates poor strain relief.

**Required action:** use locking wire-to-board connectors, board-mounted jacks, or paired labeled solder pads with nearby strain-relief holes. Group signal and return together to minimize loop area. Mark channel, tip/ring/sleeve, voltage, and polarity on both copper-side assembly drawings and silkscreen.

### P1-6: Review routing density and board size

The board is sparse for a 100 mm × 100 mm four-layer design, with large unused areas. That increases enclosure size, trace length, loop area, and fabrication cost. The long SD and controller interconnects are partly a consequence.

**Required action:** after connector and enclosure constraints are known, compact the board around functional current loops. Keep enough separation between the ESP32 antenna/digital region and analog inputs, but do not use empty distance as a substitute for controlled return paths.

### P1-7: Add production test access, fiducials, and identifiers

The existing functional test points are useful, but production coverage should also include both sides of the UART, reset/boot, each regulated rail, GNDA, GNDD, VREF, codec input/output, and expression ADC nodes. Add three global fiducials for automated assembly, local fiducials if required by fine-pitch parts, a board revision, and an unambiguous pin-1 marker for every module/socket.

### P2-1: Define the manufacturing stackup instead of using generic values

The file specifies four 35 µm copper layers and approximately equal 0.48 mm dielectric thicknesses, with dielectric constraints disabled and copper finish set to `None`. This is not yet a fabrication-controlled stackup.

**Required action:** obtain the board house's real four-layer stackup and update dielectric thickness, copper weight, finish, minimum drill, annular ring, mask, and impedance assumptions. For this design, controlled impedance is less important than a close signal-to-ground reference plane and low plane inductance, but the physical stack still matters.

### P2-2: Generate and inspect a complete fabrication package

No Gerber, drill, or Gerber job files are present in the reviewed board directory. There is an assembly PDF, but that is not a release package.

Generate and inspect:

- Gerbers and Gerber job file.
- Plated/non-plated drill files and drill map.
- Position files with correct top/bottom rotation.
- BOM with manufacturer part numbers, voltage ratings, dielectric, tolerance, and DNP status.
- Paste and assembly drawings.
- Board outline, dimensions, stackup, finish, and fabrication notes.

Use a Gerber viewer independent of the PCB editor before ordering.

## Audio-specific layout guidance

- Put input protection, RF filtering, bias resistor, and first buffer in that order from the jack inward.
- Keep high-impedance input nodes short, guarded from SD/UART/Wi-Fi routes, and away from plane boundaries.
- Route each audio signal over continuous GNDA. Do not share its return neck with SD, ESP32, or LED currents.
- Keep output traces separated from input traces to prevent high-gain effect feedback through board coupling.
- Route VREF as a quiet analog reference with no digital loads. Decouple it locally where it biases a stage.
- Put series output resistors next to the driving op-amp. Put connector ESD parts next to the connector.
- Avoid running clocks or fast GPIO in parallel with audio traces. If crossing is unavoidable, cross orthogonally on adjacent signal layers over a continuous reference.
- Add ground stitching around the digital perimeter and board edge where it does not violate the ESP32 antenna keepout or the deliberate analog return strategy.

## Routing and current checks

Before release, calculate or simulate:

- Maximum current and voltage drop from the power input to Daisy and ESP32 modules.
- Ferrite-bead dissipation and impedance at peak Wi-Fi/SD current.
- Copper temperature rise for 0.254, 0.508, and 0.762 mm power traces using the actual copper weight.
- VREF return current and ground offset between each analog stage and the codec.
- SD clock overshoot/undershoot at minimum and maximum configured clock.
- Crosstalk from UART/SD/Wi-Fi activity into a terminated audio input.

## Required PCB release checklist

- [ ] Implement the schematic review P0 corrections before final routing.
- [ ] Add protected, keyed power entry with a defined voltage contract.
- [ ] Add connector-edge ESD/RF protection and short discharge paths.
- [ ] Verify or replace the split-ground strategy using return-current analysis.
- [ ] Remove signal routing from plane layers where practical.
- [ ] Add a dedicated 100 nF decoupler at every op-amp package.
- [ ] Add and enforce the exact ESP32 antenna keepout on every copper layer.
- [ ] Shorten SD buses and add source-damping footprints.
- [ ] Add mounting holes and off-board wire strain relief.
- [ ] Replace production test-point connectors with keyed connectors or robust solder interfaces.
- [ ] Add global fiducials, revision marking, polarity labels, and pin-1 marks.
- [ ] Update to the selected fabricator's actual four-layer stackup.
- [ ] Clear all 96 current DRC warnings.
- [ ] Generate and independently inspect Gerbers, drills, BOM, and placement files.

## Bring-up sequence

1. Inspect the unpowered board for shorts; verify GNDA-to-GNDD continuity only through `NT1`.
2. Populate only protection/regulation and dummy loads. Verify current limit, reverse-polarity behavior, and rail transients.
3. Populate VREF and analog supplies. Confirm VREF startup, DC accuracy, broadband noise, and load stability.
4. Populate one audio channel. Sweep frequency and level before installing the MCUs.
5. Install Daisy and verify codec loopback with the ESP32 absent.
6. Install ESP32 with Wi-Fi disabled, then enabled. Measure the change in output noise and spurs.
7. Exercise UART continuously, then each SD bus at maximum intended speed while recording a terminated audio path.
8. Connect the final jack harness and enclosure. Repeat noise, hot-plug, ESD, and RF tests.
9. Run the worst-case four-lane DSP preset and simultaneous Wi-Fi/UI traffic; verify no audio dropouts, codec clipping, brownouts, or UART corruption.

## Review evidence

- Native KiCad DRC: 96 warnings, all classified as silkscreen clipped by solder mask.
- Native connectivity: zero unconnected PCB items.
- Native schematic parity: zero mismatches.
- Layout inventory: 431 routed segments, 98 vias, five copper zones, and 91 footprints.
- Mechanical inventory: no mounting-hole footprints.
- Release inventory: no Gerber or drill files found in the reviewed board tree.