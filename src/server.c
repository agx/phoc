/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-server"

#include "config.h"
#include "server.h"

G_DEFINE_TYPE(PhocServer, phoc_server, G_TYPE_OBJECT);

static void
phoc_server_constructed (GObject *object)
{
  PhocServer *self = PHOC_SERVER (object);

  self->wl_display = wl_display_create();
  if (self->wl_display == NULL)
    g_error("Could not create wayland display");

  self->backend = wlr_backend_autocreate(self->wl_display, NULL);
  if (self->backend == NULL)
    g_error("Could not start backend");

  self->renderer = wlr_backend_get_renderer(self->backend);
  if (self->renderer == NULL)
    g_error("Could not create renderer");

  self->data_device_manager =
    wlr_data_device_manager_create(self->wl_display);
  wlr_renderer_init_wl_display(self->renderer, self->wl_display);

  G_OBJECT_CLASS (phoc_server_parent_class)->constructed (object);
}


static void
phoc_server_dispose (GObject *object)
{
  PhocServer *self = PHOC_SERVER (object);

#ifdef PHOC_XWAYLAND
  // We need to shutdown Xwayland before disconnecting all clients, otherwise
  // wlroots will restart it automatically.
  g_clear_pointer (&self->desktop->xwayland, wlr_xwayland_destroy);
#endif

  g_clear_pointer (&self->wl_display, &wl_display_destroy_clients);
  g_clear_pointer (&self->wl_display, &wl_display_destroy);
  g_clear_object (&self->desktop);

  G_OBJECT_CLASS (phoc_server_parent_class)->finalize (object);
}


static void
phoc_server_class_init (PhocServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_server_constructed;
  object_class->finalize = phoc_server_dispose;
}

static void
phoc_server_init (PhocServer *self)
{
}

PhocServer *
phoc_server_get_default (void)
{
  static PhocServer *instance;
  static gboolean initialized;

  if (G_UNLIKELY (instance == NULL)) {
    if (G_UNLIKELY (initialized)) {
      g_error ("PhocServer can only be initialized once");
    }
    g_debug("Creating server");
    instance = g_object_new (PHOC_TYPE_SERVER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
    initialized = TRUE;
  }

  return instance;
}
