# SPDX-License-Identifier: MIT

project(
  'rampg',
  'c',
  version: '0.3.0',
  meson_version: '>=0.57.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'A lightweight, unit-agnostic ramp generator with linear and acceleration-limited S-curve profiles, asymmetric rise/fall rates and output clamping, designed for deterministic embedded control loops in C11.'

# === Version ==================================================================
# include/rampg.h defines the version macros, so a copy-in build has them.
# Configuration fails when they differ from the project version.

fs = import('fs')
version_parts = meson.project_version().split('.')
version_macros = {
  'RAMPG_VERSION_MAJOR': version_parts[0],
  'RAMPG_VERSION_MINOR': version_parts[1],
  'RAMPG_VERSION_PATCH': version_parts[2],
  'RAMPG_VERSION': '"' + meson.project_version() + '"',
}
header_lines = []
foreach line : fs.read('include' / 'rampg.h').split('\n')
  header_lines += ' '.join(line.split())
endforeach
foreach name, value : version_macros
  if '#define @0@ @1@'.format(name, value) not in header_lines
    error('include/rampg.h must define @0@ as @1@'.format(name, value))
  endif
endforeach

cc = meson.get_compiler('c')

# rampg uses sqrtf() and isfinite() from <math.h>. On most toolchains these are
# intrinsics, but glibc-style targets need an explicit link against libm.
m_dep = cc.find_library('m', required: false)

public_headers = include_directories('include')

library_sources = files('src/rampg.c')

rampg_lib = static_library(
  'rampg',
  library_sources,
  include_directories: [public_headers],
  dependencies: [m_dep],
  install: true,
)

install_headers(
  'include/rampg.h',
  'include/rampg_conf.h',
  subdir: 'rampg'
)

rampg_dep = declare_dependency(
  include_directories: [public_headers],
  link_with: rampg_lib,
  dependencies: [m_dep],
)

meson.override_dependency('rampg', rampg_dep)

pkg = import('pkgconfig')
pkg.generate(
  rampg_lib,
  description: project_description,
  filebase: 'rampg',
  name: 'rampg',
  subdirs: 'rampg',
  libraries: ['-lm'],
)

if get_option('build_tests')
  subdir('tests')
endif
