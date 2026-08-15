# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.2.0]

### Added
- S-curve (sigmoid) ramp profile via `rampg_set_shape` and the `RAMPG_DEFAULT_SHAPE` option, a quintic curve sized so its peak rate equals the configured rate, plus 15 new unit tests.
- `rampg_get_rate()`: effective rate in units per second (signed). For LINEAR this is the configured rise or fall rate while moving, and zero when at rest; for SIGMOID this is the instantaneous slope of the active S-curve, peaking at the configured rate mid-move.
- `rampg_get_state()` and the `rampg_state_t` enum (`RAMPG_STATE_MOVING`, `RAMPG_STATE_AT_TARGET`).
- MISRA C:2023 static analysis via misch, with x86_64/aarch64 platform profiles, project-level deviations for rules 15.5 and 8.7, and baseline scaffolding.
- Security policy, SPDX headers, cleaned ignore rules, and a standard coverage gate.

### Changed
- Sigmoid profile re-plans on target, rate, limit, or shape change, so an unchanged setter call no longer restarts the move (this is an internal fix to the profile, not a public API break).

## [0.1.0] - 2026-04-06

### Added

- First public release with a unit-agnostic linear ramp generator, asymmetric rise/fall rates, clamping, configurable defaults, Meson packaging, unit tests, CI, Doxygen annotations, and Sphinx documentation scaffolding.

### Fixed

- Terminate if/else-if chain in `rampg_update` to satisfy MISRA C:2012 Rule 15.7.
