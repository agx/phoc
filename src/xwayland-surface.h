/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "view.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_XWAYLAND_SURFACE (phoc_xwayland_surface_get_type ())

#ifdef PHOC_XWAYLAND
G_DECLARE_FINAL_TYPE (PhocXWaylandSurface, phoc_xwayland_surface, PHOC, XWAYLAND_SURFACE, PhocView)

PhocXWaylandSurface *phoc_xwayland_surface_new (struct wlr_xwayland_surface *surface);
PhocXWaylandSurface *phoc_xwayland_surface_from_view(PhocView *view);
struct wlr_xwayland_surface *phoc_xwayland_surface_get_wlr_surface (PhocXWaylandSurface *self);

#endif

G_END_DECLS
