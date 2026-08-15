# rampg

[![CI](https://github.com/aajll/rampg/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/rampg/actions/workflows/ci.yml)

A lightweight, unit-agnostic ramp generator with linear and S-curve (sigmoid) profiles, asymmetric rise/fall rates, and output clamping, designed for deterministic embedded control loops in C11.

## Features

- **Linear ramping** between a current value and a configurable target at a caller-specified rate
- **S-curve (sigmoid) profile** with a flat start and end, sized so the peak rate equals the configured rate, via `rampg_set_shape`
- **Asymmetric rates** with independent rise and fall settings via `rampg_set_rates`
- **Runtime introspection** with `rampg_get_rate` (effective rate, units/s) and `rampg_get_state` (moving or at-target)
- **Output clamping** to caller-supplied minimum and maximum limits, applied on every update
- **Unit-agnostic** plain `float` output. Caller decides the meaning (volts, hertz, amps, RPM, etc.)
- **Zero allocation** with caller-owned `rampg_t` storage (stack, static, or embedded in a larger struct)
- **Deterministic** time-step semantics. Caller supplies `dt` in seconds; the library has no internal timers or OS dependencies
- **Compile-time configuration** of defaults via `rampg_conf.h`, overridable before the public header is included
- **No runtime errors** out of the public API. Preconditions are documented via `@pre` annotations

## Requirements

- A C11-compatible toolchain
- A conformant `<stdbool.h>` (the public API uses `bool`)
- IEEE-754 binary32 `float` (every modern embedded target the library targets uses this)

## Installation

### Copy-in (recommended for embedded targets)

Copy three files into your project tree:

```
include/rampg.h
include/rampg_conf.h
src/rampg.c
```

Then include the public header:

```c
#include "rampg.h"
```

`rampg_conf.h` is auto-included by `rampg.h`. Make sure both headers are reachable from the same include path.

### Meson subproject

Add this repo as a wrap dependency or subproject:

```meson
rampg_dep = dependency('rampg', fallback: ['rampg', 'rampg_dep'])
```

The project also calls `meson.override_dependency('rampg', ...)` so downstream Meson builds can resolve the subproject dependency by name without a wrap file.

For subproject builds, include the public header directly:

```c
#include "rampg.h"
```

### Installed dependency

If the library is installed system-wide, include the namespaced header path:

```c
#include <rampg/rampg.h>
```

If `pkg-config` files are installed in your environment, downstream builds can also discover the package as `rampg`. The generated version header is available as `rampg_version.h` in the build tree and as `<rampg/rampg_version.h>` after install.

## Quick Start

```c
#include "rampg.h"

/* Ramp a DC bus voltage from 0 to 400 V at 50 V/s */
rampg_t vbus;
rampg_init(&vbus, 0.0f);
rampg_set_rate(&vbus, 50.0f);          /* 50 V/s */
rampg_set_limits(&vbus, 0.0f, 800.0f); /* clamp to valid range */
rampg_set_target(&vbus, 400.0f);

/* Called from a 1 kHz control loop */
while (!rampg_at_target(&vbus)) {
        float v = rampg_update(&vbus, 0.001f);
        /* apply v to hardware */
}
```

### Asymmetric ramp example

A common control pattern is a slow ramp-up paired with a fast trip-down on fault. Use `rampg_set_rates` to configure the two directions independently:

```c
rampg_t vbus;
rampg_init(&vbus, 0.0f);
rampg_set_rates(&vbus, 50.0f, 2000.0f);  /* 50 V/s up, 2 kV/s down */
rampg_set_limits(&vbus, 0.0f, 800.0f);
rampg_set_target(&vbus, 400.0f);
```

### S-curve example

For a smoother transition with a flat start and end, select the sigmoid profile. The configured rate sets the peak rate; the move begins and ends at zero velocity and is re-planned from the current value whenever the target, limits, or rates change.

```c
rampg_t vbus;
rampg_init(&vbus, 0.0f);
rampg_set_rate(&vbus, 50.0f);          /* 50 V/s peak */
rampg_set_limits(&vbus, 0.0f, 800.0f);
rampg_set_shape(&vbus, RAMPG_SHAPE_SIGMOID);
rampg_set_target(&vbus, 400.0f);
```

For the design, peak-rate sizing, re-planning behaviour, and generated graphs, see the [S-curve profile documentation](docs/s-curve-profile.md).

### Monitoring ramp progress

`rampg_get_rate` and `rampg_get_state` expose live progress without advancing the ramp. In a sigmoid move the rate starts and ends at zero and peaks at the configured rate mid-move, which is useful for dashboards and for validating a move before acting on it.

```c
float v = rampg_update(&vbus, 0.001f);
if (rampg_get_state(&vbus) == RAMPG_STATE_MOVING) {
        /* ramping at rampg_get_rate(&vbus) units/s */
} else {
        /* at rest at the effective target */
}
```

## Building

```sh
# Library only (release)
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# With unit tests (host)
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

CI runs the test build under AddressSanitizer (and UndefinedBehaviorSanitizer on Linux) on both Linux and macOS, plus a release build job that matches the configuration a downstream consumer would use.

## API Reference

### Lifecycle

```c
void rampg_init(rampg_t *ramp, float initial);
void rampg_reset(rampg_t *ramp, float value);
```

`rampg_init` seeds both the current value and the target to `initial`, and sets rates and limits to the compile-time defaults from `rampg_conf.h`. `rampg_reset` snaps the output to `value` clamped against the active limits and updates the stored target to `value` (preserving the unclamped intent).

### Configuration

```c
void rampg_set_target(rampg_t *ramp, float target);
void rampg_set_rate(rampg_t *ramp, float rate);
void rampg_set_rates(rampg_t *ramp, float rise_rate, float fall_rate);
void rampg_set_limits(rampg_t *ramp, float min, float max);
void rampg_set_shape(rampg_t *ramp, rampg_shape_t shape);
```

`rampg_set_rate` applies the same rate to both directions. `rampg_set_rates` configures them independently. `rampg_set_limits` clamps the current value to the new range immediately. `rampg_set_shape` selects the ramp profile. The stored target is not modified, so widening the limits later recovers the original intent.

### Runtime

```c
float rampg_update(rampg_t *ramp, float dt);
float rampg_get(const rampg_t *ramp);
bool  rampg_at_target(const rampg_t *ramp);
float rampg_get_rate(const rampg_t *ramp);
rampg_state_t rampg_get_state(const rampg_t *ramp);

`rampg_update` advances the output toward the effective target (the stored target clamped to the active limits) using the configured shape, then re-clamps to the active limits. In LINEAR shape it steps by `rate * dt` and snaps to the effective target when the step would overshoot. In SIGMOID shape it follows a quintic S-curve re-planned from the current value whenever the target, limits, or rates change, sized so the peak rate equals the configured rate. It returns the updated output value.

`rampg_at_target` reports whether the output equals the effective target. This uses exact float equality, which is reachable because `rampg_update` explicitly snaps to the effective target when the step would overshoot.

`rampg_get_rate` returns the effective rate in units per second (signed). For LINEAR this is the configured rise or fall rate while moving and zero when at rest; for SIGMOID it is the instantaneous slope of the planned S-curve, peaking at the configured rate mid-move. `rampg_get_state` returns `RAMPG_STATE_MOVING` or `RAMPG_STATE_AT_TARGET`, where `rampg_at_target` is the boolean form of `RAMPG_STATE_AT_TARGET`. Both getters evaluate against the effective target (the stored target clamped to the active limits), so a target that lies outside the limits resolves to the clamped value and the ramp reports at-target with a zero rate once it reaches it.

For per-function documentation, see the Doxygen comments in `include/rampg.h`.

### Struct layout

```c
typedef struct {
        float value;         /* Current output value */
        float target;        /* Stored target (unclamped) */
        float rise_rate;     /* Rise rate, units/s */
        float fall_rate;     /* Fall rate, units/s */
        float limit_min;     /* Output clamp minimum */
        float limit_max;     /* Output clamp maximum */
        rampg_shape_t shape; /* Ramp profile (linear or S-curve) */
        float move_start;    /* Start value of the planned move */
        float move_end;      /* End value of the planned move */
        float move_duration; /* Planned move duration in seconds */
        float move_elapsed;  /* Elapsed move time in seconds */
        bool plan_valid;     /* True when the planned move is current */
} rampg_t;
```

`rampg_t` is a plain aggregate with no pointers. It is safe to `memcpy`, embed in a larger struct, or place in shared memory provided the usual thread-safety caveats are respected.

### Configuration macros

| Macro                 | Default              | Meaning                                                            |
| --------------------- | -------------------- | ------------------------------------------------------------------ |
| `RAMPG_DEFAULT_RATE`  | `100.0f`             | Default rise and fall rate set by `rampg_init` (units per second). |
| `RAMPG_LIMIT_MIN`     | `-1000000.0f`        | Default output clamp minimum set by `rampg_init`.                  |
| `RAMPG_LIMIT_MAX`     | `1000000.0f`         | Default output clamp maximum set by `rampg_init`.                  |
| `RAMPG_DEFAULT_SHAPE` | `RAMPG_SHAPE_LINEAR` | Default ramp profile set by `rampg_init`.                          |

Override any of these by defining the macro before including `rampg.h`:

```c
#define RAMPG_DEFAULT_RATE 500.0f
#include "rampg.h"
```

## Preconditions and Validation

The public API does not perform runtime precondition checks. Preconditions are documented per function via `@pre` annotations in `include/rampg.h`. Honouring them is the caller's responsibility.

The contract for each function is:

| Function           | Preconditions                                                      |
| ------------------ | ------------------------------------------------------------------ |
| `rampg_init`       | `ramp != NULL`.                                                    |
| `rampg_reset`      | `ramp` has been initialised.                                       |
| `rampg_set_target` | `ramp` has been initialised.                                       |
| `rampg_set_rate`   | `ramp` has been initialised. `rate > 0`.                           |
| `rampg_set_rates`  | `ramp` has been initialised. `rise_rate > 0` and `fall_rate > 0`.  |
| `rampg_set_limits` | `ramp` has been initialised. `min <= max`.                         |
| `rampg_set_shape`  | `ramp` has been initialised.                                       |
| `rampg_update`     | `ramp` has been initialised. `dt >= 0`.                            |
| `rampg_get`        | `ramp` has been initialised.                                       |
| `rampg_at_target`  | `ramp` has been initialised.                                       |
| `rampg_get_rate`   | `ramp` has been initialised.                                       |
| `rampg_get_state`  | `ramp` has been initialised.                                       |

Inputs that violate these preconditions invoke undefined behaviour in the same sense as any C library function. Validate at the call site if your application cannot guarantee them.

## Use Cases

1. **DC bus precharge** with slow voltage ramp-up and a fast trip-down on fault, expressed naturally with `rampg_set_rates`.
2. **Output voltage/frequency setpoint shaping** for smooth transitions between operating points.
3. **Bounded rapid shutdown** as a two-stage ramp-then-trip sequence.
4. **Motor speed control** profiles with controlled acceleration and deceleration.
5. **Operator setpoint smoothing** for any unit-agnostic scalar where a sudden jump would be undesirable.

## Platform Support

### Supported architectures

rampg works on any C11 toolchain with an IEEE-754 `float` and a conformant `<stdbool.h>`. There are no chip-specific code paths.

| Requirement               | Notes                                                                                     |
| ------------------------- | ----------------------------------------------------------------------------------------- |
| C11 toolchain             | Any C99 compiler with a working `<stdbool.h>` will also build but is not exercised by CI. |
| IEEE-754 binary32 `float` | Universal on real targets.                                                                |
| `<stdbool.h>` with `bool` | Used in the public API for `rampg_at_target`.                                             |

Targets meeting these requirements are expected to work, including (but not limited to) x86_64, AArch64, ARMv7-M, ARMv8-M, RISC-V, AVR, and the TI C2000 family.

### Validated configurations

| Toolchain   | Target       | Status                       |
| ----------- | ------------ | ---------------------------- |
| GCC, Clang  | x86_64 Linux | Run in CI under ASan + UBSan |
| Apple Clang | x86_64 macOS | Run in CI under ASan         |

Other architectures from the supported list above are expected to work but are not yet routinely validated. If you bring up rampg on a new target, please report back so the table can be extended.

## Limitations

rampg is a single-axis control primitive. The following are explicitly out of scope:

- **No internal time source**: the caller supplies `dt`. The library does not query a clock or depend on an OS.
- **No runtime precondition checks**: preconditions are part of the contract, not enforced at runtime. See [Preconditions and Validation](#preconditions-and-validation).
- **No arbitrary shaping**: the two available profiles are the linear step and the quintic S-curve. There is no user-defined profile, jerk limit, or per-segment shaping.
- **No thread safety**: the caller must provide mutual exclusion when a `rampg_t` is shared across threads or interrupt service routines.
- **No persistence**: the `rampg_t` is volatile by design. Save and restore externally if you need persistence across resets.

## Notes

| Topic                 | Note                                                                                                                                                                     |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Memory**            | All operations use caller-owned storage. No dynamic allocation.                                                                                                          |
| **Memory layout**     | `rampg_t` is a plain aggregate with no pointers. Safe to `memcpy`, embed, or place in shared memory.                                                                     |
| **Thread safety**     | Not thread-safe. Caller must synchronise if a `rampg_t` is shared across threads or ISRs.                                                                                |
| **Error handling**    | No runtime validation. Preconditions are documented via `@pre`; honouring them is the caller's responsibility.                                                           |
| **Floating point**    | All values are single-precision `float`, suitable for embedded targets.                                                                                                  |
| **Time source**       | Caller supplies `dt` in seconds. The library has no dependency on clocks or OS.                                                                                          |
| **WCET**              | Execution time is bounded and constant per call. No loops on input data; arithmetic is fixed.                                                                            |
| **Limits and target** | The stored target is unclamped. `rampg_update`, `rampg_at_target`, `rampg_get_rate`, and `rampg_get_state` evaluate against the target clamped to the active limits, so widening limits later recovers intent. |
| **Configuration**     | Override `RAMPG_DEFAULT_RATE`, `RAMPG_LIMIT_MIN`, `RAMPG_LIMIT_MAX`, and `RAMPG_DEFAULT_SHAPE` before including `rampg.h`, or via a toolchain-level `-D` flag. |
| **Version header**    | `rampg_version.h` is auto-generated by the Meson build and placed in the output build folder.                                                                            |
