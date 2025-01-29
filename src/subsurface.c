/*
 * Copyright (C) 2024-2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-subsurface"

#include "phoc-config.h"

#include "subsurface.h"
#include "surface.h"


enum {
  PROP_0,
  PROP_WLR_SUBSURFACE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhocSubsurface:
 *
 * A subsurface attached to a [type@View] or [type@ViewChild].
 */
typedef struct _PhocSubsurface {
  PhocViewChild          parent_instance;

  struct wlr_subsurface *wlr_subsurface;
  struct previous {
    gint32 x;
    gint32 y;
    struct wl_list *prev;
    struct wl_list *next;
  }                      previous;

  struct wl_listener     parent_commit;
  struct wl_listener     destroy;
} PhocSubsurface;

G_DEFINE_FINAL_TYPE (PhocSubsurface, phoc_subsurface, PHOC_TYPE_VIEW_CHILD)


static void
subsurface_get_pos (PhocViewChild *child, int *sx, int *sy)
{
  struct wlr_surface *wlr_surface;
  struct wlr_subsurface *wlr_subsurface;
  PhocViewChild *parent;

  wlr_surface = phoc_view_child_get_wlr_surface (child);
  parent = phoc_view_child_get_parent (child);
  if (parent)
    phoc_view_child_get_pos (parent, sx, sy);
  else
    *sx = *sy = 0;

  wlr_subsurface = wlr_subsurface_try_from_wlr_surface (wlr_surface);
  g_assert (wlr_subsurface);
  *sx += wlr_subsurface->current.x;
  *sy += wlr_subsurface->current.y;
}


static void
subsurface_handle_destroy (struct wl_listener *listener, void *data)
{
  PhocSubsurface *self = wl_container_of (listener, self, destroy);

  g_object_unref (self);
}


static void
collect_damage_iter (struct wlr_surface *wlr_surface, int sx, int sy, gpointer data)
{
  PhocSubsurface *self = PHOC_SUBSURFACE (data);
  struct wlr_subsurface *wlr_subsurface = self->wlr_subsurface;
  PhocSurface *surface = PHOC_SURFACE (wlr_subsurface->surface->data);
  int current_x, current_y;

  if (!surface)
    return;

  g_assert (PHOC_IS_SURFACE (surface));
  phoc_view_child_get_pos (PHOC_VIEW_CHILD (self), &current_x, &current_y);

  phoc_surface_add_damage_box (surface, &(struct wlr_box) {
      /* We add the damage to the subsurface but `previous` is relative
       * to the root surface, so subtract its offset */
      self->previous.x - current_x,
      self->previous.y - current_y,
      wlr_surface->current.width,
      wlr_surface->current.height
    });
}


static void
handle_parent_commit (struct wl_listener *listener, void *data)
{
  PhocSubsurface *self = wl_container_of (listener, self, parent_commit);
  struct wlr_subsurface *wlr_subsurface = self->wlr_subsurface;
  struct wlr_surface *wlr_surface = wlr_subsurface->surface;
  gboolean moved, reordered;
  int sx, sy;

  phoc_view_child_get_pos (PHOC_VIEW_CHILD (self), &sx, &sy);

  moved = (self->previous.x != sx || self->previous.y != sy);

  reordered = (self->previous.prev != wlr_subsurface->current.link.prev ||
               self->previous.next != wlr_subsurface->current.link.next);

  if (wlr_subsurface->surface->mapped && (moved || reordered))
    wlr_surface_for_each_surface (wlr_surface, collect_damage_iter, self);

  self->previous.x = sx;
  self->previous.y = sy;
  self->previous.prev = wlr_subsurface->current.link.prev;
  self->previous.next = wlr_subsurface->current.link.next;

  if (wlr_subsurface->surface->mapped && (moved || reordered)) {
    wlr_surface_for_each_surface (wlr_surface, collect_damage_iter, self);
    phoc_view_child_apply_damage (PHOC_VIEW_CHILD (self));
  }
}


static void
set_wlr_subsurface (PhocSubsurface *self, struct wlr_subsurface *subsurface)
{
  g_assert (self->wlr_subsurface == NULL);

  self->wlr_subsurface = subsurface;
  g_debug ("New surface %p", self->wlr_subsurface);

  PHOC_VIEW_CHILD (self)->mapped = self->wlr_subsurface->surface->mapped;

  self->destroy.notify = subsurface_handle_destroy;
  wl_signal_add (&self->wlr_subsurface->events.destroy, &self->destroy);

  self->parent_commit.notify = handle_parent_commit;
  wl_signal_add (&self->wlr_subsurface->parent->events.commit, &self->parent_commit);
}


static void
phoc_subsurface_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  PhocSubsurface *self = PHOC_SUBSURFACE (object);

  switch (property_id) {
  case PROP_WLR_SUBSURFACE:
    set_wlr_subsurface (self, g_value_get_pointer (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_subsurface_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PhocSubsurface *self = PHOC_SUBSURFACE (object);

  switch (property_id) {
  case PROP_WLR_SUBSURFACE:
    g_value_set_pointer (value, self->wlr_subsurface);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_subsurface_finalize (GObject *object)
{
  PhocSubsurface *self = PHOC_SUBSURFACE (object);

  wl_list_remove (&self->parent_commit.link);
  wl_list_remove (&self->destroy.link);

  self->wlr_subsurface = NULL;

  G_OBJECT_CLASS (phoc_subsurface_parent_class)->finalize (object);
}


static void
phoc_subsurface_class_init (PhocSubsurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhocViewChildClass *view_child_class = PHOC_VIEW_CHILD_CLASS (klass);

  object_class->finalize = phoc_subsurface_finalize;
  object_class->get_property = phoc_subsurface_get_property;
  object_class->set_property = phoc_subsurface_set_property;

  view_child_class->get_pos = subsurface_get_pos;

  props[PROP_WLR_SUBSURFACE] =
    g_param_spec_pointer ("wlr-subsurface", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_subsurface_init (PhocSubsurface *self)
{
  self->previous.x = -1;
  self->previous.y = -1;
}


PhocSubsurface *
phoc_subsurface_new (PhocView *view, struct wlr_subsurface *wlr_subsurface)
{
  return g_object_new (PHOC_TYPE_SUBSURFACE,
                       "view", view,
                       "wlr-surface", wlr_subsurface->surface,
                       "wlr-subsurface", wlr_subsurface,
                       NULL);
}
