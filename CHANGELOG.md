# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.2.0]

### Added
- S-curve ramp profile via `rampg_set_shape(ramp, RAMPG_SHAPE_SCURVE)`. The profile bounds the output rate by the configured rate and the change in that rate by a new acceleration limit, so the output eases in and out of a move. Unlike a fixed-shape curve it accepts a non-zero starting rate, so a target, rate, limit, or shape change mid-move is carried at the current rate instead of restarting the move. A move long enough to reach the rate limit takes `rate / accel` seconds longer than the equivalent linear move.
- `rampg_set_accel()`, `rampg_set_accels()` and the `RAMPG_DEFAULT_ACCEL` option, setting the acceleration limit in units per second squared. The limit is selected by the direction of the move, exactly as the rate is, so a slow controlled rise can be paired with a fast trip down. A single symmetric limit cannot serve both: on a 400 V bus at 50 V/s up and 2000 V/s down, one gentle enough to ease the rise makes the trip take 4 s, and one fast enough to trip makes the rise step rather than ease. Both are ignored by the linear profile.
- `rampg_get_rate()`: the signed output rate in units per second. For LINEAR this is the configured rise or fall rate while moving and zero at rest; for SCURVE it is the rate the last update applied. It stays valid across a configuration change, so it can be used directly as a feedforward term.
- `rampg_get_state()` and the `rampg_state_t` enum (`RAMPG_STATE_MOVING`, `RAMPG_STATE_AT_TARGET`, `RAMPG_STATE_DISABLED`).
- `rampg_set_enabled()` and `rampg_is_enabled()` for enable/disable. A disabled ramp holds its output: `rampg_update` leaves the value unchanged, `rampg_get_rate` reads zero, and `rampg_get_state` reports `RAMPG_STATE_DISABLED`. Disabling clears the stored rate, so re-enabling eases away from rest. `rampg_at_target` stays purely positional.
- `rampg_update()` is now total. It holds the output unchanged, rather than corrupting the ramp state, when the time step is not finite and positive, when the effective target is not finite, or when the governing rate or acceleration is not positive. This closes a class of failure in which a single non-finite input left the output permanently NaN.
- MISRA C:2023 static analysis via misch, with x86_64/aarch64 platform profiles, project-level deviations for rules 15.5 and 8.7, a suppression for a Rule 7.3 checker false positive, and baseline scaffolding. Run locally; the rule texts are licensed and cannot be distributed with the project.
- `docs/plot_scurve.py`, which regenerates the documentation figures by compiling and driving the library, so the graphs cannot drift from the implementation.
- Security policy, SPDX headers, cleaned ignore rules, and a coverage gate now set at 100% line and branch.

### Changed
- `rampg_t` gains `rise_accel`, `fall_accel` and `vel`, and no longer carries per-move planner state. `sizeof(rampg_t)` is 44 bytes on x86_64. Only `vel` is internal; every other field is caller-facing configuration.
- `rampg_set_limits()` now resets the output rate when the new limits displace the current value, because a displaced output has not been ramped to where it is.
- `rampg_reset()` now clears the output rate.

### Fixed
- Terminate if/else-if chain in `rampg_update` to satisfy MISRA C:2012 Rule 15.7.

## [0.1.0] - 2026-04-06

### Added

- First public release with a unit-agnostic linear ramp generator, asymmetric rise/fall rates, clamping, configurable defaults, Meson packaging, unit tests, CI, Doxygen annotations, and Sphinx documentation scaffolding.
