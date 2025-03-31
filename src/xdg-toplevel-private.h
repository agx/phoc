/*
 * Copyright (C) 2023 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "xdg-toplevel.h"
#include "xdg-toplevel-decoration.h"

#include <glib-object.h>

G_BEGIN_DECLS

void               phoc_xdg_toplevel_set_decoration        (PhocXdgToplevel           *self,
                                                            PhocXdgToplevelDecoration *decoration);
PhocXdgToplevelDecoration *
                   phoc_xdg_toplevel_get_decoration        (PhocXdgToplevel *self);
struct wlr_xdg_surface *
                   phoc_xdg_toplevel_get_wlr_xdg_surface   (PhocXdgToplevel *self);

G_END_DECLS
