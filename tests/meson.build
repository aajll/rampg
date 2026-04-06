test_exe = executable(
  'test_rampg',
  files('test_rampg.c'),
  dependencies: [rampg_dep],
  include_directories: [public_headers, generated_headers],
)

test('rampg module', test_exe)
