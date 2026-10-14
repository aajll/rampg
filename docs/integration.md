# Integration guide

This guide shows how to add `rampg` to a project, configure it, build it, and run its tests. The API is described in [api.md](api.md), and the profiles in [design.md](design.md).

## Requirements

- A C11 compiler with a conformant `<stdbool.h>`; the public API uses `bool`.
- IEEE 754 binary32 `float`.
- A `<math.h>` providing `sqrtf`, `isfinite` and `nextafterf`. Only the implementation uses it, never the public header. Some hosted toolchains need an explicit `-lm`, which the Meson build adds where required.
- For the tests: Meson, Ninja, and Python 3.

The library has no chip-specific code paths, so any target that meets these requirements is expected to work.

## Copy-in

Copy-in is the recommended method for embedded targets. Copy three files into your source tree:

```
include/rampg.h
include/rampg_conf.h
src/rampg.c
```

Keep both headers on the same include path, because `rampg.h` includes `rampg_conf.h`. Then include the public header:

```c
#include "rampg.h"
```

`rampg_version.h` exists only in Meson builds.

## Meson subproject

Put the repository in `subprojects/rampg`, as a checkout or through a wrap file, then declare the dependency:

```meson
rampg_dep = dependency('rampg', fallback: ['rampg', 'rampg_dep'])
```

The build calls `meson.override_dependency('rampg', ...)`, so other subprojects in the same build find `rampg` by name. Include the header as `"rampg.h"`. The dependency carries `libm` where the toolchain needs it. Tests are off unless the `build_tests` option is set.

| Option | Default | Effect |
| --- | --- | --- |
| `build_tests` | `false` | Builds the unit tests and the documentation example test. |

## Installed package

```sh
meson setup build --buildtype=release -Dbuild_tests=false --prefix=/usr/local
meson compile -C build
meson install -C build
cc app.c $(pkg-config --cflags --libs rampg) -o app
```

The headers install to a `rampg/` subdirectory, and the `pkg-config` flags add that directory to the include path, so the same include works as in a copy-in build. Without `pkg-config`, include the namespaced path instead:

<!-- snippet: skip -->
```c
#include <rampg/rampg.h>
#include <rampg/rampg_version.h>
```

`rampg_version.h` defines `RAMPG_VERSION` and `RAMPG_VERSION_STRING` as the version string, and `RAMPG_VERSION_MAJOR`, `RAMPG_VERSION_MINOR` and `RAMPG_VERSION_PATCH` as integers.

## Configuration

The defaults that `rampg_init` applies are macros in `rampg_conf.h`. Define them with `-D`, or before the first include of `rampg.h`, identically for the library source and every file that includes the header.

| Macro | Default | Effect |
| --- | --- | --- |
| `RAMPG_DEFAULT_RATE` | `100.0f` | Rise and fall rate, in units per second. |
| `RAMPG_DEFAULT_ACCEL` | `RAMPG_DEFAULT_RATE * 10.0f` | Rise and fall acceleration limit, in units per second squared. The default reaches the default rate in 0.1 s. |
| `RAMPG_LIMIT_MIN` | `-1000000.0f` | Output clamp minimum. |
| `RAMPG_LIMIT_MAX` | `1000000.0f` | Output clamp maximum. |
| `RAMPG_DEFAULT_SHAPE` | `RAMPG_SHAPE_LINEAR` | Ramp profile, `RAMPG_SHAPE_LINEAR` or `RAMPG_SHAPE_SCURVE`. |

A Meson build takes the macros through `c_args`:

```sh
meson setup build "-Dc_args=-DRAMPG_DEFAULT_SHAPE=RAMPG_SHAPE_SCURVE -DRAMPG_DEFAULT_ACCEL=500.0f"
```

## Build and test

```sh
# Library only
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# Unit tests and the documentation test under sanitisers
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose
```

Coverage, formatting, and MISRA commands are in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Cross builds

The consuming firmware owns its cross file, linker command file, and memory placement. No cross file ships with this project.

CI compiles the library with warnings as errors using two releases of a vendor C11 compiler for a 16-bit-MAU DSP target, where `CHAR_BIT` is 16, under both of its object formats and with its hardware single-precision floating point enabled. This covers the source dialect and type assumptions only. Linking, execution, and the final map-file footprint remain checks for the consumer.

On a target without hardware floating point, every `float` operation calls a software routine. The S-curve profile's square root per update then dominates its cost; see [design.md](design.md#cost-per-update).
