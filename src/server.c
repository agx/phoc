/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-server"

#include "server.h"

G_DEFINE_TYPE(PhocServer, phoc_server, G_TYPE_OBJECT);

static void
phoc_server_class_init (PhocServerClass *klass)
{
}

static void
phoc_server_init (PhocServer *self)
{
}

PhocServer *
phoc_server_get_default (void)
{
  static PhocServer *instance;
  
  if (instance == NULL) {
    g_debug("Creating server");
    instance = g_object_new (PHOC_TYPE_SERVER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
