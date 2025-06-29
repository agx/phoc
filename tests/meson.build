if not get_option('tests')
  subdir_done()
endif

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env.set('GSETTINGS_BACKEND', 'memory')
test_env.set('GSETTINGS_SCHEMA_DIR', '@0@/data'.format(meson.project_build_root()))
test_env.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('MALLOC_PERTURB_', '123')
test_env.set('XDG_CONFIG_HOME', meson.current_source_dir())
test_env.set('XDG_CONFIG_DIRS', meson.current_source_dir())
# Use x11 backend by default
test_env.set('WLR_BACKENDS', 'x11')
test_env.set('WLR_RENDERER', 'pixman')
test_env.set('XDG_RUNTIME_DIR', meson.current_build_dir())

# For -Db_sanitize=address
test_env.set(
  'LSAN_OPTIONS',
  'suppressions=@0@/data/leak-suppress.txt'.format(meson.project_source_root()),
)
test_env.set('ASAN_OPTIONS', 'fast_unwind_on_malloc=0')
#test_env.set('ASAN_OPTIONS', 'fast_unwind_on_malloc=0:disable_coredump=0:unmap_shadow_on_exit=1:abort_on_error=1')

test_cflags = [
  '-DTEST_PHOC_INI="@0@/phoc.ini"'.format(meson.current_source_dir()),
  '-DTEST_PHOC_CLIENT_TIMEOUT=10',
]

test_link_args = ['-fPIC']

tests = [
  'client',
  'color-rect',
  'layer-shell',
  'layer-shell-effects',
  'outputs-states',
  'phosh-private',
  'property-easer',
  'run',
  'settings',
  'server',
  'timed-animation',
  'utils',
  'xdg-decoration',
  'xdg-shell',
]

if have_xwayland == true
  tests += ['xwayland']
endif

phoctest_sources = ['testlib.c', 'testlib-layer-shell.c']

phoctest_lib = static_library(
  'phoctest',
  [phoctest_sources, client_protos_headers, protos_sources],
  c_args: test_cflags,
  dependencies: [libphoc_dep, wayland_client],
)

phoctest_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: phoctest_lib,
)

# Unit tests
foreach test : tests
  t = executable(
    'test-@0@'.format(test),
    ['test-@0@.c'.format(test)],
    c_args: test_cflags,
    pie: true,
    link_args: test_link_args,
    dependencies: [phoctest_dep, libphoc_dep],
  )
  test(test, t, depends: compiled_schemas, env: test_env)
endforeach
