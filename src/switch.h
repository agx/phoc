#pragma once

#include "input.h"
#include "input-device.h"

#include <gio/gio.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_SWITCH (phoc_switch_get_type ())

G_DECLARE_FINAL_TYPE (PhocSwitch, phoc_switch, PHOC, SWITCH, PhocInputDevice);

PhocSwitch             *phoc_switch_new                            (struct wlr_input_device *device,
                                                                    PhocSeat                *seat);
gboolean                phoc_input_device_has_tablet_mode_switch   (PhocSwitch              *self);
gboolean                phoc_input_device_has_lid_switch           (PhocSwitch              *self);

G_END_DECLS
