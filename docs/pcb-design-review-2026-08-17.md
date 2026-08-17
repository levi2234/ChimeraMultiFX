# ChimeraMultiFX Schematic and PCB Review

**Date:** 2026-08-17  
**Design:** `hardware/boards/daisyBoard.kicad_sch` + `hardware/boards/daisyBoard.kicad_pcb`  
**Purpose reviewed:** Daisy Seed audio DSP, ESP32 Wi-Fi/control/UI, UART bridge, dual-channel audio input/output conditioning, expression inputs, and SD interfaces.

## Verdict

**Not ready for another fabrication run without changes.** The architecture matches the project purpose, and the ESP32 power branch already includes useful conducted-noise isolation. The principal blockers are:

1. Four ordinary TL072 devices are powered from a nominal 5 V single supply, below TI's 7 V minimum total supply specification.
2. All four unused TL072 B channels are left floating.
3. Local op-amp decoupling is inadequate: six analog ICs share only a small number of bypass capacitors placed far from several ICs.
4. The ESP32 dev-board antenna is not represented by a physical footprint or enforced keepout. Inner-layer copper extends beneath the probable antenna area.
5. Audio and reference nets change layers without nearby ground-return vias, and the four-layer stack has a fragmented power plane—not GND—adjacent to top-layer signals.

These issues can plausibly cause clipping/distortion, instability, idle noise, Wi-Fi-correlated ticking/buzzing, degraded ESP32 range, and board-to-board variation.

## Review Basis

### Analyses run

- KiCad schematic analyzer
- Full PCB analyzer with trace proximity and connectivity analysis
- Schematic ↔ PCB cross-analysis
- FCC Class B-oriented EMC risk analysis
- Thermal analyzer
- Gerber/drill analyzer on the archived production ZIP
- Raw schematic, PCB, BOM, CPL, project settings, and README inspection
- Manufacturer web documentation checks for TL072 supply range and ESP32 layout guidance

Generated evidence is under `analysis/2026-08-17_1502/`.

### Not performed / limits

- Native KiCad ERC and DRC were not run because `kicad-cli` is unavailable. Run both in KiCad before fabrication.
- SPICE was not run because ngspice, LTspice, and Xyce are unavailable.
- Thermal analysis ran but returned no modelable dissipative parts; this is a modeling gap, not proof of zero thermal risk.
- Lifecycle analysis was not useful because MPN coverage is effectively 0% in the schematic.
- Full per-component manufacturer pinout verification was not possible because no datasheet cache exists and most exact MPNs are absent.
- The exact ESP32 dev-board variant and antenna orientation are not encoded in the design. Antenna location is inferred from the header pin mapping and common 30-pin ESP32 DevKit geometry.

## Design Summary

- 100 mm × 100 mm, four-layer, nominal 1.6 mm FR-4 board.
- Stack: F.Cu / In1.Cu power / In2.Cu GND / B.Cu.
- 106 schematic components and 174 schematic nets.
- Daisy Seed and ESP32 are socketed modules with breakout headers.
- The ESP32 receives 5 V through `FB1`; `C21` (100 µF), `C22` (100 nF), and `C23` (100 nF) provide branch capacitance.
- `U3` buffers the midrail `VREF`; TL072 and MCP6002 stages condition audio/expression signals.
- Audio I/O routes between the analog stages and Daisy headers; physical jacks appear intended to be off-board/wired through test-point pads.

## Critical and High-Priority Findings

### 1. TL072 supply voltage is outside its specified operating range — CRITICAL

**Evidence basis:** raw schematic/PCB verified + TI product specification.

`IC1`–`IC4` have pin 8 on `VCC_Daisy` and pin 4 on analog ground. `VCC_Daisy` is tied to the nominal +5 V source through `NT1`. TI lists the standard TL072 minimum total supply voltage as 7 V. Therefore, 5 V single-supply operation is outside the guaranteed range.

This is not merely an audio-quality optimization. The op amps may have poor output swing, common-mode violations, distortion, phase reversal-like behavior, startup problems, or large unit-to-unit variation.

**Required action:**

- Replace `IC1`–`IC4` with a pin-compatible dual op amp explicitly specified for 5 V single-supply operation.
- Select for low noise, adequate GBW/slew rate, input common-mode range that includes the actual signal range around `VREF`, output swing under the real load, and stability at unity gain.
- Alternatively, redesign the analog supply to meet TL072 requirements, but this is less attractive for a 5 V digital/audio module system.
- Recalculate all stage headroom after selecting the replacement.

Reference: TI TL072 product page states a 7 V minimum total supply voltage: https://www.ti.com/product/TL072

### 2. Four unused TL072 amplifier channels float — HIGH

**Evidence basis:** raw PCB pad-net verification.

For each of `IC1`, `IC2`, `IC3`, and `IC4`, pads 5, 6, and 7 are unconnected. These are the B-channel non-inverting input, inverting input, and output. Floating unused op-amp inputs can allow oscillation or rail chatter, injecting noise into the shared supply and substrate.

**Required action:** configure each unused channel as a defined unity-gain follower:

- non-inverting input → `VREF`
- output → inverting input
- do not short the output directly to a rail

Follow the selected replacement op amp's unused-channel guidance.

### 3. Analog IC bypassing is inadequate and physically remote — HIGH

**Evidence basis:** schematic inventory + deterministic PCB placement checks.

The `VCC_Daisy` rail supplies four TL072s and two MCP6002s. The board does not provide one local 100 nF capacitor per IC. Existing rail capacitors `C16` and `C17` are near the Daisy/power area rather than beside every audio IC. Automated geometry checks found:

- no decoupling capacitor within 10 mm of `U2`
- nearest candidate to `U1` approximately 9.3 mm away
- nearest candidates to `IC1` and `IC2` approximately 5 mm away

Some automatically associated capacitors are actually signal-path parts, so the true bypass situation is worse than the raw detector summary.

**Required action:**

- Add one 100 nF X7R capacitor directly between pins 8 and 4 of every dual op amp.
- Place each capacitor on the same side, ideally within 2–3 mm of the pins.
- Give the ground pad an adjacent via to the In2.Cu GND plane.
- Add local 1 µF near each analog cluster and retain suitable bulk capacitance at the rail entry.
- Keep each bypass loop compact; do not route power from the IC pin to a remote capacitor and back.

This change is one of the highest-value ways to reduce Wi-Fi-correlated audio noise through shared impedance.

### 4. ESP32 antenna keepout is absent and copper extends beneath the likely antenna — HIGH

**Evidence basis:** raw PCB/analyzer geometry + Espressif layout guidance; antenna orientation is inferred.

The ESP32 is modeled only as two 1×15 socket rows (`J1` and `J2`). There is no module body, courtyard, antenna graphic, or copper/routing keepout. The PCB analyzer found zero keepout zones.

The probable module envelope spans approximately x=64.23–89.63 mm and y=53.51–89.07 mm. The `ESP-3.3V` plane on In1.Cu spans x=65.755–91.255 mm and y=50.28–90.78 mm, while the In2.Cu GND plane spans essentially the entire board. Therefore, both inner copper layers extend under the probable antenna end.

Espressif recommends putting a module antenna beyond the baseboard edge. If that is impossible, the antenna should be at the edge with baseboard material/copper cleared around and below it; at least 15 mm housing clearance is recommended in all directions.

**Required action:**

1. Identify and document the exact ESP32 dev-board model and orientation.
2. Replace the two-header-only representation with a real module/dev-board footprint containing:
   - complete body outline
   - pin-1/orientation marker
   - antenna outline
   - component courtyard
   - all-layer copper/track/via keepout under and around the antenna
3. Prefer letting the antenna end overhang the carrier-board edge.
4. If it cannot overhang, cut the carrier board back beneath/beside the antenna per the exact module guidance.
5. Keep metal enclosure walls, displays, cables, jack bodies, and wiring at least 15 mm away where practical.
6. Add GND stitching at the keepout boundary, but never put stitching vias inside the antenna keepout.

Reference: Espressif ESP32 Hardware Design Guidelines, module positioning and antenna keepout: https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/pcb-layout-design.html

### 5. Layer stack/reference arrangement increases coupling risk — HIGH

**Evidence basis:** raw-file/analyzer-derived.

Top-layer traces see In1.Cu first, but In1.Cu is a collection of power regions (`VCC_Daisy`, `ESP-3.3V`, etc.), not a continuous ground plane. The continuous GND plane is In2.Cu, farther from F.Cu. This creates reference changes and gaps for fast ESP32/SD/UART edges and weakens electrostatic shielding of high-impedance analog traces.

Cross-analysis reported signals crossing power/GND plane discontinuities, including audio, UART, SD clock/data, `VREF`, and expression ADC nets. The EMC score of 13/100 is pessimistic and includes false-positive amplification, but the stackup concern itself is real.

**Preferred action for the next revision:**

- Change the stack to F.Cu / solid GND / power or GND / B.Cu.
- Keep layer 2 unbroken except for the intentional antenna keepout.
- Route power as polygons/traces on layer 3, with GND fill in unused space.
- Do not route signals across power-island boundaries on the adjacent reference layer.
- Keep SD clock/data together over one continuous reference and add source series damping where supported by the module/interface.

This matches Espressif's recommended four-layer ordering: top signal, layer-2 solid GND, layer-3 power/GND, bottom signal.

### 6. Audio and `VREF` layer transitions lack adjacent return vias — HIGH

**Evidence basis:** deterministic PCB topology.

Detected transitions without a GND stitching via within 1 mm include:

- `/AUDIO_IN_2`
- `/AUDIO_OUT_2`
- `Net-(U1B-+)`
- six transitions on `VREF`
- both expression ADC inputs
- multiple SD/UART-related nets

At audio frequency this is not a transmission-line problem, but the same copper can receive or rectify 2.4 GHz RF. A tight high-frequency return path reduces the loop area and RF-to-audio conversion.

**Required action:**

- Keep high-impedance analog nets on one layer wherever possible.
- Add a GND via immediately beside every unavoidable analog signal via.
- Avoid long `VREF` routing. Treat it as a low-noise analog supply/reference, not an ordinary signal.
- Add local `VREF` bypassing at each destination cluster if permitted by `U3` stability requirements.

## ESP32-to-Audio Noise Assessment

### What is already good

1. **Physical partitioning is directionally good.** The likely antenna is near the top board edge; primary audio stages are mostly around y≈99–141 mm, providing useful separation.
2. **ESP32 conducted-noise branch exists.** `FB1` separates +5 V from `VCC_ESP32`, with 100 µF and two 100 nF capacitors on the ESP branch.
3. **A solid In2.Cu GND fill exists.** The zone itself is filled as one region with about 86% area coverage.
4. **Input networks include RF attenuation.** Series resistance and shunt capacitance around the analog inputs reduce demodulation of RF at op-amp inputs.
5. **Analog and digital supply nets are named and intentionally partitioned.** This is a good basis for controlled return-current design.

### What still allows audible Wi-Fi artifacts

- Unsupported TL072 operation and floating channels can rectify or amplify RF and supply disturbances.
- Sparse/remote bypassing raises analog supply impedance during ESP32 current bursts.
- Copper beneath the antenna alters the antenna and drives stronger RF currents into the carrier board.
- The top-layer adjacent reference is fragmented power, encouraging larger return loops.
- Long audio traces (approximately 48–70 mm) and the 121 mm `VREF` route offer coupling area.
- Analog layer changes lack local GND-return vias.
- Exact ferrite current rating and DCR are unknown because `FB1` has no MPN. A “600 Ω @ 100 MHz” value alone is insufficient; verify current rating for ESP32 transmit peaks and low enough DCR to avoid brownout.

### Recommended priority order

1. Replace the TL072s and terminate unused channels.
2. Add local bypassing to every analog IC.
3. Create and enforce the exact ESP32 antenna keepout/overhang.
4. Put solid GND on layer 2 and move power partitioning to layer 3.
5. Shorten/re-route high-impedance audio and `VREF`; add return vias.
6. Move the ESP ferrite/bulk network closer to the module 5 V entry if the dev board's own local input capacitance is insufficient.
7. Verify `FB1` current rating/DCR and measure the ESP branch at maximum TX power.

## Schematic Function Review

### Architecture fit

The schematic broadly matches the project:

- ESP32 handles Wi-Fi/control and communicates with Daisy.
- Daisy receives and outputs two audio channels.
- Analog front-end stages buffer/filter/level-shift external audio around `VREF`.
- Expression inputs are buffered before Daisy ADC inputs.
- Separate SD interfaces and module breakout headers support development and expansion.

### Additional schematic concerns

- Exact module MPNs are absent for the ESP32 dev board and Daisy Seed. Connector pinout correctness cannot be guaranteed without these identities.
- `FB1`–`FB3` have impedance values but no current/DCR/voltage ratings or manufacturer parts.
- Rail-source warnings exist because power inputs rely on passive connectors/net ties without explicit `PWR_FLAG` declarations. Add correct flags after verifying the intended source direction so ERC becomes useful.
- Six jack symbols (`J6`–`J11`) exist in the schematic but not on the PCB. This appears intentional for off-board jacks, but mark them explicitly as excluded from board and document the wiring harness/pinout.
- MPN coverage is below the pre-fabrication threshold. Exact parts are essential for pinout, package, voltage rating, ferrite current, and assembly verification.

## PCB / EMC Findings

### Ground-plane finding triage

The connectivity analyzer reported “42 GND islands,” but the raw GND zone is filled as one In2.Cu region. The island count likely includes isolated pads/tracks or an analyzer limitation and should not be treated literally without native KiCad DRC and visual inspection.

The following residual findings remain valid despite that likely overcount:

- In1.Cu is fragmented power and is adjacent to F.Cu.
- Multiple signals cross adjacent-layer power boundaries.
- GND return vias are absent beside many signal transitions.
- Copper exists under the probable ESP32 antenna.

### Routing and DFM

- Routing is represented as complete by the parser, but native KiCad DRC must confirm no unconnected copper.
- Several 0.127 mm analog/power traces are narrow but electrically adequate for op-amp signals; they are less robust for fabrication and return-path control than 0.20–0.25 mm routing.
- `Card2` is approximately 0.32 mm from the board edge and `Card1` approximately 0.82 mm. Verify connector-body and fab edge-clearance requirements.
- No front-side fiducials were detected despite 76 SMD placements. Add three global fiducials for automated assembly.
- Test-point coverage is approximately 10%. Add accessible points for ESP 5 V, ESP 3.3 V, Daisy 5 V, analog 5 V, `VREF`, GND, UART, SD clocks, and each audio stage input/output.
- The analyzer flagged many untented via-in-pad cases. Many are likely ordinary zone/via overlaps rather than true component-pad vias; inspect native DRC and fabrication outputs before accepting the warning.

## Gerber / Manufacturing Review

The archived production package contains:

- all four copper layers
- front/back mask, paste, and silkscreen
- edge cuts
- PTH and NPTH drill files
- 100 mm × 100 mm outline

Warnings requiring visual CAM confirmation:

1. Copper/edge extents vary by roughly 6.6 mm × 6.3 mm. This may be legitimate copper not reaching the board edge, so treat it as a likely analyzer false positive unless CAM visualization shows actual layer offsets.
2. Front paste flashes are 43% of front copper flashes. Through-hole pads and non-pasted copper explain much of this, but compare every SMD BOM item against paste apertures before ordering.
3. Project stackup says copper finish “None.” Explicitly set/document the intended finish (for example ENIG or lead-free HASL) before generating final outputs.
4. Re-export Gerbers after all electrical/layout corrections; the present archive is not suitable for ordering because it predates the required fixes.

## Verification and Bring-Up Plan

After revision:

1. Run KiCad ERC and DRC with all zones refilled.
2. Re-run schematic, PCB, cross, EMC, thermal, and Gerber analysis.
3. Power from a current-limited bench supply with the ESP32 removed; verify +5 V, `VCC_Daisy`, and `VREF`.
4. Install Daisy only and measure audio noise/distortion.
5. Install ESP32 and compare audio FFT/noise floor with Wi-Fi disabled, idle, continuous transmit, and maximum TX power.
6. Probe `VCC_ESP32`, `VCC_Daisy`, and `VREF` simultaneously. Look for Wi-Fi burst-correlated ripple.
7. Near-field probe the antenna area, ESP power branch, `VREF`, op-amp inputs, and audio connector wiring.
8. Check ESP32 throughput/RSSI with the final enclosure, display, wiring, and jacks installed.
9. Perform an A/B test with the ESP32 antenna physically outside the carrier edge. A large noise or RSSI improvement confirms antenna/copper coupling.

## Final Readiness Checklist

- [ ] TL072 replacement selected and validated at 5 V
- [ ] Every unused op-amp channel terminated safely
- [ ] 100 nF local bypass at every analog IC
- [ ] Exact ESP32 and Daisy module identities documented
- [ ] ESP32 footprint includes antenna/body/courtyard/keepout
- [ ] No copper/tracks/vias under antenna on any layer
- [ ] Antenna overhang or exact vendor-approved edge clearance implemented
- [ ] Layer 2 changed to solid GND or equivalent continuous adjacent reference demonstrated
- [ ] Analog signal transitions have adjacent GND vias
- [ ] `VREF` routing shortened and locally bypassed
- [ ] Ferrite MPN/current/DCR verified
- [ ] Off-board jack/harness intent documented
- [ ] Native ERC/DRC clean after zone refill
- [ ] Final Gerbers visually inspected and regenerated
- [ ] Wi-Fi TX audio-noise test passed in the final enclosure
