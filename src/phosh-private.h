/*
 * Copyright (C) 2019,2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "keybindings.h"

#include <wlr/types/wlr_layer_shell_v1.h>
#include "glib-object.h"

G_BEGIN_DECLS

#define PHOSH_PRIVATE_XDG_SWITCHER_SINCE_VERSION 2

#define PHOC_TYPE_PHOSH_PRIVATE (phoc_phosh_private_get_type ())

G_DECLARE_FINAL_TYPE (PhocPhoshPrivate, phoc_phosh_private, PHOC, PHOSH_PRIVATE, GObject)

struct phoc_phosh_private_screencopy_frame {
  struct wl_resource *resource, *toplevel;
  struct phosh_private *phosh;
  struct wl_listener view_destroy;

  enum wl_shm_format format;
  uint32_t width;
  uint32_t height;
  uint32_t stride;

  struct wl_shm_buffer *buffer;
  struct roots_view *view;
};

typedef struct _PhocDesktop PhocDesktop;
PhocPhoshPrivate *phoc_phosh_private_new (PhocDesktop *desktop);
PhocPhoshPrivate *phoc_phosh_private_from_resource(struct wl_resource *resource);
struct phoc_phosh_private_screencopy_frame *phoc_phosh_private_screencopy_frame_from_resource(struct wl_resource *resource);
bool   phoc_phosh_private_forward_keysym (PhocKeyCombo *combo, uint32_t timestamp);

G_END_DECLS
