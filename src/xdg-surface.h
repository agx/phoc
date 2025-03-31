/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "view.h"

#include <wlr/types/wlr_xdg_shell.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_XDG_TOPLEVEL (phoc_xdg_toplevel_get_type ())

G_DECLARE_FINAL_TYPE (PhocXdgToplevel, phoc_xdg_toplevel, PHOC, XDG_TOPLEVEL, PhocView)

PhocXdgToplevel    *phoc_xdg_toplevel_new (struct wlr_xdg_toplevel *xdg_toplevel);
void                phoc_xdg_toplevel_get_geometry (PhocXdgToplevel *self, struct wlr_box *geom);
struct wlr_surface *phoc_xdg_toplevel_get_wlr_surface_at (PhocXdgToplevel *self,
                                                          double           sx,
                                                          double           sy,
                                                          double          *sub_x,
                                                          double          *sub_y);

void                phoc_handle_xdg_shell_toplevel (struct wl_listener *listener, void *data);

G_END_DECLS
