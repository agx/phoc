/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-debug-control"

#include "phoc-config.h"
#include "phoc-enums.h"
#include "debug-control.h"
#include "server.h"

#include <gio/gio.h>

#define DEBUG_CONTROL_DBUS_PATH "/mobi/phosh/Phoc/DebugControl"
#define DEBUG_CONTROL_DBUS_NAME "mobi.phosh.Phoc.DebugControl"

/**
 * PhocDebugControl:
 *
 * DBus Debug control interface
 */

enum {
  PROP_0,
  PROP_SERVER,
  PROP_EXPORTED,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocDebugControl {
  PhocDBusDebugControlSkeleton parent;

  guint                        dbus_name_id;
  gboolean                     exported;
};

static void phoc_dbus_debug_control_iface_init (PhocDBusDebugControlIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhocDebugControl, phoc_debug_control,
                         PHOC_DBUS_TYPE_DEBUG_CONTROL_SKELETON,
                         G_IMPLEMENT_INTERFACE (PHOC_DBUS_TYPE_DEBUG_CONTROL,
                                                phoc_dbus_debug_control_iface_init))

static void
phoc_dbus_debug_control_iface_init (PhocDBusDebugControlIface *iface)
{
}


static gboolean
transform_flag_to_bool (GBinding     *binding,
                        const GValue *from_value,
                        GValue       *to_value,
                        gpointer      user_data)
{
  guint mask = GPOINTER_TO_UINT (user_data);
  PhocServerDebugFlags flags = g_value_get_flags (from_value);
  gboolean enabled = !!(flags & mask);

  g_value_set_boolean (to_value, enabled);
  return TRUE;
}


static gboolean
transform_flag_from_bool (GBinding     *binding,
                          const GValue *from_value,
                          GValue       *to_value,
                          gpointer      user_data)
{
  g_autoptr (PhocServer) server = PHOC_SERVER (g_binding_dup_source (binding));
  guint mask = GPOINTER_TO_UINT (user_data);
  gboolean enable = g_value_get_boolean (from_value);
  PhocServerDebugFlags flags = phoc_server_get_debug_flags (server);

  if (enable)
    flags |= mask;
  else
    flags &= ~mask;

  g_value_set_flags (to_value, flags);
  return TRUE;
}


static void
on_bus_acquired (GDBusConnection *connection, const char *name, gpointer user_data)
{
  PhocDebugControl *self = user_data;
  g_autoptr (GError) err = NULL;

  if (g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                        connection,
                                        DEBUG_CONTROL_DBUS_PATH,
                                        &err)) {
    self->exported = TRUE;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EXPORTED]);
    g_debug ("Debug interface exported on '%s'", DEBUG_CONTROL_DBUS_NAME);
  } else {
    g_warning ("Failed to export on %s: %s", DEBUG_CONTROL_DBUS_NAME, err->message);
  }
}


static void
phoc_debug_control_set_server (PhocDebugControl *self, PhocServer *server)
{
  g_autoptr (GFlagsClass) eclass = NULL;
  PhocServerDebugFlags exported[] = {
    PHOC_SERVER_DEBUG_FLAG_TOUCH_POINTS,
    PHOC_SERVER_DEBUG_FLAG_DAMAGE_TRACKING,
  };

  eclass = G_FLAGS_CLASS (g_type_class_ref (phoc_server_debug_flags_get_type ()));
  for (int i = 0; i < G_N_ELEMENTS (exported); i++) {
    PhocServerDebugFlags flag = exported[i];
    GFlagsValue *fv = g_flags_get_first_value (eclass, flag);
    if (!fv) {
      g_critical ("Got invalid debug flag value: %d", flag);
      continue;
    }
    g_object_bind_property_full (server,
                                 "debug-flags",
                                 self,
                                 fv->value_nick,
                                 G_BINDING_SYNC_CREATE |
                                 G_BINDING_BIDIRECTIONAL,
                                 transform_flag_to_bool,
                                 transform_flag_from_bool,
                                 GUINT_TO_POINTER (flag),
                                 NULL);
  }

  g_object_bind_property (server,
                          "log-domains",
                          self,
                          "log-domains",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
}


static void
phoc_debug_control_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhocDebugControl *self = PHOC_DEBUG_CONTROL (object);

  switch (property_id) {
  case PROP_SERVER:
    phoc_debug_control_set_server (self, g_value_get_object (value));
    break;
  case PROP_EXPORTED:
    phoc_debug_control_set_exported (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_debug_control_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhocDebugControl *self = PHOC_DEBUG_CONTROL (object);

  switch (property_id) {
  case PROP_EXPORTED:
    g_value_set_boolean (value, self->exported);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_debug_control_dispose (GObject *object)
{
  PhocDebugControl *self = PHOC_DEBUG_CONTROL (object);

  if (self->exported) {
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));
    self->exported = FALSE;
  }
  g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);

  G_OBJECT_CLASS (phoc_debug_control_parent_class)->dispose (object);
}


static void
phoc_debug_control_class_init (PhocDebugControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_debug_control_get_property;
  object_class->set_property = phoc_debug_control_set_property;
  object_class->dispose = phoc_debug_control_dispose;

  props[PROP_SERVER] =
    g_param_spec_object ("server", "", "",
                         PHOC_TYPE_SERVER,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  props[PROP_EXPORTED] =
    g_param_spec_boolean ("exported", "", "",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_debug_control_init (PhocDebugControl *self)
{
}


PhocDebugControl *
phoc_debug_control_new (PhocServer *server)
{
  return g_object_new (PHOC_TYPE_DEBUG_CONTROL, "server", server, NULL);
}


void
phoc_debug_control_set_exported (PhocDebugControl *self, gboolean exported)
{
  g_assert (PHOC_IS_DEBUG_CONTROL (self));

  if (self->exported == exported)
    return;

  if (exported) {
    self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                         DEBUG_CONTROL_DBUS_NAME,
                                         G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                         G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                         on_bus_acquired,
                                         NULL,
                                         NULL,
                                         self,
                                         NULL);
  } else {
    g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);
    self->exported = FALSE;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EXPORTED]);
  }
}
