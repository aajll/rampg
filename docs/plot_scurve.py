#!/usr/bin/env python3
"""Regenerate the S-curve figures in docs/images.

The figures are produced by driving the real library rather than by
re-deriving the profile in Python, so they cannot drift from the
implementation. The C source is compiled into a temporary shared object and
called through ctypes.

Usage:
    python docs/plot_scurve.py

Requires matplotlib and a C compiler.
"""

from __future__ import annotations

import ctypes
import pathlib
import shutil
import subprocess
import sys
import tempfile

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
IMAGES = ROOT / "docs" / "images"

SHAPE_LINEAR = 0
SHAPE_SCURVE = 1

VIOLET = "#7b4fc9"
BLUE = "#1f77b4"
ORANGE = "#e08a1e"
GREY = "#8a8a8a"


class Ramp(ctypes.Structure):
    """Mirror of rampg_t. Field order must match include/rampg.h."""

    _fields_ = [
        ("value", ctypes.c_float),
        ("target", ctypes.c_float),
        ("rise_rate", ctypes.c_float),
        ("fall_rate", ctypes.c_float),
        ("accel", ctypes.c_float),
        ("limit_min", ctypes.c_float),
        ("limit_max", ctypes.c_float),
        ("vel", ctypes.c_float),
        ("shape", ctypes.c_int),
        ("enabled", ctypes.c_bool),
    ]


def build_library(workdir: pathlib.Path) -> ctypes.CDLL:
    """Compile src/rampg.c into a shared object and load it."""
    cc = shutil.which("cc") or shutil.which("gcc")
    if cc is None:
        sys.exit("no C compiler found on PATH")

    # rampg.h includes the generated version header only via the build; the
    # library source itself needs just the public headers.
    so = workdir / "librampg_plot.so"
    subprocess.run(
        [
            cc,
            "-std=c11",
            "-O2",
            "-fPIC",
            "-shared",
            "-I",
            str(ROOT / "include"),
            str(ROOT / "src" / "rampg.c"),
            "-lm",
            "-o",
            str(so),
        ],
        check=True,
    )

    lib = ctypes.CDLL(str(so))
    lib.rampg_init.argtypes = [ctypes.POINTER(Ramp), ctypes.c_float]
    lib.rampg_update.argtypes = [ctypes.POINTER(Ramp), ctypes.c_float]
    lib.rampg_update.restype = ctypes.c_float
    lib.rampg_get_rate.argtypes = [ctypes.POINTER(Ramp)]
    lib.rampg_get_rate.restype = ctypes.c_float
    lib.rampg_set_target.argtypes = [ctypes.POINTER(Ramp), ctypes.c_float]
    lib.rampg_set_rate.argtypes = [ctypes.POINTER(Ramp), ctypes.c_float]
    lib.rampg_set_rates.argtypes = [
        ctypes.POINTER(Ramp),
        ctypes.c_float,
        ctypes.c_float,
    ]
    lib.rampg_set_accel.argtypes = [ctypes.POINTER(Ramp), ctypes.c_float]
    lib.rampg_set_limits.argtypes = [
        ctypes.POINTER(Ramp),
        ctypes.c_float,
        ctypes.c_float,
    ]
    lib.rampg_set_shape.argtypes = [ctypes.POINTER(Ramp), ctypes.c_int]
    return lib


def make_ramp(lib, shape, rate, accel, limits=(-1000.0, 1000.0), initial=0.0):
    ramp = Ramp()
    lib.rampg_init(ctypes.byref(ramp), initial)
    lib.rampg_set_shape(ctypes.byref(ramp), shape)
    lib.rampg_set_rate(ctypes.byref(ramp), rate)
    lib.rampg_set_accel(ctypes.byref(ramp), accel)
    lib.rampg_set_limits(ctypes.byref(ramp), limits[0], limits[1])
    return ramp


def run(lib, ramp, dt, steps, schedule=None):
    """Drive `ramp` for `steps` updates, sampling value, rate and acceleration.

    `schedule` is an optional callable (tick) -> target or None.
    """
    ts, values, rates, accels = [], [], [], []
    previous_rate = lib.rampg_get_rate(ctypes.byref(ramp))
    for tick in range(steps):
        if schedule is not None:
            target = schedule(tick)
            if target is not None:
                lib.rampg_set_target(ctypes.byref(ramp), target)
        lib.rampg_update(ctypes.byref(ramp), dt)
        rate = lib.rampg_get_rate(ctypes.byref(ramp))
        ts.append(tick * dt)
        values.append(ramp.value)
        rates.append(rate)
        accels.append((rate - previous_rate) / dt)
        previous_rate = rate
    return ts, values, rates, accels


def style_axis(ax, ylabel, xlabel=None):
    ax.set_ylabel(ylabel)
    if xlabel:
        ax.set_xlabel(xlabel)
    ax.grid(True, alpha=0.25, linewidth=0.6)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def figure_profiles(lib):
    """Value, rate and acceleration for a linear and an S-curve move."""
    dt = 0.002
    rate, accel = 100.0, 200.0
    steps = 900

    lin = make_ramp(lib, SHAPE_LINEAR, rate, accel)
    lib.rampg_set_target(ctypes.byref(lin), 100.0)
    lt, lv, lr, la = run(lib, lin, dt, steps)

    sc = make_ramp(lib, SHAPE_SCURVE, rate, accel)
    lib.rampg_set_target(ctypes.byref(sc), 100.0)
    st, sv, sr, sa = run(lib, sc, dt, steps)

    fig, axes = plt.subplots(3, 1, figsize=(8.5, 8.0), sharex=True)

    axes[0].plot(lt, lv, color=BLUE, lw=1.8, label="linear")
    axes[0].plot(st, sv, color=VIOLET, lw=1.8, label="S-curve")
    style_axis(axes[0], "output value")
    axes[0].legend(frameon=False, loc="lower right")
    axes[0].set_title(
        "0 to 100 at a 100 unit/s rate limit and a 200 unit/s² "
        "acceleration limit",
        fontsize=10,
    )

    axes[1].plot(lt, lr, color=BLUE, lw=1.8)
    axes[1].plot(st, sr, color=VIOLET, lw=1.8)
    axes[1].axhline(rate, color=GREY, ls="--", lw=1.0)
    axes[1].annotate(
        "rate limit",
        xy=(lt[-1], rate),
        xytext=(-70, 6),
        textcoords="offset points",
        color=GREY,
        fontsize=8,
    )
    style_axis(axes[1], "rate (units/s)")

    axes[2].plot(lt, la, color=BLUE, lw=1.4)
    axes[2].plot(st, sa, color=VIOLET, lw=1.8)
    for sign in (1, -1):
        axes[2].axhline(sign * accel, color=GREY, ls="--", lw=1.0)
    axes[2].set_ylim(-3.0 * accel, 3.0 * accel)
    axes[2].annotate(
        "acceleration limit",
        xy=(st[-1], accel),
        xytext=(-100, 8),
        textcoords="offset points",
        color=GREY,
        fontsize=8,
    )
    axes[2].annotate(
        "linear steps off-scale at both ends",
        xy=(0.02, -2.4 * accel),
        color=BLUE,
        fontsize=8,
    )
    axes[2].annotate(
        "arrival step, bounded at about twice one acceleration step",
        xy=(st[-1], -2.0 * accel),
        xytext=(-330, -12),
        textcoords="offset points",
        color=VIOLET,
        fontsize=8,
    )
    style_axis(axes[2], "acceleration (units/s²)", "time (s)")

    fig.tight_layout()
    fig.savefig(IMAGES / "scurve_profiles_and_rates.png", dpi=140)
    plt.close(fig)


def figure_online(lib):
    """Retarget, reversal and asymmetric rates: the online behaviour."""
    dt = 0.002
    rate, accel = 100.0, 200.0

    # (A) retarget mid-move, then reverse past the start.
    sc = make_ramp(lib, SHAPE_SCURVE, rate, accel)

    def schedule(tick):
        if tick == 0:
            return 100.0
        if tick == 250:
            return 160.0
        if tick == 600:
            return -40.0
        return None

    at, av, ar, _ = run(lib, sc, dt, 1900, schedule)

    # (B) asymmetric rise and fall.
    asym = make_ramp(lib, SHAPE_SCURVE, rate, accel)
    lib.rampg_set_rates(ctypes.byref(asym), 50.0, 150.0)

    def schedule_b(tick):
        if tick == 0:
            return 200.0
        if tick == 2600:
            return 0.0
        return None

    bt, bv, br, _ = run(lib, asym, dt, 4700, schedule_b)

    fig, axes = plt.subplots(2, 2, figsize=(11.0, 6.6))

    axes[0][0].plot(at, av, color=VIOLET, lw=1.8)
    for tick, label in ((250, "retarget"), (600, "reverse")):
        axes[0][0].axvline(tick * dt, color=GREY, ls=":", lw=1.0)
        axes[0][0].annotate(
            label,
            xy=(tick * dt, max(av)),
            xytext=(4, -10),
            textcoords="offset points",
            color=GREY,
            fontsize=8,
        )
    style_axis(axes[0][0], "output value")
    axes[0][0].set_title("(A) retarget and reversal mid-move", fontsize=10)

    axes[1][0].plot(at, ar, color=VIOLET, lw=1.8)
    for tick in (250, 600):
        axes[1][0].axvline(tick * dt, color=GREY, ls=":", lw=1.0)
    axes[1][0].axhline(0.0, color=GREY, lw=0.8)
    style_axis(axes[1][0], "rate (units/s)", "time (s)")
    axes[1][0].annotate(
        "rate is carried across both changes",
        xy=(0.05, 0.06),
        xycoords="axes fraction",
        fontsize=8,
        color=VIOLET,
    )

    axes[0][1].plot(bt, bv, color=ORANGE, lw=1.8)
    style_axis(axes[0][1], "output value")
    axes[0][1].set_title(
        "(B) asymmetric rates: 50 unit/s up, 150 unit/s down", fontsize=10
    )

    axes[1][1].plot(bt, br, color=ORANGE, lw=1.8)
    axes[1][1].axhline(50.0, color=GREY, ls="--", lw=1.0)
    axes[1][1].axhline(-150.0, color=GREY, ls="--", lw=1.0)
    axes[1][1].axhline(0.0, color=GREY, lw=0.8)
    style_axis(axes[1][1], "rate (units/s)", "time (s)")

    fig.tight_layout()
    fig.savefig(IMAGES / "scurve_retarget_and_asymmetric.png", dpi=140)
    plt.close(fig)


def main() -> int:
    IMAGES.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        lib = build_library(pathlib.Path(tmp))
        figure_profiles(lib)
        figure_online(lib)
    print(f"wrote figures to {IMAGES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
