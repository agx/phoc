wayland_egl = dependency('wayland-egl')
wayland_cursor = dependency('wayland-cursor')
egl = dependency('egl', required: false, disabler: true)
glesv2 = dependency('glesv2', required: false, disabler: true)

executable('layer-shell-effects',
  sources: ['layer-shell-effects.c', 'egl-common.c', client_protos_headers, protos_sources],
  dependencies: [egl,
		 glesv2,
		 glib,
		 wayland_client,
		 wayland_cursor,
		 wayland_egl,
		 wlroots,
		],
)