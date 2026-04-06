rampg
=====

A lightweight, unit-agnostic linear ramp generator with asymmetric rise/fall
rates and output clamping, designed for deterministic embedded control loops
in C11.

Features
--------

- **Linear ramping** — smoothly transitions a float value toward a target
- **Asymmetric rates** — independent rise and fall rates
- **Output clamping** — configurable min/max limits enforced on every update
- **Unit-agnostic** — caller decides the meaning (volts, Hz, amps, etc.)
- **Zero allocation** — caller owns the ``rampg_t`` struct
- **Deterministic** — time-based ``dt`` update, no internal timers

Quick Start
-----------

.. code-block:: c

   #include "rampg.h"

   /* Ramp a DC bus voltage from 0 to 400 V at 50 V/s */
   rampg_t vbus;
   rampg_init(&vbus, 0.0f);
   rampg_set_rate(&vbus, 50.0f);
   rampg_set_limits(&vbus, 0.0f, 800.0f);
   rampg_set_target(&vbus, 400.0f);

   /* Called from a 1 kHz control loop */
   while (!rampg_at_target(&vbus)) {
           float v = rampg_update(&vbus, 0.001f);
           /* apply v to hardware */
   }

Building
--------

.. code-block:: sh

   # Library only
   meson setup build --buildtype=release -Dbuild_tests=false
   meson compile -C build

   # With tests
   meson setup build --buildtype=debug -Dbuild_tests=true
   meson compile -C build
   meson test -C build --verbose

Contents
--------

.. toctree::
   :maxdepth: 2

   api/modules
