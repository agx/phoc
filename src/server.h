#pragma once

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/session.h>
#include <wlr/config.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_data_device.h>
#ifdef PHOC_XWAYLAND
#include <wlr/xwayland.h>
#endif
#include "settings.h"
#include "desktop.h"
#include "input.h"

G_BEGIN_DECLS

#define PHOC_TYPE_SERVER (phoc_server_get_type())

G_DECLARE_FINAL_TYPE (PhocServer, phoc_server, PHOC, SERVER, GObject);

/* TODO: we keep the struct public due to heaps of direct access
   which will be replaced by getters and setters over time */
struct _PhocServer {
  GObject parent;

  /* Phoc resources */
  struct roots_config *config;
  PhocDesktop *desktop;
  struct roots_input *input;

  /* Wayland resources */
  struct wl_display *wl_display;

  /* WLR tools */
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;

  /* Global resources */
  struct wlr_data_device_manager *data_device_manager;
};

PhocServer *phoc_server_get_default (void);

G_END_DECLS
