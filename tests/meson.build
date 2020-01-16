if get_option('tests')

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env.set('GSETTINGS_BACKEND', 'memory')
test_env.set('GSETTINGS_SCHEMA_DIR', '@0@/data'.format(meson.build_root()))
test_env.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('XDG_CONFIG_HOME', meson.current_source_dir())
test_env.set('XDG_CONFIG_DIRS', meson.current_source_dir())
# Use x11 backend by default
test_env.set('WLR_BACKENDS', 'x11')
test_env.set('XDG_RUNTIME_DIR', meson.current_build_dir())

test_cflags = [
]

test_link_args = [
  '-fPIC',
]

test_deps = [
  phoc_lib,
]

tests = [
  'server',
]

# Unit tests
foreach test : tests
  t = executable('test-@0@'.format(test),
                 ['test-@0@.c'.format(test)],
                 c_args: test_cflags,
                 pie: true,
                 link_args: test_link_args,
                 dependencies: phoc_dep)
  test(test, t, env: test_env)
endforeach

endif

