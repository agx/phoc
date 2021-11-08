#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_seat.h>
#include "settings.h"
#include "input-device.h"

G_BEGIN_DECLS

#define PHOC_TYPE_TOUCH (phoc_touch_get_type ())

G_DECLARE_FINAL_TYPE (PhocTouch, phoc_touch, PHOC, TOUCH, PhocInputDevice);

/* TODO: we keep the struct public due to the list links and
   notifiers but we should avoid other member access */
struct _PhocTouch {
  PhocInputDevice          parent;

  struct wl_list           link; // seat::touch
  struct wl_listener       touch_destroy;
};

PhocTouch *phoc_touch_new (struct wlr_input_device *device, PhocSeat *seat);

G_END_DECLS
