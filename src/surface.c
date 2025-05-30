/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-surface"

#include "phoc-config.h"

#include "surface.h"

/**
 * PhocSurface:
 *
 * A Wayland wl_surface backed by a wlr_surface
 */

enum {
  PROP_0,
  PROP_WLR_SURFACE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocSurface {
  GObject             parent;

  struct wlr_surface *wlr_surface;
  pixman_region32_t   damage;

  struct wl_listener  commit;
  struct wl_listener  destroy;
};
G_DEFINE_TYPE (PhocSurface, phoc_surface, G_TYPE_OBJECT)


static void
handle_commit (struct wl_listener *listener, void *data)
{
  PhocSurface *self = wl_container_of (listener, self, commit);
  struct wlr_surface *wlr_surface = self->wlr_surface;

  if (wlr_surface->WLR_PRIVATE.previous.width == wlr_surface->current.width &&
      wlr_surface->WLR_PRIVATE.previous.height == wlr_surface->current.height &&
      wlr_surface->current.dx == 0 && wlr_surface->current.dy ==  0)
    return;

  /* Damage surface size or contents offset changed */
  pixman_region32_union_rect (&self->damage,
                              &self->damage,
                              -wlr_surface->current.dx,
                              -wlr_surface->current.dy,
                              wlr_surface->WLR_PRIVATE.previous.width,
                              wlr_surface->WLR_PRIVATE.previous.height);
}


static void
handle_destroy (struct wl_listener *listener, void *data)
{
  PhocSurface *self = wl_container_of (listener, self, destroy);

  g_debug ("Surface %p destroyed", self->wlr_surface);

  g_object_unref (self);
}


static void
set_wlr_surface (PhocSurface *self, struct wlr_surface *wlr_surface)
{
  g_assert (self->wlr_surface == NULL);

  self->wlr_surface = wlr_surface;
  g_debug ("New surface %p", self->wlr_surface);
  self->wlr_surface->data = self;

  self->commit.notify = handle_commit;
  wl_signal_add (&self->wlr_surface->events.commit, &self->commit);

  self->destroy.notify = handle_destroy;
  wl_signal_add (&self->wlr_surface->events.destroy, &self->destroy);
}


static void
phoc_surface_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PhocSurface *self = PHOC_SURFACE (object);

  switch (property_id) {
  case PROP_WLR_SURFACE:
    set_wlr_surface (self, g_value_get_pointer (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_surface_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PhocSurface *self = PHOC_SURFACE (object);

  switch (property_id) {
  case PROP_WLR_SURFACE:
    g_value_set_pointer (value, self->wlr_surface);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_surface_finalize (GObject *object)
{
  PhocSurface *self = PHOC_SURFACE (object);

  pixman_region32_fini (&self->damage);

  wl_list_remove (&self->commit.link);
  wl_list_remove (&self->destroy.link);

  self->wlr_surface = NULL;

  G_OBJECT_CLASS (phoc_surface_parent_class)->finalize (object);
}


static void
phoc_surface_class_init (PhocSurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_surface_get_property;
  object_class->set_property = phoc_surface_set_property;
  object_class->finalize = phoc_surface_finalize;

  props[PROP_WLR_SURFACE] =
    g_param_spec_pointer ("wlr-surface", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_surface_init (PhocSurface *self)
{
  pixman_region32_init (&self->damage);
}


PhocSurface *
phoc_surface_new (struct wlr_surface *surface)
{
  return g_object_new (PHOC_TYPE_SURFACE, "wlr-surface", surface, NULL);
}


const pixman_region32_t *
phoc_surface_get_damage (PhocSurface *self)
{
  g_assert (PHOC_IS_SURFACE (self));

  return &self->damage;
}


void
phoc_surface_clear_damage (PhocSurface *self)
{
  g_assert (PHOC_IS_SURFACE (self));

  pixman_region32_clear (&self->damage);
}

/**
 * phoc_surface_add_damage:
 * @self: The to be damaged surface
 * @damage: The region in surface local coordinates
 *
 * Adds the given region to the surface's damaged region. The damage
 * will be collected by the `phoc_*_apply_damage()` functions. The
 * damaged region is not clipped to the surface.
 *
 * This is used to add damage triggered by e.g. surface moves or
 * unmaps (as opposed to buffer damage submitted by the client).
 */
void
phoc_surface_add_damage (PhocSurface *self, pixman_region32_t *damage)
{
  g_assert (PHOC_IS_SURFACE (self));

  pixman_region32_union (&self->damage, &self->damage, damage);
}

/**
 * phoc_surface_add_damage_box:
 * @self: The to be damaged surface
 * @box: The box in surface local coordinates
 *
 * Convenience function to add a box to the surface's damaged region.
 * See [method@Surface.add_damage] for details.
 */
void
phoc_surface_add_damage_box (PhocSurface *self, struct wlr_box *box)
{
  pixman_region32_t damage;

  g_assert (PHOC_IS_SURFACE (self));

  pixman_region32_init_rect (&damage, box->x, box->y, box->width, box->height);
  phoc_surface_add_damage (self, &damage);
  pixman_region32_fini (&damage);
}
