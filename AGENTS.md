# AGENTS.md

## Project-specific instructions

**Project:** `rampg`
**Primary goal:** A lightweight, unit-agnostic linear ramp generator with asymmetric rise/fall rates and output clamping, designed for deterministic embedded control loops in C11.

### Essential commands

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

### CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs.
- If `pre-commit` is configured later, run it before committing.

## Docs / commit conventions

- Use Conventional Commits when asked to commit.
- Keep commits focused and explain why the change exists.

## C code style (MANDATORY)

### Build and configuration

- Use the Meson build system; do not introduce another build system.
- Update `meson.build` when adding or removing source files.

### Formatting

- `.clang-format` is present and should be used on modified `.c` and `.h` files.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Section headers

Every `.h` file must contain these section headers in order:

```
/* ================ INCLUDES ================================================ */
/* ================ DEFINES ================================================= */
/* ================ STRUCTURES ============================================== */
/* ================ TYPEDEFS ================================================ */
/* ================ MACROS ================================================== */
/* ================ GLOBAL VARIABLES ======================================== */
/* ================ GLOBAL PROTOTYPES ======================================= */
```

Every `.c` file must contain these section headers in order:

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

Do NOT invent alternative section styles (e.g., `/* ── Section ── */`). Use the exact banner format shown above. Sections may be empty — that is intentional (communicates absence).

### Style and correctness

- Match the conventions in the existing files.
- Keep public headers minimal and stable.
- Prefer explicit fixed-width integer types when ABI or serialization matters.
- Use `rampg_conf.h` for compile-time configuration options. This header is automatically included by `rampg.h` and can be overridden before including the main header.
- Document preconditions with `@pre` annotations in doxygen comments.

### Testing

- Run `meson test -C build` after changes.
- Add a test case for each bug fix.
- Keep tests in `tests/test_*.c`.
