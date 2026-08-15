# S-curve (sigmoid) ramp profile

The `rampg` library ships two ramp profiles. The default, `RAMPG_SHAPE_LINEAR`, moves at a constant rate from the current value to the target. The S-curve profile, `RAMPG_SHAPE_SIGMOID`, moves along a smooth quintic that starts and ends at zero velocity, so the output eases in and out instead of stepping in and out. This document explains the S-curve profile: what it buys you, how it is configured, how it is computed, and what the generated graphs show.

The rest of the public API (targets, limits, resets, `rampg_update`) behaves exactly the same in both profiles. See the README for the full API reference.

## Why an S-curve

A linear step has a hard change in velocity at both ends. The rate jumps from zero to `rate` at the start of the move, and from `rate` back to zero at the end. For a physical actuator (a motor, a valve, a power supply) those two jumps are impulsive: they show up as a jerk that can excite mechanical resonance, spin up inductors, or trip a soft limit.

The S-curve removes those velocity steps. The rate starts at zero, ramps up to the configured peak, holds near that peak through the middle of the move, and ramps back down to zero. The output is still bounded by the same configured rate, but it is never applied abruptly.

The cost is time. Because the rate is only at its peak for part of the move, an S-curve takes **1.875× as long** as a linear step that covers the same distance at the same peak rate. For a 0 to 100 move at 100 units/s, the linear step finishes in about 1.0 s and the S-curve finishes in about 1.875 s. Use the S-curve where the smoother transition matters; use the linear profile where the move must be as fast as possible.

## How it is computed

The move follows a normalized quintic S-curve. Let `start` and `end` be the value at the beginning and end of the move, and let `u` run from 0 to 1 over the move duration. The output is

```
value = start + (end - start) * s(u)
```

where `s` is the standard quintic smoothstep. This particular polynomial has three properties that make it a good ramp:

- `s(0) = 0` and `s(1) = 1`, so the output reaches the target exactly.
- The first derivative is zero at both `u = 0` and `u = 1`. The output therefore starts and ends at **zero velocity**.
- The second derivative is also zero at both ends. The output starts and ends at **zero acceleration**, so the rate itself changes smoothly rather than kinking.

### Peak-rate sizing

The peak slope of the quintic is exactly `1.875` (at `u = 0.5`). To make the move's peak rate equal the configured rate, the move duration is set to

```
duration = 1.875 * distance / rate
```

so that `distance * 1.875 / duration == rate`. In other words, the configured rate is the **peak rate**, which is the same meaning it has in the linear profile. Two practical consequences:

- The average rate over the move is `distance / duration = rate / 1.875`, about 53% of the peak rate.
- The move takes `1.875 * distance / rate`, which is 1.875× the linear step time for the same distance and peak rate.

### Direction and asymmetric rates

The direction is decided by the sign of `end - start`. A rising move uses `rise_rate`, a falling move uses `fall_rate`. Setting these independently with `rampg_set_rates` gives an asymmetric S-curve, for example a slow controlled rise paired with a fast trip-down. The duration of each leg scales as `1/rate`, so the faster leg is shorter.

### Re-planning

The move is re-planned from the current value whenever the effective target, the limits, or the rates change. A re-plan resets the move clock to zero and keeps the output at its current value. The result is that a mid-move retarget or a rate change never introduces a jump: the output stays continuous and restarts at zero velocity toward the new goal. The same holds when switching to the SIGMOID profile; a switch to LINEAR resumes at full rate instead. This is the behaviour shown in the second graph, panel (A).

## Configuring it

Select the profile with `rampg_set_shape`. The default is linear, so a sigmoid move requires an explicit call. The configured rate is the peak rate.

```c
#include "rampg.h"

/* Ease a setpoint from 0 to 400 V with a 50 V/s peak rate. */
rampg_t vbus;
rampg_init(&vbus, 0.0f);
rampg_set_rate(&vbus, 50.0f);            /* 50 V/s peak */
rampg_set_limits(&vbus, 0.0f, 800.0f);
rampg_set_shape(&vbus, RAMPG_SHAPE_SIGMOID);
rampg_set_target(&vbus, 400.0f);

/* From the control loop, as usual. */
while (!rampg_at_target(&vbus)) {
    float v = rampg_update(&vbus, 0.001f);
    /* apply v */
}
```

For asymmetric rise and fall, use `rampg_set_rates` instead of `rampg_set_rate`. The rise leg uses the first argument as its peak rate and the fall leg uses the second:

```c
rampg_set_rates(&vbus, 50.0f, 400.0f);   /* 50 V/s up, 400 V/s down */
```

The default profile can also be set at compile time. Define `RAMPG_DEFAULT_SHAPE` before including `rampg.h`:

```c
#define RAMPG_DEFAULT_SHAPE RAMPG_SHAPE_SIGMOID
#include "rampg.h"
```

`rampg_set_shape` takes effect at the next `rampg_update`. If a move is in progress, it is re-planned from the current value, so switching profiles mid-move is also continuous.

## Reading the rate and state

Two read-only getters expose live progress without advancing the ramp: `rampg_get_rate` and `rampg_get_state`.

`rampg_get_rate` returns the **effective rate** in units per second. For a sigmoid move this is the instantaneous slope of the planned S-curve, so it reads zero before the first `rampg_update` (the plan does not exist yet), rises to the configured peak at the midpoint of the move, and falls back to zero at the end. A target, limit, rate, or shape change invalidates the plan, so the rate reads zero again until the next `rampg_update` re-plans from the current value. This is the same quantity that appears in the rate panels of the graphs below.

`rampg_get_state` returns `RAMPG_STATE_MOVING` while the output is ramping and `RAMPG_STATE_AT_TARGET` when the output is at rest at the **effective target** (the stored target clamped to the active limits). `rampg_at_target` is the boolean form of `RAMPG_STATE_AT_TARGET`. Because the state resolves against the clamped target, a target that lies outside the limits is treated as reachable at the clamped value: the ramp moves to the clamped value, then reports at-target with a zero rate. There is no separate "unreachable" state.

## Reading the graphs

The graphs below were generated by driving the real library and sampling the output at a fixed 4 ms step, so they show the actual behaviour rather than a re-derivation of it. The S-curve traces are in violet and the linear or reference traces are in blue and orange.

### Value and rate profiles

![S-curve and linear value and rate profiles](images/scurve_profiles_and_rates.png)

Both panels compare a 0 to 100 move at a 100 units/s peak rate.

- **Value profile** (top). The blue linear step is a straight line that reaches 100 in about 1.0 s. The violet S-curve is flat near 0, rises through 50 at the midpoint, and is flat again as it settles at 100. It finishes in about 1.875 s, the 1.875× time penalty.
- **Rate profile** (bottom). The blue line is flat at the configured 100 units/s for the whole move, with sharp corners at the start and end. The violet rate is zero at both ends, climbs to the same 100 units/s peak in the middle, and returns to zero. The two curves have the same peak, which is the point of the sizing.

### Mid-move retarget and asymmetric rates

![S-curve mid-move retarget and asymmetric rise/fall](images/scurve_retarget_and_asymmetric.png)

- **(A) Mid-move retarget.** The move starts toward a target of 100. At `t = 0.252 s`, with the output at about 1.96, the target is changed to 50. The violet trace does not jump. It re-plans from the current value and restarts at zero velocity toward the new target of 50.
- **(B) Asymmetric rise/fall.** The rise from 0 to 100 runs at 200 units/s and takes about 0.9375 s. The fall from 100 to 0 runs at 400 units/s, twice as fast, and takes about 0.46875 s, half as long. Duration scales as `1/rate`.

## Notes and limitations

- The profile is a quintic, so the output is continuous in value, velocity, and acceleration at the move boundaries. Jerk is finite but discontinuous at the boundary between the flat region and the curve. This is the standard trade-off for a fifth-order polynomial. A higher-order or piecewise profile would reduce the jerk step, but rampg deliberately ships a single fixed profile for a bounded, predictable cost.
- The configured rate is the peak rate. The move's average rate is lower, about 53% of the peak, and the move takes 1.875× the linear step time for the same distance.
- The move is re-planned on target, limit, or rate changes. The output stays continuous, but the in-progress shape is discarded and a fresh S-curve starts from the current value.
