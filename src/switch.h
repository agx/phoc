#pragma once

#include "input.h"
#include "input-device.h"

#include <gio/gio.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_SWITCH (phoc_switch_get_type ())

G_DECLARE_FINAL_TYPE (PhocSwitch, phoc_switch, PHOC, SWITCH, PhocInputDevice);

PhocSwitch *phoc_switch_new (struct wlr_input_device *device, PhocSeat *seat);

G_END_DECLS
