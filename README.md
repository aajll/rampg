# rampg

[![CI](https://github.com/aajll/rampg/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/rampg/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**A unit-agnostic ramp generator for embedded control loops in C11, with linear and acceleration-limited S-curve profiles, asymmetric rise and fall, and output clamping.**

## Quick start

Copy `include/` and `src/rampg.c` into your project. For a Meson subproject or an installed `pkg-config` package, see the [integration guide](docs/integration.md).

```c
#include "rampg.h"

static rampg_t vbus;

void
vbus_start(void)
{
        /* Ramp a DC bus setpoint from 0 to 400 V at 50 V/s. */
        rampg_init(&vbus, 0.0f);
        rampg_set_rate(&vbus, 50.0f);
        rampg_set_limits(&vbus, 0.0f, 800.0f);
        rampg_set_target(&vbus, 400.0f);
}

/* Called from a 1 kHz control loop; the output reaches 400 V after 8 s. */
float
vbus_tick(void)
{
        return rampg_update(&vbus, 0.001f);
}
```

## Description

`rampg` moves a `float` output toward a target at a bounded rate, one caller-timed step at a time. The caller decides the unit (volts, hertz, amps, RPM) and supplies the time step, so the library has no clock and no OS dependency.

- A linear profile at a constant rate, and an S-curve profile that also bounds how fast the rate changes, so the output eases in and out of a move
- Independent rise and fall rates and accelerations, so a slow controlled rise can be paired with a fast trip down
- The output rate is carried across a target, rate, limit or shape change, so a ramp driven by a live setpoint never restarts or steps
- Output clamping on every update, enable and disable, and live rate and state readout for feedforward and monitoring
- An update that holds the output on a non-finite or non-positive time step, target, rate or acceleration rather than corrupting its state
- Caller-owned storage, no allocation, and bounded constant work per update
- Unit tests under ASan and UBSan with 100% line and branch coverage, and MISRA C:2023 analysis with [`misch`](https://pypi.org/project/misch/)

## Validated toolchains

| Toolchain               | Target                        | Validation                                   |
| ----------------------- | ----------------------------- | -------------------------------------------- |
| GCC                     | x86_64 Linux                  | Unit tests under ASan and UBSan              |
| DSP vendor C11 compiler | 16-bit MAU (`CHAR_BIT == 16`) | Library compile, two object formats          |

Target linking and execution belong to the consuming firmware. See [cross builds](docs/integration.md#cross-builds).

## Documentation

- [Integration guide](docs/integration.md) — copy-in, Meson subproject, installed package, configuration, and cross builds
- [API reference](docs/api.md) — every function, type and configuration macro, with control-loop recipes
- [Design](docs/design.md) — the linear and S-curve profiles, their guarantees, numerical range, and verification
- [Changelog](CHANGELOG.md) — release history

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before you open a pull request. Report security issues as described in [SECURITY.md](.github/SECURITY.md).

## License

MIT. See [LICENSE](LICENSE).
