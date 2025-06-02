subdir('tracing')

check_indent = find_program('check-indent', required: true)
test('check-indent', check_indent, args: meson.project_source_root())

executable(
  'phoc-outputs-states',
  sources: ['phoc-outputs-states.c'],
  dependencies: [glib, libphoc_static_dep],
  install: true,
)
