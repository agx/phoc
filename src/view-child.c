/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-view-child"

#include "phoc-config.h"

#include "child-root.h"
#include "desktop.h"
#include "input.h"
#include "output.h"
#include "server.h"
#include "subsurface.h"
#include "utils.h"
#include "view-private.h"
#include "view-child-private.h"

/**
 * PhocViewChild:
 * @view: The [type@PhocView] this child belongs to
 * @parent: (nullable): The parent of this child if another child
 * @children: (nullable): children of this child
 *
 * A child of a [type@View], e.g. a [type@XdgPopup] or subsurface.
 *
 * Since `PhocViewChild`ren are created from toplevels or other
 * children they add themselves to the toplevels list of children upon
 * construction and remove themselves when their last ref is dropped
 * (either via subsurface / popup's `handle_destroy` or when the view
 * disposes frees the list of its children).
 */

enum {
  PROP_0,
  PROP_CHILD_ROOT,
  PROP_WLR_SURFACE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _PhocViewChildPrivate {
  PhocChildRoot                *root;
  PhocViewChild                *parent;
  GSList                       *children;
  struct wlr_surface           *wlr_surface;
  bool                          mapped;

  struct wl_listener            map;
  struct wl_listener            unmap;
  struct wl_listener            commit;
  struct wl_listener            new_subsurface;
} PhocViewChildPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhocViewChild, phoc_view_child, G_TYPE_OBJECT)

#define PHOC_VIEW_CHILD_SELF(p) PHOC_PRIV_CONTAINER(PHOC_VIEW_CHILD, PhocViewChild, (p))

static void
phoc_view_child_subsurface_create (PhocViewChild *self, struct wlr_subsurface *wlr_subsurface)
{
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);
  PhocSubsurface *subsurface = phoc_subsurface_new (priv->root, wlr_subsurface);
  PhocViewChildPrivate *subsurface_priv;

  subsurface_priv = phoc_view_child_get_instance_private (PHOC_VIEW_CHILD (subsurface));

  subsurface_priv->parent = self;
  priv->children = g_slist_prepend (priv->children, subsurface);

  phoc_view_child_damage_whole (PHOC_VIEW_CHILD (subsurface));
}


static void
phoc_view_child_init_subsurfaces (PhocViewChild *self, struct wlr_surface *surface)
{
  struct wlr_subsurface *subsurface;

  wl_list_for_each (subsurface, &surface->current.subsurfaces_below, current.link)
    phoc_view_child_subsurface_create (self, subsurface);

  wl_list_for_each (subsurface, &surface->current.subsurfaces_above, current.link)
    phoc_view_child_subsurface_create (self, subsurface);
}


static bool
phoc_view_child_is_mapped (PhocViewChild *self)
{
  while (self) {
    PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

    if (!priv->mapped)
      return false;

    self = priv->parent;
  }
  return true;
}


static void
phoc_view_child_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

  switch (property_id) {
  case PROP_CHILD_ROOT:
    /* TODO: Should hold a ref */
    priv->root = g_value_get_object (value);
    break;
  case PROP_WLR_SURFACE:
    priv->wlr_surface = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_view_child_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

  switch (property_id) {
  case PROP_CHILD_ROOT:
    g_value_set_object (value, priv->root);
    break;
  case PROP_WLR_SURFACE:
    g_value_set_pointer (value, priv->wlr_surface);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_view_child_handle_map (struct wl_listener *listener, void *data)
{
  PhocViewChildPrivate *priv = wl_container_of (listener, priv, map);
  PhocViewChild *self = PHOC_VIEW_CHILD_SELF (priv);

  PHOC_VIEW_CHILD_GET_CLASS (self)->map (self);
}


static void
phoc_view_child_handle_unmap (struct wl_listener *listener, void *data)
{
  PhocViewChildPrivate *priv = wl_container_of (listener, priv, unmap);
  PhocViewChild *self = PHOC_VIEW_CHILD_SELF (priv);

  PHOC_VIEW_CHILD_GET_CLASS (self)->unmap (self);
}


static void
phoc_view_child_handle_new_subsurface (struct wl_listener *listener, void *data)
{
  PhocViewChildPrivate *priv = wl_container_of (listener, priv, new_subsurface);
  PhocViewChild *self = PHOC_VIEW_CHILD_SELF (priv);
  struct wlr_subsurface *wlr_subsurface = data;

  phoc_view_child_subsurface_create (self, wlr_subsurface);
}


static void
phoc_view_child_handle_commit (struct wl_listener *listener, void *data)
{
  PhocViewChildPrivate *priv = wl_container_of (listener, priv, commit);
  PhocViewChild *self = PHOC_VIEW_CHILD_SELF (priv);

  phoc_view_child_apply_damage (self);
}


static void
phoc_view_child_constructed (GObject *object)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

  G_OBJECT_CLASS (phoc_view_child_parent_class)->constructed (object);

  priv->map.notify = phoc_view_child_handle_map;
  wl_signal_add (&priv->wlr_surface->events.map, &priv->map);

  priv->unmap.notify = phoc_view_child_handle_unmap;
  wl_signal_add (&priv->wlr_surface->events.unmap, &priv->unmap);

  priv->commit.notify = phoc_view_child_handle_commit;
  wl_signal_add (&priv->wlr_surface->events.commit, &priv->commit);

  priv->new_subsurface.notify = phoc_view_child_handle_new_subsurface;
  wl_signal_add (&priv->wlr_surface->events.new_subsurface, &priv->new_subsurface);

  phoc_child_root_add_child (priv->root, self);

  phoc_view_child_init_subsurfaces (self, priv->wlr_surface);
}


static void
phoc_view_child_remove_child (PhocViewChild *self, PhocViewChild *child)
{
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

  priv->children = g_slist_remove (priv->children, child);
}


static void
phoc_view_child_finalize (GObject *object)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

  if (phoc_view_child_is_mapped (self) && phoc_child_root_is_mapped (priv->root))
    phoc_view_child_damage_whole (self);

  /* Remove from parent if it's also a PhocViewChild */
  if (priv->parent != NULL) {
    phoc_view_child_remove_child (priv->parent, self);
    priv->parent = NULL;
  }

  /* Detach us from all children */
  for (GSList *elem = priv->children; elem; elem = elem->next) {
    PhocViewChild *subchild = elem->data;
    PhocViewChildPrivate *subchild_priv = phoc_view_child_get_instance_private (subchild);

    subchild_priv->parent = NULL;
    /* The subchild lost its parent, so it cannot see that the parent is unmapped.
     * Unmap it directly */
    /* TODO: But then we won't damage them on destroy? */
    subchild_priv->mapped = false;
  }
  g_clear_pointer (&priv->children, g_slist_free);

  phoc_child_root_remove_child (priv->root, self);

  wl_list_remove (&priv->map.link);
  wl_list_remove (&priv->unmap.link);
  wl_list_remove (&priv->commit.link);
  wl_list_remove (&priv->new_subsurface.link);

  priv->root = NULL;
  priv->wlr_surface = NULL;

  G_OBJECT_CLASS (phoc_view_child_parent_class)->finalize (object);
}


static void
phoc_view_child_map_default (PhocViewChild *self)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocInput *input = phoc_server_get_input (phoc_server_get_default ());
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);
  PhocChildRoot *root = priv->root;

  priv->mapped = true;
  phoc_view_child_damage_whole (self);

  struct wlr_box box;
  phoc_child_root_get_box (root, &box);

  PhocOutput *output;
  wl_list_for_each (output, &desktop->outputs, link) {
    bool intersects;

    intersects = wlr_output_layout_intersects (desktop->layout, output->wlr_output, &box);
    if (intersects)
      phoc_utils_wlr_surface_enter_output (priv->wlr_surface, output->wlr_output);
  }

  phoc_input_update_cursor_focus (input);
}


static void
phoc_view_child_unmap_default (PhocViewChild *self)
{
  PhocInput *input = phoc_server_get_input (phoc_server_get_default ());
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);

  phoc_view_child_damage_whole (self);
  phoc_input_update_cursor_focus (input);
  priv->mapped = false;
}


G_NORETURN
static void
phoc_view_child_get_pos_default (PhocViewChild *self, int *sx, int *sy)
{
  g_assert_not_reached ();
}


static void
phoc_view_child_class_init (PhocViewChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhocViewChildClass *view_child_class = PHOC_VIEW_CHILD_CLASS (klass);

  object_class->get_property = phoc_view_child_get_property;
  object_class->set_property = phoc_view_child_set_property;
  object_class->constructed = phoc_view_child_constructed;
  object_class->finalize = phoc_view_child_finalize;

  view_child_class->map = phoc_view_child_map_default;
  view_child_class->unmap = phoc_view_child_unmap_default;
  view_child_class->get_pos = phoc_view_child_get_pos_default;

  props[PROP_CHILD_ROOT] =
    g_param_spec_object ("child-root", "", "",
                         PHOC_TYPE_CHILD_ROOT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_WLR_SURFACE] =
    g_param_spec_pointer ("wlr-surface", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_view_child_init (PhocViewChild *self)
{
}

/**
 * phoc_view_child_apply_damage:
 * @self: A view child
 *
 * This is the equivalent of `phoc_view_apply_damage` but for [type@ViewChild].
 */
void
phoc_view_child_apply_damage (PhocViewChild *self)
{
  PhocViewChildPrivate *priv;

  g_assert (PHOC_IS_VIEW_CHILD (self));
  priv = phoc_view_child_get_instance_private (self);

  if (!self || !phoc_view_child_is_mapped (self) || !phoc_child_root_is_mapped (priv->root))
    return;

  phoc_child_root_apply_damage (priv->root);
}

/**
 * phoc_view_child_damage_whole:
 * @self: A view child
 *
 * This is the equivalent of [method@View.damage_whole] but for
 * [type@ViewChild].
 */
void
phoc_view_child_damage_whole (PhocViewChild *self)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocViewChildPrivate *priv = phoc_view_child_get_instance_private (self);
  PhocOutput *output;
  int sx, sy;
  struct wlr_box root_box;

  if (!self || !phoc_view_child_is_mapped (self) || !phoc_child_root_is_mapped (priv->root))
    return;

  phoc_child_root_get_box (priv->root, &root_box);
  phoc_view_child_get_pos (self, &sx, &sy);

  wl_list_for_each (output, &desktop->outputs, link) {
    struct wlr_box output_box;
    wlr_output_layout_get_box (desktop->layout, output->wlr_output, &output_box);
    phoc_output_damage_whole_surface (output,
                                      priv->wlr_surface,
                                      root_box.x + sx - output_box.x,
                                      root_box.y + sy - output_box.y);
  }
}


void
phoc_view_child_get_pos (PhocViewChild *self, int *sx, int *sy)
{
  g_assert (PHOC_IS_VIEW_CHILD (self));

  PHOC_VIEW_CHILD_GET_CLASS (self)->get_pos (self, sx, sy);
}

/**
 * phoc_view_child_get_root:
 * @self: A view child
 *
 * Get the root of the tree this child belongs to.
 *
 * Returns: (transfer none): The root
 */
PhocChildRoot *
phoc_view_child_get_root (PhocViewChild *self)
{
  PhocViewChildPrivate *priv;

  g_assert (PHOC_IS_VIEW_CHILD (self));
  priv = phoc_view_child_get_instance_private (self);

  return priv->root;
}

/**
 * phoc_view_child_get_parent:
 * @self: A view child
 *
 * Get the view's parent (if any). The parent is either a `PhocViewChild`
 * or `NULL` (in that case `self` is a direct child of it's `view`).
 *
 * Returns: (transfer none)(nullable): The view child
 */
PhocViewChild *
phoc_view_child_get_parent (PhocViewChild *self)
{
  PhocViewChildPrivate *priv;

  g_assert (PHOC_IS_VIEW_CHILD (self));
  priv = phoc_view_child_get_instance_private (self);

  return priv->parent;
}

/**
 * phoc_view_child_get_wlr_surface:
 * @self: A view child
 *
 * Get the `wlr_surface` associated with this view child.
 *
 * Returns: (transfer none): The `wlr_surface`
 */
struct wlr_surface *
phoc_view_child_get_wlr_surface (PhocViewChild *self)
{
  PhocViewChildPrivate *priv;

  g_assert (PHOC_IS_VIEW_CHILD (self));
  priv = phoc_view_child_get_instance_private (self);

  return priv->wlr_surface;
}

/**
 * phoc_view_child_set_mapped:
 * @self: A view child
 *
 * Sets whether the view child is currently mapped
 */
void
phoc_view_child_set_mapped (PhocViewChild *self, bool mapped)
{
  PhocViewChildPrivate *priv;

  g_assert (PHOC_IS_VIEW_CHILD (self));
  priv = phoc_view_child_get_instance_private (self);

  priv->mapped = mapped;
}
