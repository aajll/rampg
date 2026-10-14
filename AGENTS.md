# AGENTS.md

---

## 1) Project-specific instructions

**Project:** `rampg`
**Primary goal:** A C11 unit-agnostic ramp generator for embedded control loops, with linear and acceleration-limited S-curve profiles, asymmetric rise and fall, output clamping, caller-owned storage and no allocation.

### 1.1 Essential commands

#### Configure and build (library only)

```sh
meson setup build --wipe --buildtype=release -Dbuild_tests=false
meson compile -C build
```

#### Configure, build, and run unit tests

```sh
meson setup build --wipe --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

#### MISRA C:2023 (reads `build/compile_commands.json`)

```sh
env -u MISRA_RULE_TEXTS misch run --baseline
env -u MISRA_RULE_TEXTS misch run --baseline --profile aarch64
```

#### Formatting (pinned clang-format through `uvx`)

```sh
scripts/format.sh          # check, as CI does
scripts/format.sh --fix    # rewrite in place
```

### 1.2 Notes

- `meson setup` generates the optional `rampg_version.h` into the build directory; `rampg.h` does not include it.
- Compile-time defaults (`RAMPG_DEFAULT_RATE`, `RAMPG_DEFAULT_ACCEL`, `RAMPG_LIMIT_MIN`, `RAMPG_LIMIT_MAX`, `RAMPG_DEFAULT_SHAPE`) live in `rampg_conf.h`. The only Meson option is `build_tests`.
- `docs/plot_scurve.py` regenerates the graphs in `docs/images/` with `uv run docs/plot_scurve.py`.

---

## 2) Non-negotiable design invariants

- **No overshoot.** The output never passes the effective target (the stored target clamped to the limits) and never leaves the limits, and it lands exactly on the target so `rampg_at_target` can compare exactly.
- **Continuous rate.** The output rate is carried across a change of target, rate, acceleration, limits, or a switch from linear to S-curve. Never restart a move from rest unless the output was displaced, reset or disabled.
- **Braking envelope is a clamp.** The S-curve clamps the rate to the braking envelope and never tracks it as a target; tracking it is unstable at the end of a move.
- **Total update.** `rampg_update` holds the output for a non-finite or non-positive `dt`, a non-finite effective target, or a non-positive rate or acceleration for the direction of travel. The setters keep `@pre` contracts without runtime checks.
- **No exact 8-bit types.** `uint8_t` does not exist where `CHAR_BIT` is 16.
- No heap, no recursion, no global state.

---

## 3) CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs (see §1.1 above).
- Install Python tools (Meson, Ninja, gcovr, misch) with `uv tool install`, never `pip`; CI does the same through `astral-sh/setup-uv`. CI installs `cppcheck` from Homebrew because Ubuntu's 2.13 reports different findings.
- Regenerate the MISRA baselines only with `MISRA_RULE_TEXTS` unset (`env -u MISRA_RULE_TEXTS misch baseline`, and again with `--profile aarch64`), because the rule texts change the recorded messages.
- The CI jobs are: tests under ASan/UBSan, a release build, a 100 percent line and branch coverage gate, a TI C2000 library build (CGT 22.6.2 and 25.11.1, COFF and EABI, warnings as errors), `clang-format`, and `misch` against the x86_64 and AArch64 baselines.

---

## 4) Docs / commit conventions

### Documentation layout

- `README.md` is a short landing page (about 70 lines). Put detail in `docs/`, never in the README; link to the owning file instead.
- `docs/integration.md` owns requirements, install methods, configuration, building, and cross builds.
- `docs/api.md` owns the API reference and recipes. Change it together with the headers; keep signatures in fenced `c` blocks and describe them in prose, not function tables. Check every value an example states against a build.
- `docs/design.md` is the single authoritative design record, including the S-curve derivation and graphs.
- `CONTRIBUTING.md` documents the contributor workflow; `CHANGELOG.md` holds release history (Keep a Changelog); `.github/SECURITY.md` covers vulnerability reporting.
- Code blocks in `README.md`, `CONTRIBUTING.md`, and `docs/` are compiled by the `rampg docs examples` test (`tests/docs/check_snippets.py`). Each document is compiled as one source file: a `c` fence is file-level code, `<!-- snippet: body -->` wraps statements, and `<!-- snippet: skip -->` excludes pseudo-code. Do not use `skip` to silence a broken example.
- Prose uses British spelling. Do not hard-wrap Markdown: write one line per paragraph. The 80-column limit applies to C source, not Markdown.

### Commits

- Use **Conventional Commits** format when asked to commit.
- Keep commits focused; explain *why* in the message body.

---

## 5) C style expectations

### Build & configuration

- Use the Meson build system. Do not introduce CMake, Make, or other systems.
- Update `meson.build` and `tests/meson.build` when adding or removing files.
- Compile-time configuration goes in `rampg_conf.h` behind `#ifndef` guards. Do not add a runtime setting for something a compile-time default expresses.

### Formatting

- `.clang-format` is present and **mandatory**. Run `scripts/format.sh --fix` before committing. It runs the clang-format release pinned in the script through `uvx`. Do not use a system `clang-format`: another release can format the same code differently.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Section banners

Every `.h` file contains these banners, in order:

```
/* ================ INCLUDES ================================================ */
/* ================ DEFINES ================================================= */
/* ================ STRUCTURES ============================================== */
/* ================ TYPEDEFS ================================================ */
/* ================ MACROS ================================================== */
/* ================ GLOBAL VARIABLES ======================================== */
/* ================ GLOBAL PROTOTYPES ======================================= */
```

Every `.c` file contains these banners, in order:

```
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

Use the exact banner format. Sections may be empty; an empty banner records a deliberate absence.

### Style & correctness

- Match conventions in the existing files (indentation, braces, naming).
- Keep public headers minimal and stable.
- Document preconditions with `@pre` on every public function.
- The public `rampg_t` uses plain `float` by design; the library is unit-agnostic.

### Error handling

- Public functions return `void`, the output value, a rate, a state, or `bool`. No `errno`; no exceptions.
- The setters do not check preconditions at run time; `rampg_update` contains invalid inputs by holding the output.

### Comment placement (Doxygen)

- Inline trailing annotations (`/**< ... */`) on `enum`/`struct` members are allowed only when the resulting line fits the 80-column limit.
- If any member's annotation would overrun, move **all** of that aggregate's member docs into a single structured Doxygen block above the type, as an `@details` list of `- ::SYMBOL  description` entries.
- Never mix inline and block forms within one aggregate, and never leave a trailing comment that clang-format would wrap onto a second line.
- After editing, run `scripts/format.sh` and an 80-column scan.

### Testing

- Run `meson test -C build` after every change.
- Add a test case for each bug fix and each new feature.
- Tests must be deterministic. Do not depend on wall-clock time or thread scheduling.
- Check profile changes against the per-update invariants in the property sweeps: rate within its limit, change in rate within the acceleration bound, output inside the limits.
- Keep `src/` and `include/` at the coverage gate (100 percent line and branch).
