# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Changed
- Added security policy, SPDX headers, cleaned ignore rules, and a standard coverage gate.
### Added
- MISRA C:2023 static analysis via misch, with x86_64/aarch64 platform profiles, project-level deviations for rules 15.5 and 8.7, and baseline scaffolding.
- S-curve (sigmoid) ramp profile via `rampg_set_shape` and the `RAMPG_DEFAULT_SHAPE` option, a quintic curve sized so its peak rate equals the configured rate, plus 12 new unit tests.
### Fixed
- Terminate if/else-if chain in `rampg_update` to satisfy MISRA C:2012 Rule 15.7.

## [1.1.0] - 2026-04-06

### Added

- First public release with a unit-agnostic linear ramp generator, asymmetric rise/fall rates, clamping, configurable defaults, Meson packaging, unit tests, CI, Doxygen annotations, and Sphinx documentation scaffolding.
