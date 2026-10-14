# Contributing to rampg

`rampg` is a small C11 ramp generator for embedded control loops. Firmware drives actuators from its output, so two contracts carry most of the design weight: the output never overshoots its target or leaves its limits, and the output rate stays continuous when any parameter changes in the middle of a move. Keep both in mind when you change anything.

## Getting started

Install a C11 compiler and install the Python tools with [uv](https://docs.astral.sh/uv/), as CI does. The MISRA analysis also needs a current `cppcheck`; CI installs it from Homebrew because the 2.13 release in Ubuntu 24.04 reports different findings. The same commands CI runs, locally:

```sh
# Python tools
uv tool install meson
uv tool install ninja
uv tool install gcovr
uv tool install misch

# Configure with tests and sanitisers (CI default)
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose

# Release build (the configuration a downstream consumer uses)
meson setup build_rel --buildtype=release -Dbuild_tests=false
meson compile -C build_rel

# Coverage (CI gate is 100% line and branch for src/ and include/)
meson setup build_cov --buildtype=debug -Dbuild_tests=true -Db_coverage=true
meson compile -C build_cov && meson test -C build_cov
gcovr --root . --filter 'src/' --filter 'include/' --merge-mode-functions=separate --print-summary

# MISRA C:2023 against the baselines (reads build/compile_commands.json)
env -u MISRA_RULE_TEXTS misch run --baseline
env -u MISRA_RULE_TEXTS misch run --baseline --profile aarch64

# Formatting (pinned clang-format through uvx)
scripts/format.sh --fix
```

## Source style

- Formatting is mandatory. Run `scripts/format.sh --fix` before submitting, or `scripts/format.sh` to check without writing, as CI does. The script pins the clang-format release and runs it through `uvx`, so an editor formatter gives the same result only when it runs the same release.
- 8-space indent, Linux brace style, 80-column limit. Match the existing conventions and do not reformat unrelated code.
- The Meson build system is the single source of truth. Update `meson.build` and `tests/meson.build` when you add or remove files. No CMake, no Make, and no other build systems.

### Section banners

Every `.h` file contains these section banners, in order:

<!-- snippet: skip -->
```c
/* ================ INCLUDES ================================================ */
/* ================ DEFINES ================================================= */
/* ================ STRUCTURES ============================================== */
/* ================ TYPEDEFS ================================================ */
/* ================ MACROS ================================================== */
/* ================ GLOBAL VARIABLES ======================================== */
/* ================ GLOBAL PROTOTYPES ======================================= */
```

Every `.c` file contains these section banners, in order:

<!-- snippet: skip -->
```c
/* ================ INCLUDES ================================================ */
/* ================ DEFINES ================================================= */
/* ================ STRUCTURES ============================================== */
/* ================ TYPEDEFS ================================================ */
/* ================ STATIC PROTOTYPES ======================================= */
/* ================ STATIC VARIABLES ======================================== */
/* ================ MACROS ================================================== */
/* ================ STATIC FUNCTIONS ======================================== */
/* ================ GLOBAL FUNCTIONS ======================================== */
```

Empty sections stay: an empty `STATIC VARIABLES` banner records the deliberate absence of file-scope mutable state. Do not invent other section styles.

## C language rules

- C11 only.
- The public `rampg_t` uses plain `float` by design: the library is unit-agnostic and single precision is the natural type for embedded control loops. Use fixed-width types from `<stdint.h>` where layout or serialisation matters, and never `uint8_t` or `int8_t`, which C11 does not provide where `CHAR_BIT` is 16.
- No heap allocation (`malloc`, `free`, VLAs), no recursion, and no global state. The caller owns every `rampg_t`.
- Public functions return `void`, the output value, a rate, a state, or `bool`. There is no `errno` and no exceptions.
- Preconditions are documented with `@pre` on every public function and are not checked at run time. `rampg_update` is the exception: it must hold the output, rather than corrupt the state, for any `dt`, target, rate or acceleration.

## Configuration

- Compile-time defaults live in `include/rampg_conf.h` behind `#ifndef` guards. `rampg.h` includes it, and any default can be overridden by defining it first.
- Do not add a runtime setting to the public API for something a compile-time default expresses. Keep the API surface small.

## MISRA C:2023

The library is analysed with `misch` (cppcheck-backed MISRA C:2023 analysis), configured by `misra.toml`, over `include/` and `src/`, for an x86_64 and an AArch64 profile. It reads the compilation database from a Meson `build/` directory, so configure one first. The baselines in `analysis/baseline/` record the existing findings, and CI fails on any finding that is not in them.

The baselines are recorded without rule texts, and the texts change the messages a finding is matched on. If you have `MISRA_RULE_TEXTS` set, unset it for `misch run --baseline` as well, as CI does.

A change must not add findings. If it removes one, regenerate both baselines with `MISRA_RULE_TEXTS` unset:

```sh
env -u MISRA_RULE_TEXTS misch baseline
env -u MISRA_RULE_TEXTS misch baseline --profile aarch64
```

Full tool-driven compliance beyond this analysis needs a certified static analyser, which this repository does not vendor.

## Tests and coverage

- Add a test for every bug fix and every new feature.
- Tests must be deterministic. Do not depend on wall-clock time, thread scheduling, or uninitialised memory.
- Check new profile behaviour against the invariants on every update, as the property sweeps in `tests/test_rampg.c` do: the rate within its limit, the change in rate within the acceleration bound, and the output inside the limits.
- Tests live in `tests/test_*.c`, each its own executable registered with `test()` in `tests/meson.build`.
- CI enforces 100 percent line and branch coverage of `src/` and `include/`. Every new branch needs a test that fails when the branch is broken.
- The `rampg docs examples` test compiles every C code block in `README.md`, `CONTRIBUTING.md`, and `docs/` against the public headers, through `tests/docs/check_snippets.py`. A block compiles at file scope by default. On the line directly above the fence, put `<!-- snippet: body -->` for a block of statements, or `<!-- snippet: skip -->` for pseudo-code or a copy of a header definition.

## Documentation

| File | Purpose |
| --- | --- |
| `README.md` | Landing page: quick start, description, validated toolchains, and links. Keep it short. |
| `docs/integration.md` | Installation, configuration, building, and cross builds. |
| `docs/api.md` | Reference for every function and type, with recipes. |
| `docs/design.md` | The linear and S-curve profiles, their guarantees, numerical range, cost, and verification. |

Update `docs/api.md` and the header Doxygen together when an interface changes, and check every value an example states against a build. Update `docs/design.md` when behaviour changes, and regenerate its graphs with `uv run docs/plot_scurve.py` when the profiles change. Keep it standalone: describe how the library works, not how it was built or which other projects use it. Write prose in British English, one line per paragraph.

## API stability

The public API in `include/rampg.h` and the macros in `include/rampg_conf.h` are the project's contract:

- Changing a function signature, the layout of `rampg_t`, or the meaning of a `rampg_conf.h` macro is a breaking change and needs a major release.
- Renaming or removing a public symbol is a breaking change.
- Adding a function, or a configuration macro with a backwards-compatible default, is a minor release.

## Commits

Use Conventional Commits:

- `feat: ...` new feature
- `fix: ...` bug fix
- `docs: ...` documentation only
- `test: ...` test-only changes
- `chore: ...` build, CI, release work
- `refactor: ...` a change that neither fixes a bug nor adds a feature

Keep the subject under about 70 characters. Use the body to explain why the change is needed.

## Pull requests

- Open an issue first for non-trivial changes so the design can be agreed before implementation.
- Keep pull requests focused: one feature or one fix.
- All CI checks must pass.
- Add a `CHANGELOG.md` entry under `## [Unreleased]` for a user-visible change.
- Flag any change to a profile's timing or output in the description.

## When in doubt

Open an issue. A change to the output shape changes how every actuator driven by it moves.
