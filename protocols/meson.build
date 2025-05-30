fs = import('fs')

wl_protocol_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')

wayland_scanner = find_program('wayland-scanner')

wayland_protocols = [
  [wl_protocol_dir, 'stable/tablet/tablet-v2.xml'],
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wl_protocol_dir, 'staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml'],
  [wl_protocol_dir, 'staging/ext-image-capture-source/ext-image-capture-source-v1.xml'],
  [wl_protocol_dir, 'staging/ext-image-copy-capture/ext-image-copy-capture-v1.xml'],
  [wl_protocol_dir, 'unstable/pointer-constraints/pointer-constraints-unstable-v1.xml'],
  [wl_protocol_dir, 'unstable/xdg-decoration/xdg-decoration-unstable-v1.xml'],
  ['gtk-shell.xml'],
  ['phoc-device-state-unstable-v1.xml'],
  ['phoc-layer-shell-effects-unstable-v1.xml'],
  ['phosh-private.xml'],
  ['wlr-foreign-toplevel-management-unstable-v1.xml'],
  ['wlr-layer-shell-unstable-v1.xml'],
  ['wlr-output-power-management-unstable-v1.xml'],
  ['wlr-screencopy-unstable-v1.xml'],
]

wayland_client_protocols = [[wl_protocol_dir, 'staging/ext-idle-notify/ext-idle-notify-v1.xml']]

protos_sources = []
server_protos_headers = []
client_protos_headers = []

foreach p : wayland_protocols
  xml = join_paths(p)

  base = fs.name(xml)
  proto = fs.stem(base)

  protos_sources += custom_target(
    '@0@ source'.format(proto),
    input: xml,
    output: '@0@-protocol.c'.format(proto),
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )

  server_protos_headers += custom_target(
    '@0@ server header'.format(proto),
    input: xml,
    output: '@0@-protocol.h'.format(proto),
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  )

  client_protos_headers += custom_target(
    '@0@ client header'.format(proto),
    input: xml,
    output: '@0@-client-protocol.h'.format(proto),
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach

foreach p : wayland_client_protocols
  xml = join_paths(p)

  base = fs.name(xml)
  proto = fs.stem(base)

  protos_sources += custom_target(
    '@0@ source'.format(proto),
    input: xml,
    output: '@0@-protocol.c'.format(proto),
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )

  client_protos_headers += custom_target(
    '@0@ client header'.format(proto),
    input: xml,
    output: '@0@-client-protocol.h'.format(proto),
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach
