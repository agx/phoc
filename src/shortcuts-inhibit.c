/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Andrey Skvortsov <andrej.skvortzov@gmail.com>
 */

#define G_LOG_DOMAIN "phoc-shortcuts-inhibit"

#include "phoc-config.h"

#include <assert.h>

#include "seat.h"
#include "shortcuts-inhibit.h"

enum {
  PROP_0,
  PROP_INHIBITOR,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  DESTROY,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

typedef struct _PhocKeyboardShortcutsInhibit {
  GObject parent;

  struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor;
  struct wl_listener destroy;
} PhocKeyboardShortcutsInhibit;

G_DEFINE_TYPE (PhocKeyboardShortcutsInhibit, phoc_keyboard_shortcuts_inhibit, G_TYPE_OBJECT)


static void
handle_keyboard_shortcuts_inhibit_destroy (struct wl_listener *listener, void *data)
{
  PhocKeyboardShortcutsInhibit *self = wl_container_of (listener, self, destroy);

  g_assert (PHOC_IS_KEYBOARD_SHORTCUTS_INHIBIT (self));

  g_debug ("Keyboard shortcuts inhibitor %p destroyed", self->inhibitor);
  g_signal_emit (self, signals[DESTROY], 0);
}


void
phoc_handle_keyboard_shortcuts_inhibit_new_inhibitor (struct wl_listener *listener, void *data)
{
  struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor = data;
  PhocSeat *seat = phoc_seat_from_wlr_seat (inhibitor->seat);

  g_assert (PHOC_IS_SEAT (seat));
  g_debug ("Keyboard shortcuts inhibitor %p requested", inhibitor);
  phoc_seat_add_shortcuts_inhibit (seat, inhibitor);
}


static void
phoc_keyboard_shortcuts_set_inhibitor (PhocKeyboardShortcutsInhibit               *self,
                                       struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor)
{
  g_assert (inhibitor);
  self->inhibitor = inhibitor;
}


static void
phoc_keyboard_shortcuts_inhibit_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  PhocKeyboardShortcutsInhibit *self = PHOC_KEYBOARD_SHORTCUTS_INHIBIT (object);

  switch (property_id) {
  case PROP_INHIBITOR:
    phoc_keyboard_shortcuts_set_inhibitor (self, g_value_get_pointer (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_keyboard_shortcuts_inhibit_constructed (GObject *object)
{
  PhocKeyboardShortcutsInhibit *self = PHOC_KEYBOARD_SHORTCUTS_INHIBIT (object);

  G_OBJECT_CLASS (phoc_keyboard_shortcuts_inhibit_parent_class)->constructed (object);

  self->destroy.notify = handle_keyboard_shortcuts_inhibit_destroy;
  wl_signal_add (&self->inhibitor->events.destroy, &self->destroy);

  wlr_keyboard_shortcuts_inhibitor_v1_activate (self->inhibitor);
}


struct wlr_surface*
phoc_keyboard_shortcuts_inhibit_get_surface (PhocKeyboardShortcutsInhibit *self)
{
  return self->inhibitor->surface;
}


static void
phoc_keyboard_shortcuts_inhibit_finalize (GObject *object)
{
  PhocKeyboardShortcutsInhibit *self = PHOC_KEYBOARD_SHORTCUTS_INHIBIT (object);

  wl_list_remove (&self->destroy.link);
  G_OBJECT_CLASS (phoc_keyboard_shortcuts_inhibit_parent_class)->finalize (object);
}


static void
phoc_keyboard_shortcuts_inhibit_class_init (PhocKeyboardShortcutsInhibitClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_keyboard_shortcuts_inhibit_constructed;
  object_class->finalize = phoc_keyboard_shortcuts_inhibit_finalize;
  object_class->set_property = phoc_keyboard_shortcuts_inhibit_set_property;

  /**
   * PhocKeyboardShortcutsInhibit:inhibitor:
   *
   * The underlying wlroots keyboard shortcuts inhibitor
   */
  props[PROP_INHIBITOR] =
    g_param_spec_pointer ("inhibitor", "", "",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  /**
   * PhocKeyboardShortcutsInhibit:destroy:
   *
   * The underlying wlr keyboard shortcuts inhibitor is about to be destroyed
   */
  signals[DESTROY] = g_signal_new ("destroy",
                                   G_TYPE_FROM_CLASS (object_class),
                                   G_SIGNAL_RUN_LAST,
                                   0,
                                   NULL, NULL, NULL,
                                   G_TYPE_NONE, 0);
}


static void
phoc_keyboard_shortcuts_inhibit_init (PhocKeyboardShortcutsInhibit *self)
{
}


PhocKeyboardShortcutsInhibit *
phoc_keyboard_shortcuts_inhibit_new (struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor)
{
  return g_object_new (PHOC_TYPE_KEYBOARD_SHORTCUTS_INHIBIT,
                       "inhibitor", inhibitor,
                       NULL);
}
