sources = [
  'settings.c',
  'cursor.c',
  'desktop.c',
  'ini.c',
  'input.c',
  'keyboard.c',
  'keybindings.c',
  'layer_shell.c',
  'main.c',
  'output.c',
  'phosh.c',
  'render.c',
  'seat.c',
  'server.c',
  'switch.c',
  'text_input.c',
  'view.c',
  'virtual_keyboard.c',
  'xdg_shell.c',
  'xdg_shell_v6.c',
]

if have_xwayland
  sources += 'xwayland.c'
endif

phoc_deps = [
  input,
  gio,
  math,
  pixman,
  wayland_server,
  wlroots,
  server_protos,
  xkbcommon,
]

executable(
  'phoc',
  sources,
  dependencies: phoc_deps,
  install: true,
)
