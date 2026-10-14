# SPDX-License-Identifier: MIT

test_exe = executable(
  'test_rampg',
  files('test_rampg.c'),
  dependencies: [rampg_dep],
  include_directories: [public_headers, generated_headers],
)

test('rampg module', test_exe)

# Compiles every C block in README.md, CONTRIBUTING.md and docs/ against the
# public headers, so a documented signature cannot drift from the code.
test(
  'rampg docs examples',
  import('python').find_installation(),
  args: [
    files('docs/check_snippets.py'),
    '--root', meson.project_source_root(),
    '--lang', 'c',
    '--prelude', '#include "rampg.h"',
    '--',
  ] + meson.get_compiler('c').cmd_array() + [
    '-std=c11',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Werror',
    '-Wno-unused-function',
    '-Wno-unused-variable',
    '-I' + (meson.project_source_root() / 'include'),
    '-c', '{source}', '-o', '{object}',
  ],
  timeout: 60,
)
