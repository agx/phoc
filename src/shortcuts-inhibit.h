/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>

G_BEGIN_DECLS

#define PHOC_TYPE_KEYBOARD_SHORTCUTS_INHIBIT (phoc_keyboard_shortcuts_inhibit_get_type ())

G_DECLARE_FINAL_TYPE (PhocKeyboardShortcutsInhibit, phoc_keyboard_shortcuts_inhibit, PHOC, KEYBOARD_SHORTCUTS_INHIBIT, GObject)

void                          phoc_handle_keyboard_shortcuts_inhibit_new_inhibitor(struct wl_listener *listener, void *data);

struct wlr_surface*           phoc_keyboard_shortcuts_inhibit_get_surface (PhocKeyboardShortcutsInhibit *self);
PhocKeyboardShortcutsInhibit *phoc_keyboard_shortcuts_inhibit_new (struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor);
