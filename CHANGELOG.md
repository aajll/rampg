# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.1.0] - 2026-04-06

First public release.

### Added

- Public API in `include/rampg.h` for a unit-agnostic linear ramp
  generator targeting deterministic embedded control loops:
  - Lifecycle: `rampg_init`, `rampg_reset`.
  - Configuration: `rampg_set_target`, `rampg_set_rate`,
    `rampg_set_rates`, `rampg_set_limits`.
  - Runtime: `rampg_update`, `rampg_get`, `rampg_at_target`.
- Asymmetric rise and fall rates with independent units-per-second
  configuration via `rampg_set_rates`.
- Output clamping to caller-supplied minimum and maximum limits, with
  the stored target preserved so widening limits later recovers the
  original intent.
- Compile-time configuration header `include/rampg_conf.h` with
  overridable defaults `RAMPG_DEFAULT_RATE`, `RAMPG_LIMIT_MIN`, and
  `RAMPG_LIMIT_MAX`. The configuration header is auto-included by
  `rampg.h`.
- Meson build system with `static_library`, public header install,
  `pkg-config` generation, and `meson.override_dependency('rampg', ...)`
  for downstream subproject consumption.
- Auto-generated `rampg_version.h` produced from `config/rampg_version.h.in`
  and installed under `<includedir>/rampg/`.
- Unit test suite in `tests/test_rampg.c` covering ramp up, ramp down,
  asymmetric rates, target snapping, clamping, and overshoot avoidance.
- CI workflow in `.github/workflows/ci.yml` running tests on Linux and
  macOS under AddressSanitizer (plus UndefinedBehaviorSanitizer on
  Linux) and a separate release build job.
- Doxygen-style documentation on every public function with explicit
  `@pre` precondition annotations.
- Sphinx documentation scaffolding under `docs/` with `ruff` configured
  for documentation tooling.
