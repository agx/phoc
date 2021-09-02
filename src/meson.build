sources = files(
  'settings.c',
  'settings.h',
  'cursor.c',
  'cursor.h',
  'desktop.c',
  'desktop.h',
  'gtk-shell.c',
  'gtk-shell.h',
  'ini.c',
  'ini.h',
  'input.c',
  'input.h',
  'keyboard.c',
  'keyboard.h',
  'keybindings.c',
  'keybindings.h',
  'layer_shell.c',
  'layers.h',
  'output.c',
  'output.h',
  'phosh-private.c',
  'phosh-private.h',
  'pointer.c',
  'pointer.h',
  'render.c',
  'render.h',
  'seat.c',
  'seat.h',
  'server.c',
  'server.h',
  'switch.c',
  'switch.h',
  'text_input.c',
  'text_input.h',
  'touch.c',
  'touch.h',
  'utils.c',
  'utils.h',
  'view.c',
  'view.h',
  'virtual.c',
  'virtual.h',
  'xdg_shell.c',
)

phoc_deps = [
  input,
  drm,
  gio,
  glesv2,
  gnome_desktop,
  gsettings_desktop_schemas_dep,
  math,
  pixman,
  wayland_server,
  wlroots,
  server_protos,
  xkbcommon,
]

if have_xwayland
  sources += 'xwayland.c'
  phoc_deps += dependency('xcb')
endif

phoc_lib = static_library(
  'phoc',
  sources,
  dependencies: phoc_deps,
  install: false,
)

libphoc_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: phoc_lib,
  dependencies: phoc_deps)

executable(
  'phoc',
  sources: 'main.c',
  dependencies: libphoc_dep,
  install: true,
)
