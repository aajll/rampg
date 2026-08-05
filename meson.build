project(
  'rampg',
  'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'A lightweight, unit-agnostic linear ramp generator with asymmetric rise/fall rates and output clamping, designed for deterministic embedded control loops in C11.'

ver = meson.project_version().split('.')
major = ver[0].to_int()
minor = ver[1].to_int()
patch = ver[2].to_int()

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('MAJOR', major)
conf.set('MINOR', minor)
conf.set('PATCH', patch)

generated_version_header = configure_file(
  input: 'config' / 'rampg_version.h.in',
  output: 'rampg_version.h',
  configuration: conf,
  install: true,
  install_dir: get_option('includedir') / 'rampg',
)

public_headers = include_directories('include')
generated_headers = include_directories('.')

library_sources = files('src/rampg.c')

rampg_lib = static_library(
  'rampg',
  library_sources,
  include_directories: [public_headers, generated_headers],
  install: true,
)

install_headers(
  'include/rampg.h',
  'include/rampg_conf.h',
  subdir: 'rampg'
)

rampg_dep = declare_dependency(
  include_directories: [public_headers, generated_headers],
  link_with: rampg_lib,
  sources: generated_version_header,
)

meson.override_dependency('rampg', rampg_dep)

pkg = import('pkgconfig')
pkg.generate(
  rampg_lib,
  description: project_description,
  filebase: 'rampg',
  name: 'rampg',
  subdirs: 'rampg',
)

if get_option('build_tests')
  subdir('tests')
endif
