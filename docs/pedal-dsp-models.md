# Pedal DSP Models

These are original, embedded-friendly models guided by public descriptions,
measurements, and the audible behavior of the named pedals. They do not claim
to reproduce proprietary digital code or exact protected circuit designs. All
API values are normalized from `0.0` to `1.0`; tapers and physical ranges are
mapped inside each effect.

## TC Hall of Fame 2-style reverb

Registry name: `TCHallOfFame2`

Tone: a clean dry path with an additive, dense wet field. Room is compact and
early-reflection-led; Hall is smooth; Spring is resonant and transient-sensitive;
Plate is fast and bright; Church is long and dark; Shimmer blooms one octave up
inside the tail; Mod moves slowly; LoFi is dark, quantized, and grainy.

| Parameter | Range | Default | Internal mapping |
| --- | ---: | ---: | --- |
| `decay` | 0-1 | 0.48 | Mode-dependent FDN feedback, up to 0.968 |
| `tone` | 0-1 | 0.48 | In-loop damping, approximately 520 Hz-13.3 kHz |
| `level` | 0-1 | 0.28 | Quadratic wet gain; dry remains unity |
| `predelay` | 0-1 | 0.16 | Quadratic 2-120 ms |
| `mode` | 0-1 | 1/7 | Room, Hall, Spring, Plate, Church, Shimmer, Mod, LoFi |
| `mash` | 0-1 | 0 | Mode macro: decay, drip, brightness, shimmer, motion, or degradation |

Approximation: eight mutually prime delay lines use an orthonormal Hadamard
feedback matrix, mode-specific geometry, early reflections, diffusion, and
feedback damping. Modes crossfade for 55 ms. Shimmer uses a dual-grain +1 octave
shift in the tank feedback; Spring adds dispersive delay geometry and two
transient-driven resonances.

Presets:

| Use | Decay | Tone | Level | PreDelay | Mode | MASH |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Subtle room | 0.20 | 0.43 | 0.18 | 0.04 | 0 | 0.10 |
| Classic hall | 0.58 | 0.52 | 0.34 | 0.22 | 1/7 | 0.15 |
| Shimmer bloom | 0.82 | 0.58 | 0.42 | 0.30 | 5/7 | 0.72 |

## TC Sub N Up-style octaver

Registry name: `TCSubNUp`

Tone: Poly is clean, low-latency granular transposition that retains chord
content. Classic is intentionally monophonic and square-edged, with filtered,
envelope-following divider voices and modest tracking artifacts.

| Parameter | Range | Default | Internal mapping |
| --- | ---: | ---: | --- |
| `dry` | 0-1 | 1.00 | Quadratic voice gain |
| `up` | 0-1 | 0.32 | Quadratic +1 octave gain |
| `sub` | 0-1 | 0.42 | Quadratic -1 octave gain, 4.2 kHz low-pass |
| `sub2` | 0-1 | 0.00 | Quadratic -2 octave gain, 2.25 kHz low-pass |
| `mode` | 0-1 | 0 | Poly at 0, Classic at 1; smoothed crossfade |

Approximation: Poly uses two Hann-crossfaded delay grains per voice with a
21 ms window, preserving polyphonic input without pitch detection. Classic uses
qualified zero crossings, period confidence, silence rejection, envelope-shaped
oscillators, and low-pass filtering. Voice-power normalization preserves useful
headroom when several controls are high.

Presets:

| Use | Dry | Up | Sub | Sub2 | Mode |
| --- | ---: | ---: | ---: | ---: | ---: |
| Subtle thickener | 1.00 | 0.16 | 0.24 | 0 | Poly |
| Modern octave stack | 0.85 | 0.52 | 0.58 | 0.22 | Poly |
| Vintage synth bass | 0.30 | 0.18 | 0.82 | 0.64 | Classic |

## Fulltone OCD-style overdrive

Registry name: `FulltoneOCD`

Tone: broad low end, open transient response, and compound asymmetric clipping.
LP is flatter and more transparent; HP raises pre-clipping gain, trims more bass,
and adds upper-mid bite.

| Parameter | Range | Default | Internal mapping |
| --- | ---: | ---: | --- |
| `drive` | 0-1 | 0.38 | Quadratic 1.25x-32.25x gain |
| `tone` | 0-1 | 0.52 | Exponential 1.75-12.6 kHz post-clipping contour |
| `volume` | 0-1 | 0.50 | Quadratic output gain with drive compensation |
| `peak_mode` | 0-1 | 0 | LP at 0, HP at 1; smoothed gain/filter transition |

Approximation: the cascade includes input bandwidth limiting, a wide-band bass
split, LP/HP-dependent drive, a 2x-oversampled compound asymmetric soft clipper,
presence and tone networks, output compensation, and a DC blocker. The open
clipping law deliberately compresses less than the project's TS-style model.

Presets:

| Use | Drive | Tone | Volume | Peak mode |
| --- | ---: | ---: | ---: | --- |
| Always-on edge | 0.16 | 0.48 | 0.58 | LP |
| Open crunch | 0.46 | 0.56 | 0.52 | LP |
| British push | 0.72 | 0.63 | 0.46 | HP |

## BOSS BD-2-style Blues Driver

Registry name: `BossBD2`

Tone: bright, full-range breakup with strong pick and guitar-volume response.
Low gain remains nearly clean; high gain blends a fuzzy hard edge into the
asymmetric multi-stage saturation without losing the interstage bass control.

| Parameter | Range | Default | Internal mapping |
| --- | ---: | ---: | --- |
| `gain` | 0-1 | 0.34 | Quadratic gain across two clipping stages |
| `tone` | 0-1 | 0.50 | Exponential 1.25-11.9 kHz contour plus high shelf |
| `level` | 0-1 | 0.52 | Quadratic output gain with gain compensation |

Approximation: a 68 Hz input high-pass feeds two different 2x-oversampled
asymmetric stages. A frequency-dependent interstage path controls bass drive;
an envelope follower increases headroom on transients, and the second stage
adds progressively harder edges at high gain. Tone, level compensation, and DC
blocking follow the clipping cascade.

Presets:

| Use | Gain | Tone | Level |
| --- | ---: | ---: | ---: |
| Clean bloom | 0.12 | 0.46 | 0.62 |
| Blues grit | 0.43 | 0.52 | 0.52 |
| Fuzzy lead | 0.82 | 0.38 | 0.43 |

## Validation

Run the host smoke test from `firmware/daisy`:

```sh
g++ -std=c++14 -O2 -I. Utils/dsp_model_smoke_test.cpp -o build/dsp_model_smoke_test
./build/dsp_model_smoke_test
```

The test checks normalized metadata, sample-exact bypass, finite/bounded output,
DC settling, large parameter changes, mode changes, and octaver recovery after
silence. `make` verifies all four models and SDRAM sections with the ARM toolchain.

Hardware checks should use a 48 kHz capture and the Daisy cycle counter:

1. Send a -18 dBFS 220 Hz sine and pink noise through each unity preset; compare
   bypass RMS and confirm only intentional drive presets clip.
2. Automate every parameter from 0 to 1 over 100 ms and inspect for discontinuities.
3. Run a 30-second impulse tail at maximum reverb decay in all modes; assert the
   peak falls below 0 dBFS and never grows. Repeat while changing modes.
4. Feed single notes, dyads, full chords, silence, and noisy muted strings into
   both octaver modes. Confirm Classic confidence fades during silence and locks
   again without a burst; measure Poly latency near its 21 ms grain window.
5. Record 3, 6, and 9 kHz sines through both drives at maximum gain. Compare FFT
   energy folded below Nyquist against a non-oversampled reference.
6. Measure audio callback cycles with the final four-lane routing configuration.
   Keep worst-case callback time below 70% of the block deadline, then audition
   with rapid transients to catch cache-related overruns.