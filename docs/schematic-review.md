# ChimeraMultiFX Schematic Review

Review date: 2026-08-09  
Design reviewed: `hardware/boards/daisyBoard.kicad_sch`  
KiCad version used for checks: 10.0.3

## Scope and product assumptions

ChimeraMultiFX is treated as a dual-MCU guitar effects platform. The Daisy Seed owns the real-time stereo audio path at 48 kHz, while an ESP32 development board owns Wi-Fi, the browser UI, and control-plane communication over a 115200 baud UART. The board also provides two expression inputs and separate microSD interfaces for the Daisy and ESP32.

This is a design review, not a certification or a substitute for prototype measurements. Component recommendations must be confirmed against the exact manufacturer part numbers, source impedance, expected audio level, enclosure, and external power adapter.

## Executive decision

**Status: not ready for fabrication as a production revision.**

The architecture is workable, but the current schematic has one fundamental analog compatibility problem and several release blockers:

1. The four TL072 devices are not suitable for a nominal 5 V single-supply design.
2. KiCad ERC reports 98 unresolved violations.
3. External audio, expression, power, and SD connections lack a complete protection and EMC strategy.
4. The power-source contract is ambiguous and the board does not implement the regulator/protection stage described in its notes.
5. Some filter, headroom, SD-interface, and unused-op-amp details need explicit design calculations or datasheet-backed values.

Do not order the present design as a final PCB. A small engineering prototype is reasonable only after the P0 items below are corrected and ERC is clean.

## What is already sound

- Audio channels are symmetrical and use AC coupling, a high input impedance, VREF biasing, buffering, output isolation, and output DC blocking.
- A buffered midrail reference is provided instead of biasing every stage directly from a resistor divider.
- Daisy and ESP32 power branches include ferrite-bead isolation and local bulk/high-frequency capacitors.
- GNDA and GNDD are joined explicitly through `NT1`, which makes the intended current boundary reviewable.
- UART labels agree with the working firmware architecture: Daisy D13/PB6 transmits to ESP32 GPIO16 RX, and ESP32 GPIO17 TX drives Daisy D14/PB7 RX.
- Test points are provided for supply rails, VREF, expression channels, audio channels, and grounds.

## Priority findings

### P0-1: TL072 cannot be used from the shown 5 V single supply

`IC1` through `IC4` are TL072 devices powered from `+5V` and `GNDA`, with the signal centered on an approximately 2.5 V VREF. A standard TL072 is specified for a substantially higher total supply voltage and is neither rail-to-rail at its input nor output. At 5 V, correct bias, gain, noise, and output swing are not guaranteed. The result can be clipping, phase reversal or nonlinear behavior, startup latch-like behavior, and large channel-to-channel variation.

**Required action:** replace all TL072 stages with an audio-suitable op-amp explicitly specified at 5 V single supply, with input common-mode including 2.5 V and enough output swing for the Daisy codec input. Candidate families to evaluate include OPA1652, OPA1678, OPA1692, or another unity-gain-stable low-noise device that is available in the required package. Do not substitute solely by pinout; compare noise density, input bias current, capacitive-load stability, output current, common-mode range, and output swing at the intended load.

Also account for the current 100 pF feedback capacitor and any capacitive load created by the Daisy input or wiring. Recalculate stability after selecting the amplifier.

### P0-2: Resolve all ERC violations before release

KiCad 10.0.3 reports:

| ERC class | Count | Review interpretation |
| --- | ---: | --- |
| Dangling labels | 53 | Mostly controller breakout labels that are not actually attached |
| Unconnected wire endpoints | 30 | Tiny open stubs, generally 0.0127 mm long |
| Undriven power/input pins | 10 | Requires `PWR_FLAG`, corrected pin types, or real wiring fixes |
| Dangling no-connect flags | 3 | No-connect markers are not attached to pins |
| Library symbol mismatch | 2 | Both microSD symbols differ from the current library copy |

The report includes power errors on op-amp, SD, and rail symbols. Some are likely ERC-modeling issues, but they cannot be waived as a group because the same report can conceal a real open connection.

**Required action:** remove decorative labels and zero-length stubs, attach intentional no-connect flags directly to pins, update or rescue the SD symbols, and use power-output symbols or `PWR_FLAG` only where the physical source is understood. Record any justified exclusions individually.

**Release criterion:** zero unexplained ERC errors and zero unexplained warnings.

### P0-3: Define and protect the power input

The schematic describes an off-board USB-C input and elsewhere mentions a 9 V or 12 V source with a buck/linear regulator combination, but the implemented board accepts a `+5V` rail directly at a solder pad. The board contains ferrite branches, not a buck converter or a complete protected power-entry stage.

This ambiguity is hazardous: applying 9 V or 12 V to the implemented `+5V` node can damage the ESP32 development board, SD card, and analog circuitry.

**Required action:** choose one explicit contract and put it on the schematic and silkscreen:

- **Regulated 5 V input:** specify tolerance, ripple, minimum current, connector pinout, and grounding. Add a fuse or resettable fuse, reverse-polarity/ideal-diode protection, input TVS sized for the chosen connector, and local bulk plus ceramic bypassing.
- **Pedal 9 V input:** add a protected buck stage that produces 5 V with adequate transient response and conducted-noise filtering. Select the converter and post-filter from measured audio-band and RF noise, not only its DC rating.

Avoid powering the assembly simultaneously from the board input and either development board's USB connector unless power-path isolation is explicitly implemented.

### P0-4: Add connector-level protection to all off-board analog lines

The audio jacks and expression jacks are represented as bare test-point/solder-pad connections. Their signal paths do not show a complete IEC ESD or RF-ingress strategy. Guitar cables, expression cables, and hand-accessible jacks routinely inject static discharge, cable transients, radio-frequency energy, and incorrect voltages.

**Required action:** at each physical jack entry, add:

- A low-capacitance ESD suppressor selected for the allowed signal range.
- A defined series impedance and RF shunt close to the connector, coordinated with the existing audio filter.
- A discharge/bias path on both sides of coupling capacitors to reduce plug-in pops.
- A documented jack switching and sleeve-ground scheme.
- Protection for expression ADC inputs against shorts to ground, 3.3 V, 5 V, and common miswired TRS pedals.

Connect ESD current to chassis/entry ground with the shortest practical path. Do not route an ESD pulse through the quiet analog ground plane before it reaches the supply return.

### P1-1: Verify audio headroom and codec limits quantitatively

The input stages appear to be unity-gain, VREF-centered buffers. That is appropriate only if the maximum guitar/line signal remains inside both the replacement op-amp range and the Daisy codec input range.

Document these values before freezing the design:

- Maximum expected input in Vpeak and Vrms, including boosted pedals.
- Daisy audio input full-scale voltage and allowable DC bias.
- VREF tolerance, noise, startup time, and maximum load.
- Replacement op-amp linear input/output range at worst-case supply and load.
- Required margin; target at least 6 dB above the normal maximum signal.

The available symmetrical signal swing is approximately

$$V_{PK,available} = \min(V_{REF}-V_{OL},\ V_{OH}-V_{REF})$$

using guaranteed, loaded output limits rather than typical no-load graphs.

### P1-2: Recalculate the audio filters as complete networks

The schematic uses 1 µF coupling capacitors, 1 MΩ bias resistors, 1 kΩ series resistors, 100 pF feedback capacitors, and 100 Ω output resistors. Each isolated value is plausible, but the effective poles depend on the source, load, codec input impedance, cable capacitance, and op-amp output impedance.

For every AC-coupled node, document

$$f_c = \frac{1}{2\pi R_{effective}C}$$

For example, 1 µF with an ideal 1 MΩ bias path gives about 0.16 Hz, but the real pole changes when parallel loading is included. Confirm the input RF low-pass and feedback capacitor do not reduce wanted audio bandwidth or compromise phase margin. Add a deliberate anti-alias/RF pole rather than relying on incidental parasitics.

### P1-3: Strengthen op-amp and VREF decoupling

The board has only five 100 nF capacitors for seven dual op-amps plus the surrounding reference and rail functions. Every dual op-amp package should have a dedicated 100 nF X7R capacitor directly between its supply pins, with a very short return. Add local 1-10 µF capacitance per analog cluster where transient currents warrant it.

The VREF buffer must remain stable while driving all audio bias resistors and any bypass capacitance. Verify the selected buffer's capacitive-load stability. If needed, isolate the VREF output with a small resistor before its bulk capacitor and route VREF as a quiet reference, not as a general power rail.

### P1-4: Complete both microSD interface designs

Each SD interface needs a datasheet-backed set of pull-ups, local decoupling, and signal conditioning. Confirm the following for both `J5` and `J12`:

- 10-100 µF local bulk plus 100 nF at the socket supply.
- Required pull-ups on CMD/DAT lines for the selected SPI or SDMMC mode.
- Series damping resistors close to the driving MCU; 22-47 Ω is a reasonable starting range to validate.
- Card-detect behavior, if needed.
- ESD protection at a user-accessible socket.
- No boot-strap conflict on ESP32 pins.

The Daisy firmware currently does not appear to use the SDMMC socket in the active application. Decide whether the unvalidated interface should remain populated in this revision.

### P1-5: Verify expression-pedal compatibility and ADC settling

The expression circuits buffer a reference and return signal through MCP6002 stages and large-value resistors. Confirm support for the actual pedal wiring conventions: TRS tip/ring polarity, 10 kΩ versus 100 kΩ potentiometers, passive versus active pedals, and cable hot-plugging.

Check the Daisy ADC's required source impedance and acquisition time. An op-amp may be stable in DC service but ring when directly driving the ADC sample capacitor. Provide a small series resistor and local capacitor at the ADC pin if recommended by the STM32/Daisy reference design. Add clamp-current limiting independently of the op-amp.

### P1-6: Terminate every unused amplifier section deliberately

The design uses four TL072s and three MCP6002s, which provide fourteen amplifier channels. Verify that every section is used. Any unused section must not float. Configure it as a unity-gain follower at VREF, following the replacement amplifier manufacturer's guidance.

### P2-1: Replace ambiguous names and notes with electrical specifications

Names such as `VCC_Daisy`, `VCC_ESP32`, `ESP-3.3V`, `+3.3VA`, `+3.3VD`, `GNDA`, and `GNDD` are useful only when their sources and permitted connections are explicit. Add a power tree with nominal voltage, tolerance, current budget, source, and expected load. Correct note typos such as the TFT current listed as approximately 500 A.

Use consistent signal capitalization. `Audio In 1`, `AUDIO_IN_1`, and similar near-duplicates invite wiring mistakes.

## DSP and mixed-signal considerations

- Keep all analog gain before the codec only as high as necessary. Digital effects such as compression, overdrive, pitch shifting, and reverb need predictable codec headroom more than extra analog gain.
- Wi-Fi burst current and SD writes are likely dominant interference sources. Validate the analog noise floor with Wi-Fi transmitting, both SD interfaces active, and worst-case DSP load.
- Define the analog bypass behavior on boot, reset, firmware crash, and brownout. The current design has no relay or analog fail-safe bypass.
- If true pedal bypass is a product requirement, add a relay or low-distortion analog switch architecture and define pop suppression.
- Establish one gain convention in firmware and hardware: input full scale, internal dBFS headroom, wet/dry summing headroom, codec output full scale, and final analog output level.

## Required schematic release checklist

- [ ] Replace TL072 with a verified 5 V audio op-amp and recheck all stages.
- [ ] Resolve all 98 current ERC violations; document only specific justified exclusions.
- [ ] Define exactly one power-input contract and add protection/regulation accordingly.
- [ ] Add ESD, RF, and fault-current protection at every external connector.
- [ ] Verify audio headroom against guaranteed codec and op-amp limits.
- [ ] Calculate all input, output, anti-alias, and RF poles with real impedances.
- [ ] Add one local 100 nF decoupler per op-amp package.
- [ ] Verify VREF load, noise, startup, and capacitive stability.
- [ ] Complete SD pull-up, decoupling, damping, ESD, and boot-pin design.
- [ ] Verify expression wiring conventions, ADC settling, and overvoltage behavior.
- [ ] Tie every unused amplifier input to a defined operating point.
- [ ] Add a power tree, current budget, and connector pinout table.
- [ ] Define safe behavior for USB plus external-power coexistence.

## Prototype validation plan

1. Power the empty/partially populated board from a current-limited supply. Verify no rail is back-fed and record inrush and steady current.
2. Measure 5 V, 3.3 V rails, and VREF noise from 10 Hz to at least 20 MHz with Wi-Fi idle, Wi-Fi transmitting, SD writes, and full DSP load.
3. Apply a swept sine from 20 Hz to 20 kHz at several amplitudes. Record gain, phase, THD+N, clipping level, channel matching, and residual DC.
4. Terminate the input with 50 Ω and with a representative guitar pickup impedance. Measure A-weighted and unweighted output noise.
5. Hot-plug every audio and expression connector and record peak transient level at the codec and output.
6. Test ESD first at reduced levels, then to the product target with the enclosure and jack grounding finalized.
7. Run an impulse and full-scale multitone through the maximum-DSP preset. Verify codec clipping and analog clipping cannot occur before the documented digital limit.

## Review evidence

- Native KiCad ERC: 98 violations as classified above.
- PCB/schematic parity check: zero mismatches.
- Firmware: active Daisy/ESP32 control link uses Daisy D13/D14 and ESP32 GPIO16/GPIO17 at 115200 baud.
- Design inventory: four TL072, three MCP6002, two microSD sockets, ferrite-isolated Daisy/ESP32 branches, and explicit GNDA/GNDD net tie.
