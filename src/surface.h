/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wlr/types/wlr_compositor.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_SURFACE (phoc_surface_get_type ())

G_DECLARE_FINAL_TYPE (PhocSurface, phoc_surface, PHOC, SURFACE, GObject)

PhocSurface             *phoc_surface_new (struct wlr_surface *self);

G_END_DECLS
