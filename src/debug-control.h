/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "phoc-dbus-debug-control.h"
#include "server.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_DEBUG_CONTROL (phoc_debug_control_get_type ())

G_DECLARE_FINAL_TYPE (PhocDebugControl, phoc_debug_control, PHOC, DEBUG_CONTROL,
                      PhocDBusDebugControlSkeleton)

PhocDebugControl       *phoc_debug_control_new                   (PhocServer       *server);
void                    phoc_debug_control_set_exported          (PhocDebugControl *self,
                                                                  gboolean          exported);

G_END_DECLS
