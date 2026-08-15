# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.2.0]

### Added
- S-curve (sigmoid) ramp profile via `rampg_set_shape` and the `RAMPG_DEFAULT_SHAPE` option: a quintic curve sized so its peak rate equals the configured rate, with zero velocity at the start and end. The move is re-planned from the current value whenever the target, limits, rates, or shape change, and a setter call that leaves the configuration unchanged does not restart the move. Adds 17 new unit tests for the profile.
- `rampg_get_rate()`: effective rate in units per second (signed). For LINEAR this is the configured rise or fall rate while moving, and zero when at rest; for SIGMOID this is the instantaneous slope of the active S-curve, peaking at the configured rate mid-move.
- `rampg_get_state()` and the `rampg_state_t` enum (`RAMPG_STATE_MOVING`, `RAMPG_STATE_AT_TARGET`).
- `rampg_set_enabled()` and `rampg_is_enabled()` for enable/disable. A disabled ramp holds its output: `rampg_update` leaves the value unchanged and `rampg_get_rate` reads zero, while `rampg_at_target` and `rampg_get_state` keep reporting the underlying movement. The ramp is enabled by default after `rampg_init`, and re-enabling resumes the move from the current value.
- Seven new fields appended at the end of `rampg_t`, including the new `enabled` flag. Existing field offsets are unchanged, but `sizeof(rampg_t)` grows from 24 to 48 bytes.
- MISRA C:2023 static analysis via misch, with x86_64/aarch64 platform profiles, project-level deviations for rules 15.5 and 8.7, and baseline scaffolding.
- Security policy, SPDX headers, cleaned ignore rules, and a standard coverage gate.

### Fixed
- Terminate if/else-if chain in `rampg_update` to satisfy MISRA C:2012 Rule 15.7.

## [0.1.0] - 2026-04-06

### Added

- First public release with a unit-agnostic linear ramp generator, asymmetric rise/fall rates, clamping, configurable defaults, Meson packaging, unit tests, CI, Doxygen annotations, and Sphinx documentation scaffolding.
