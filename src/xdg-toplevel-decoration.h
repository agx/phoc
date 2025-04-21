/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "xdg-toplevel.h"

#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _PhocXdgToplevelDecoration PhocXdgToplevelDecoration;

void               phoc_handle_xdg_toplevel_decoration (struct wl_listener *listener, void *data);
void               phoc_xdg_toplevel_decoration_set_mode (PhocXdgToplevelDecoration *decoration);

G_END_DECLS
