/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <wlr/types/wlr_data_device.h>

#include <glib.h>

#pragma once

G_BEGIN_DECLS

typedef struct _PhocSeat PhocSeat;
typedef struct _PhocDragIcon PhocDragIcon;


struct _PhocDragIcon {
  PhocSeat             *seat;
  struct wlr_drag_icon *wlr_drag_icon;

  double                x, y;
  double                dx, dy;

  struct wl_listener    surface_commit;
  struct wl_listener    map;
  struct wl_listener    unmap;
  struct wl_listener    destroy;
};


PhocDragIcon *phoc_drag_icon_create (PhocSeat *seat, struct wlr_drag_icon *icon);
void          phoc_drag_icon_update_position (PhocDragIcon *self);

G_END_DECLS
