# Contributing to rampg

rampg is a small C library used as a control-loop primitive for
unit-agnostic linear ramping with asymmetric rise/fall rates and
output clamping. It is designed to be safe to drop into embedded
firmware running deterministic control loops.

## Getting started

The same commands CI runs, locally:

```sh
# Configure with tests + sanitisers (CI default on Linux)
meson setup build --buildtype=debug -Dbuild_tests=true \
                  -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose

# Release build (the configuration a downstream consumer uses)
meson setup build_rel --buildtype=release -Dbuild_tests=false
meson compile -C build_rel
```

## Source style

- `.clang-format` is mandatory. Run `clang-format -i` on every modified
  `.c` / `.h` file before submitting.
- 8-space indent, Linux brace style, 80-column limit. Match the
  existing conventions; do not reformat unrelated code.
- The Meson build system is the single source of truth. Update
  `meson.build` / `tests/meson.build` when adding or removing source
  files.
- No CMake, no Make, no other build systems.

### Header section banners

Every `.h` file in the project must contain these section banners in
order:

```
/* ================ INCLUDES ================================================ */
/* ================ DEFINES ================================================= */
/* ================ STRUCTURES ============================================== */
/* ================ TYPEDEFS ================================================ */
/* ================ MACROS ================================================== */
/* ================ GLOBAL VARIABLES ======================================== */
/* ================ GLOBAL PROTOTYPES ======================================= */
```

Every `.c` file must contain these section banners in order:

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

Empty sections are kept on purpose: an empty `STATIC VARIABLES` banner
communicates the deliberate absence of file-scope mutable state. Do
not invent alternative section styles.

## C language rules

- C11 only.
- Use fixed-width types from `<stdint.h>` and `<stdbool.h>` when ABI or
  serialisation matters. The public `rampg_t` struct uses plain `float`
  by design, since the library is unit-agnostic and single-precision is
  the natural choice for embedded control loops.
- No heap allocation (`malloc`, `free`, VLAs). The caller owns the
  `rampg_t` storage.
- Public functions either return `void`, return the current output
  value, or return `bool`. There is no `errno` and no exceptions.
- Preconditions are documented via `@pre` annotations on every public
  function. The library does not perform runtime precondition checks;
  honouring `@pre` is the caller's responsibility.

## Configuration

- Compile-time configuration lives in `include/rampg_conf.h`. It is
  auto-included by `rampg.h` and any option can be overridden by
  defining it before the include.
- Do not add runtime configuration knobs to the public API for things
  that can be expressed as a compile-time default. Keep the API
  surface small.

## Tests

- Add a test for every bug fix.
- Add a test for every new feature.
- Tests live in `tests/test_*.c` and run under AddressSanitizer in CI
  (plus UndefinedBehaviorSanitizer on Linux).
- All tests must pass on both Linux and macOS, since CI runs the matrix
  on both.
- Tests must be deterministic. Do not depend on wall-clock time,
  thread scheduling, or uninitialised memory.

## API stability

The public API in `include/rampg.h` and the configuration macros in
`include/rampg_conf.h` are the project's contract:

- Breaking changes to function signatures, struct layout, or the
  meaning of `rampg_conf.h` macros require a new major release and a
  deprecation period.
- Renaming or removing a public symbol is a breaking change.
- Adding new functions, new struct fields at the end, or new
  configuration macros with backwards-compatible defaults is a minor
  release.

## Commits

Use Conventional Commits:

- `feat: ...` new feature
- `fix: ...` bug fix
- `doc: ...` documentation only
- `test: ...` test-only changes
- `chore: ...` build, CI, release work
- `refactor: ...` code change that neither fixes a bug nor adds a feature
- `lint: ...` formatting or lint-only changes

Keep the subject under ~70 characters. Use the body to explain _why_
the change is needed, not _what_ the diff already shows.

## Pull requests

- Open an issue first for non-trivial changes so the design can be
  agreed before implementation.
- Keep PRs focused. One feature or one fix per PR.
- The PR description should explain _why_ the change is needed.
- All CI checks must pass: tests on Linux + macOS under sanitisers,
  release build.
- If the change is user-visible, add a `CHANGELOG.md` entry under an
  `## [Unreleased]` heading (or update the next release heading if one
  is already open).

## When in doubt

Open an issue and discuss before writing code. The library is small
enough that even modest design changes have outsized implications for
downstream consumers.
