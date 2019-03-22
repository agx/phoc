#pragma once

#include <wayland-server.h>
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

struct phoc_server {
  /* Phoc resources */
  struct roots_config *config;
  struct roots_desktop *desktop;
  struct roots_input *input;

  /* Wayland resources */
  struct wl_display *wl_display;

  /* WLR tools */
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;

  /* Global resources */
  struct wlr_data_device_manager *data_device_manager;
};

extern struct phoc_server server;
