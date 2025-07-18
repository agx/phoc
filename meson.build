project(
  'phoc',
  'c',
  version: '0.48.0',
  license: 'GPLv3+',
  meson_version: '>= 0.63.0',
  default_options: ['warning_level=1', 'buildtype=debugoptimized', 'c_std=gnu11'],
)

add_project_arguments(
  ['-DWLR_USE_UNSTABLE', '-DGMOBILE_USE_UNSTABLE_API', '-I' + meson.project_build_root()],
  language: 'c',
)

root_inc = include_directories('.')
src_inc = include_directories('src')
protocol_inc = include_directories('protocols')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')
desktopdir = prefix / datadir / 'applications'
pkgdatadir = prefix / datadir / meson.project_name()

cc = meson.get_compiler('c')

glib_ver = '2.80'
glib_ver_str = 'GLIB_VERSION_@0@'.format(glib_ver.replace('.', '_'))
glib_ver_cmp = '>=@0@'.format(glib_ver)

gio = dependency('gio-2.0', version: glib_ver_cmp)
glib = dependency('glib-2.0', version: glib_ver_cmp)
gobject = dependency('gobject-2.0', version: glib_ver_cmp)
glesv2 = dependency('glesv2')
gnome_desktop = dependency('gnome-desktop-3.0', version: '>=3.26')
gsettings_desktop_schemas_dep = dependency('gsettings-desktop-schemas')
input = dependency('libinput')
udev = dependency('libudev')
drm = dependency('libdrm')
pixman = dependency('pixman-1', version: '>= 0.43.4')
wayland_client = dependency('wayland-client', version: '>= 1.23.1')
wayland_protos = dependency('wayland-protocols', version: '>=1.15')
wayland_server = dependency('wayland-server', version: '>= 1.23.1')
xkbcommon = dependency('xkbcommon')
math = cc.find_library('m')
gmobile_dep = dependency(
  'gmobile',
  version: '>= 0.1.0',
  fallback: ['gmobile', 'gmobile_dep'],
  default_options: ['examples=false', 'introspection=false', 'gtk_doc=false', 'tests=false'],
)
gvdb_dep = subproject('gvdb', default_options: ['tests=false']).get_variable('gvdb_dep')
embed_wlroots = get_option('embed-wlroots')

if not embed_wlroots.disabled()
  # Try first to find wlroots as a subproject, then as a system dependency
  wlroots_proj = subproject(
    'wlroots',
    default_options: ['examples=false', 'renderers=gles2,vulkan'],
    required: false,
  )

  if embed_wlroots.enabled() and not wlroots_proj.found()
    error('Wlroots subproject not found but enabled')
  endif
endif

if not embed_wlroots.disabled() and wlroots_proj.found()
  wlroots = wlroots_proj.get_variable('wlroots')
else
  wlroots = dependency('wlroots-0.19')
endif
wlroots_has_xwayland = wlroots.get_variable(pkgconfig: 'have_xwayland', internal: 'have_xwayland') == 'true'

if get_option('xwayland').enabled() and not wlroots_has_xwayland
  error('Cannot enable Xwayland in phoc: wlroots has been built without Xwayland support')
endif
have_xwayland = get_option('xwayland').enabled()

use_dtrace = false
if not get_option('dtrace').disabled()
  has_sdt = cc.has_header('sys/sdt.h')

  if get_option('dtrace').enabled() and not has_sdt
    error('Cannot enable tracing, sys/sdt.h not found')
  endif

  use_dtrace = has_sdt
endif

use_sysprof = false
sysprof_capture = dependency(
  'sysprof-capture-4',
  version: '>= 48',
  required: get_option('sysprof').enabled(),
)
if not get_option('sysprof').disabled()
  use_sysprof = sysprof_capture.found()
endif

global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdate-time',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Wendif-labels',
  '-Werror=incompatible-pointer-types',
  '-Werror=missing-declarations',
  '-Werror=overflow',
  '-Werror=return-type',
  '-Werror=shift-count-overflow',
  '-Werror=shift-overflow=2',
  '-Werror=implicit-fallthrough=3',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wmissing-field-initializers',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wtype-limits',
  '-Wundef',
  '-Wunused-function',
  '-Wswitch-default',
]

# TODO:
#  '-Wfloat-equal',
#  '-Wswitch-enum',

if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif

c_args = cc.get_supported_arguments(test_c_args)

if cc.get_id() == 'clang'
  # Avoid cast align warnings for wl_container_of, etc
  c_args += '-Wno-cast-align'
endif

add_project_arguments(c_args, language: 'c')

gnome = import('gnome')
i18n = import('i18n')

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('LOCALEDIR', localedir)
config_h.set_quoted('PHOC_APP_ID', 'mobi.phosh.Phoc')
config_h.set_quoted('PHOC_VERSION', meson.project_version())
config_h.set('PHOC_XWAYLAND', have_xwayland, description: 'Whether xwayland is enabled')
config_h.set('PHOC_USE_DTRACE', use_dtrace, description: 'Whether tracing via dtrace/stp is enabled')
config_h.set('PHOC_USE_SYSPROF', use_sysprof, description: 'Whether tracing via sysprof is enabled')

phoc_config_h = configure_file(output: 'phoc-config.h', configuration: config_h)

subdir('po')
subdir('protocols')
subdir('data')
subdir('src')
subdir('tests')
subdir('tools')
subdir('doc')
subdir('examples')

run_data = configuration_data()
run_data.set('ABS_BUILDDIR', meson.current_build_dir())
run_data.set('ABS_SRCDIR', meson.current_source_dir())
configure_file(input: 'run.in', output: 'run', configuration: run_data)

summary(
  {
    'wlroots version': wlroots.version(),
    'wlroots as subproject': not embed_wlroots.disabled() and wlroots_proj.found(),
    'Xwayland': have_xwayland,
    'Documentation': get_option('gtk_doc'),
    'Manual pages': get_option('man'),
    'Tracing (dtrace)': use_dtrace,
    'Tracing (sysprof)': use_sysprof,
    'Tests': get_option('tests'),
  },
  bool_yn: true,
  section: 'Build',
)

gnome.post_install(glib_compile_schemas: true)
