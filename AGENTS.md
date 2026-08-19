# AGENTS.md

---

## 1) Project-specific instructions

**Project:** `rampg`
**Primary goal:** A lightweight, unit-agnostic ramp generator with linear and acceleration-limited S-curve profiles, asymmetric rise/fall rates and output clamping, designed for deterministic embedded control loops in C11.

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

#### Notes

- `meson setup` generates the auto-included `rampg_version.h` into the **build directory**.

---

## 2) CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs (see §1.1 above).
- If `pre-commit` is configured later, run `pre-commit run --all-files` before committing.

---

## 3) Docs / commit conventions

- Use **Conventional Commits** format when asked to commit.
- Keep commits focused; explain _why_ in the message body.
- User-visible changes must be recorded in `CHANGELOG.md` (Keep a Changelog format).
- Contributor expectations are documented in `CONTRIBUTING.md`; keep it in sync with this file.

---

## 4) C style expectations

### Build & configuration

- Use the Meson build system. Do not introduce CMake, Make, or other systems.
- Update `meson.build` / `tests/meson.build` when adding or removing source files.
- Use `rampg_conf.h` for compile-time configuration options. This header is automatically included by `rampg.h` and any option can be overridden by defining it before the include.

### Formatting

- `.clang-format` is present and **mandatory**. Run `clang-format -i` on all modified `.c` / `.h` files before committing.
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

Do NOT invent alternative section styles (e.g., `/* ── Section ── */`). Use the exact banner format shown above. Sections may be empty; that is intentional and communicates absence.

### Style & correctness

- Match conventions in the existing files (indentation, braces, naming).
- Keep public headers minimal and stable.
- Document preconditions with `@pre` annotations in Doxygen comments on every public function.
- Prefer explicit fixed-width integer types when ABI or serialisation matters. The public `rampg_t` deliberately uses plain `float` since the library is unit-agnostic and single-precision is the natural choice for embedded control loops.
- No heap allocation (`malloc` / `free` / VLAs). The caller owns the `rampg_t` storage.

### Error handling

- Public functions return `void`, the current output value, or `bool`. No `errno`; no exceptions.
- The library does NOT perform runtime precondition checks. Preconditions are documented via `@pre`. Honouring them is the caller's responsibility.

### Comment placement (Doxygen)

- Inline trailing annotations (`/**< ... */`) on `enum`/`struct` members are allowed only when the resulting line fits the 80-column limit.
- If any member's annotation would overrun, move **all** of that aggregate's member docs into a single structured Doxygen block above the type, as an `@details` list of `- ::SYMBOL  description` entries.
- Never mix inline and block forms within one aggregate, and never leave a trailing comment that clang-format would wrap onto a second line.
- After editing, verify with a `clang-format --style=file` no-reformat diff and an 80-column scan.

### Testing

- Run `meson test -C build` after every change.
- Add a test case for each bug fix and for each new feature.
- Tests live in `tests/test_*.c`; all tests must pass on both Linux and macOS (CI matrix).
- Tests must be deterministic. Do not depend on wall-clock time or thread scheduling.

---
