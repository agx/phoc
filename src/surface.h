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
const pixman_region32_t *phoc_surface_get_damage (PhocSurface *self);
void                     phoc_surface_add_damage (PhocSurface *self, pixman_region32_t *damage);
void                     phoc_surface_add_damage_box (PhocSurface *self, struct wlr_box *box);
void                     phoc_surface_clear_damage (PhocSurface *self);

G_END_DECLS
