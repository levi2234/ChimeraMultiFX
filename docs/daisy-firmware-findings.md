# Daisy Seed Firmware Findings

Date: 2026-07-03

Scope reviewed:

- `firmware/daisy/main.cpp`
- `firmware/daisy/Effect.h`
- `firmware/daisy/Router.h`
- `firmware/daisy/SerialController.h`
- `firmware/daisy/effects/**`

## Summary

The current firmware architecture is clear and promising: effects share a small `Effect` interface, the router model is easy to reason about, and the serial command protocol gives a useful live-control surface.

The main risks are real-time safety and input validation. The code currently allows USB serial commands to mutate and delete the active audio graph while the audio callback is traversing it. Several command paths also accept invalid slot indices or report success when an operation silently fails. Those are the highest-priority issues to address before relying on this on hardware.

## Findings

### High: USB Commands Mutate the Live Audio Graph

USB receive handling feeds and executes commands from `UsbRxCallback` in `firmware/daisy/main.cpp`, while the audio callback continuously calls `router.Process(...)`. Commands such as `add`, `insert`, `remove`, `clear`, and `move` can allocate, delete, or rearrange effects while the audio thread is dereferencing `slots[i]`.

Relevant locations:

- `firmware/daisy/main.cpp`: `UsbRxCallback`, `SetReceiveCallback`, `StartAudio`
- `firmware/daisy/Router.h`: `Lane::ProcessChain`
- `firmware/daisy/SerialController.h`: `CmdAdd`, `CmdInsert`, `CmdRemove`, `CmdMove`, `CmdClear`

Possible effects:

- Hard faults from dereferencing a deleted effect pointer
- Occasional corrupted audio
- Hard-to-reproduce crashes during live editing

Recommended direction:

Queue parsed serial commands and apply graph mutations at a controlled synchronization point, or use a fixed preallocated graph/effect pool with atomic state changes. Avoid `new` and `delete` while audio is running.

### High: Slot Bounds Validation Is Incomplete

Some router operations do not reject negative or out-of-range target indices.

Examples:

- `Lane::Insert` checks `pos > count`, but not `pos < 0`.
- `Lane::Swap` checks `a < count && b < count`, but not negative indices.
- `CmdInsert`, `CmdSwap`, and `CmdMove` do not validate every target slot before mutating the router.

Relevant locations:

- `firmware/daisy/Router.h`: `Lane::Insert`, `Lane::Swap`, `Router::MoveEffect`
- `firmware/daisy/SerialController.h`: `CmdInsert`, `CmdSwap`, `CmdMove`

Possible effects:

- Out-of-bounds writes to `slots`
- Corrupted router state
- Hard faults

Recommended direction:

Add explicit validation for all source and destination lane/slot positions. Consider adding router-level helpers such as `CanInsert(lane, slot)`, `CanSwap(lane, a, b)`, and `CanMove(...)`, or make `Add`, `Insert`, `Swap`, and `MoveEffect` return `bool`.

### High: Failed Add/Insert/Move Operations Can Leak or Lose Effects

`CmdAdd` and `CmdInsert` create an effect before calling the lane operation. If the lane is full or the insert position is invalid, the router silently rejects the operation, but the newly allocated effect is not deleted and the command still replies with `OK`.

`MoveEffect` removes the effect from the source lane before knowing whether the destination insert can succeed. If the destination lane is full or the destination slot is invalid, the effect can be lost from the graph.

Relevant locations:

- `firmware/daisy/Router.h`: `Lane::Add`, `Lane::Insert`, `Router::MoveEffect`
- `firmware/daisy/SerialController.h`: `CmdAdd`, `CmdInsert`, `CmdMove`

Possible effects:

- Memory leaks
- Lost effects after failed moves
- Incorrect serial responses that make the host UI believe an operation succeeded

Recommended direction:

Validate capacity and target positions before allocating or moving effects. Make mutation methods report success/failure and only reply `OK` after a confirmed change.

### Medium: All Delay Instances Share One Static Buffer

`Delay` uses a single static SDRAM buffer. Because the serial factory can create multiple `Delay` instances, multiple delay effects will share the same memory while maintaining separate write pointers.

Relevant locations:

- `firmware/daisy/effects/time/Delay.h`: static `buffer_`
- `firmware/daisy/SerialController.h`: `CreateFromName("delay")`

Possible effects:

- Delay effects interfere with each other
- Cross-talk between lanes or slots
- Unexpected delay behavior when more than one delay is active

Recommended direction:

Either enforce a single delay instance, allocate buffer regions from a fixed delay-memory pool, or give each delay instance its own configured slice of SDRAM.

### Medium: DSP Parameters Are Mostly Unclamped

Many serial-controlled parameters accept arbitrary values. Invalid or extreme values can produce unstable DSP, NaNs, runaway feedback, or division by zero.

Examples:

- Delay `feedback` can exceed `1.0`.
- Delay `time` can be negative before conversion to `uint32_t`.
- Compressor `attack` and `release` can be zero or negative before division.
- Low-pass `cutoff` can exceed Nyquist or be negative.
- Low-pass `resonance` can exceed its expected range.
- Bitcrusher `rate` is assigned directly instead of using the safer `SetRateReduce` helper.

Relevant locations:

- `firmware/daisy/effects/time/Delay.h`
- `firmware/daisy/effects/dynamics/Compressor.h`
- `firmware/daisy/effects/filter/LowPass.h`
- `firmware/daisy/effects/distortion/Bitcrusher.h`

Recommended direction:

Define explicit parameter ranges per effect and clamp inside `SetParam`. The serial layer should not need to know every DSP invariant; each effect should protect itself.

### Medium: Serial Reply Can Read Past Its Stack Buffer

`Reply` formats output into a 128-byte stack buffer using `vsnprintf`. If the formatted string is longer than the buffer, `vsnprintf` returns the full length that would have been written. That value is passed directly to `TransmitInternal`, which can cause it to read beyond the end of `out`.

Relevant location:

- `firmware/daisy/SerialController.h`: `Reply`

Possible effects:

- Sending garbage bytes after long replies
- Reading unrelated stack memory
- Occasional unstable serial output

Recommended direction:

Clamp the transmit length after `vsnprintf`:

```cpp
if (len >= static_cast<int>(sizeof(out))) {
    len = sizeof(out) - 1;
}
```

Also consider increasing the reply buffer or using chunked replies for larger JSON status outputs.

## Positive Notes

- The `Effect` base class is compact and easy to extend.
- The router abstraction is readable and fits the intended multi-lane design.
- Header-only effects keep the current Makefile simple.
- The serial command protocol is human-readable and easy to test manually.
- `status` and `info` responses are a good foundation for a host UI.

## Suggested Fix Order

1. Add robust lane/slot validation and make router mutations return success/failure.
2. Fix `Reply` transmit length clamping.
3. Add parameter clamping inside each effect.
4. Replace live graph mutation from USB callback with a safer command queue or synchronized update model.
5. Decide on a deterministic memory model for effects, especially delay buffers.
6. Add lightweight host-side or native tests for command parsing and router mutation behavior.

## Validation Notes

VS Code diagnostics reported no errors for the reviewed core headers at the time of review. A `make` build was attempted but skipped, so compile/link validation was not completed during this pass.
