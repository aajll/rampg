# rampg

[![CI](https://github.com/aajll/rampg/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/rampg/actions/workflows/ci.yml)

A lightweight, unit-agnostic linear ramp generator with asymmetric rise/fall rates and output clamping, designed for deterministic embedded control loops in C11.

## Features

- **Linear Ramping**: Smoothly transitions a float value toward a target at a configurable rate
- **Asymmetric Rates**: Independent rise and fall rates for different ramp-up and ramp-down behaviour
- **Output Clamping**: Configurable min/max limits enforced on every update
- **Unit-Agnostic**: Ramps a plain float — caller decides the meaning (volts, Hz, amps, etc.)
- **Zero Allocation**: Caller owns the `rampg_t` struct (stack, static, or embedded)
- **Deterministic**: Time-based update with caller-supplied `dt` — no internal timers or OS dependencies
- **Compile-Time Configuration**: Default limits and rate overridable via `rampg_conf.h`

## Using the Library

### As a Meson subproject

```meson
rampg_dep = dependency('rampg', fallback: ['rampg', 'rampg_dep'])
```

The template also exports `meson.override_dependency('rampg', ...)`
so downstream Meson builds can resolve the subproject dependency by name.

For subproject builds, include the public header directly:

```c
#include "rampg.h"
```

### As an installed dependency

If the library is installed system-wide, include the namespaced header path:

```c
#include <rampg/rampg.h>
```

If `pkg-config` files are installed in your environment, downstream builds can
also discover the package as `rampg`.

The generated version header is available as `rampg_version.h` in the
build tree and as `<rampg/rampg_version.h>` after install.

## Building

```sh
# Library only (release)
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# With unit tests
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

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

## API Reference

### Lifecycle

```c
void  rampg_init(rampg_t *ramp, float initial);
void  rampg_reset(rampg_t *ramp, float value);
```

### Configuration

```c
void  rampg_set_target(rampg_t *ramp, float target);
void  rampg_set_rate(rampg_t *ramp, float rate);
void  rampg_set_rates(rampg_t *ramp, float rise_rate, float fall_rate);
void  rampg_set_limits(rampg_t *ramp, float min, float max);
```

### Runtime

```c
float rampg_update(rampg_t *ramp, float dt);
float rampg_get(const rampg_t *ramp);
bool  rampg_at_target(const rampg_t *ramp);
```

For detailed documentation, see the Doxygen comments in `include/rampg.h`.

## Use Cases

- **DC Bus Precharge**: Slow voltage ramp-up with fast trip-down on fault
- **Output Voltage/Frequency Setpoints**: Smooth transitions between operating points
- **Bounded Rapid Shutdown**: Two-stage ramp-then-trip sequences
- **Motor Speed Control**: Controlled acceleration/deceleration profiles

## Notes

| Topic | Note |
|-------|------|
| **Memory Layout** | `rampg_t` is a plain struct with no pointers — safe to memcpy, embed, or place in shared memory |
| **Thread Safety** | Not thread-safe — caller must synchronise if shared across threads |
| **Error Handling** | No runtime validation — preconditions are documented via `@pre` annotations |
| **Floating Point** | All values are `float` (single precision), suitable for embedded targets |
| **Time Source** | Caller supplies `dt` in seconds — library has no dependency on clocks or OS |
| **Configuration** | Override `RAMPG_DEFAULT_RATE`, `RAMPG_LIMIT_MIN`, `RAMPG_LIMIT_MAX` before including `rampg.h` |
