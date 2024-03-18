/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-xdg-popup"

#include "phoc-config.h"

#include <stdbool.h>
#include <stdlib.h>
#include <wlr/types/wlr_xdg_shell.h>
#include "cursor.h"
#include "desktop.h"
#include "input.h"
#include "server.h"
#include "view.h"
#include "view-child-private.h"
#include "utils.h"
#include "xdg-popup.h"

enum {
  PROP_0,
  PROP_WLR_POPUP,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhocXdgPopup:
 *
 * A popup as defined in the xdg-shell protocol
 */
typedef struct _PhocXdgPopup {
  PhocViewChild         parent_instance;

  struct wlr_xdg_popup *wlr_popup;

  struct wl_listener    destroy;
  struct wl_listener    new_popup;
  struct wl_listener    reposition;
} PhocXdgPopup;

G_DEFINE_FINAL_TYPE (PhocXdgPopup, phoc_xdg_popup, PHOC_TYPE_VIEW_CHILD)


static void
popup_get_pos (PhocViewChild *child, int *sx, int *sy)
{
  PhocXdgPopup *popup = PHOC_XDG_POPUP (child);
  struct wlr_xdg_popup *wlr_popup = popup->wlr_popup;

  wlr_xdg_popup_get_toplevel_coords (wlr_popup,
                                     wlr_popup->current.geometry.x - wlr_popup->base->current.geometry.x,
                                     wlr_popup->current.geometry.y - wlr_popup->base->current.geometry.y,
                                     sx, sy);
}


static void
popup_unconstrain (PhocXdgPopup* popup)
{
  /* get the output of the popup's positioner anchor point and convert it to
   * the toplevel parent's coordinate system and then pass it to
   * wlr_xdg_popup_unconstrain_from_box */
  PhocView *view = PHOC_VIEW_CHILD (popup)->view;

  PhocOutput *output = phoc_desktop_layout_get_output (view->desktop, view->box.x, view->box.y);
  if (output == NULL)
    return;

  struct wlr_box output_box;
  wlr_output_layout_get_box (view->desktop->layout, output->wlr_output, &output_box);
  struct wlr_box usable_area = output->usable_area;
  usable_area.x += output_box.x;
  usable_area.y += output_box.y;

  /* the output box expressed in the coordinate system of the toplevel parent
   * of the popup */
  struct wlr_box output_toplevel_sx_box = {
    .x = usable_area.x - view->box.x,
    .y = usable_area.y - view->box.y,
    .width = usable_area.width,
    .height = usable_area.height,
  };

  wlr_xdg_popup_unconstrain_from_box (popup->wlr_popup, &output_toplevel_sx_box);
}


static void
popup_handle_destroy (struct wl_listener *listener, void *data)
{
  PhocXdgPopup *popup = wl_container_of (listener, popup, destroy);

  g_object_unref (popup);
}


static void
popup_handle_new_popup (struct wl_listener *listener, void *data)
{
  PhocXdgPopup *popup = wl_container_of (listener, popup, new_popup);
  struct wlr_xdg_popup *wlr_popup = data;

  phoc_xdg_popup_new (PHOC_VIEW_CHILD (popup)->view, wlr_popup);
}


static void
popup_handle_reposition (struct wl_listener *listener, void *data)
{
  PhocXdgPopup *popup = wl_container_of (listener, popup, reposition);

  /* clear the old popup positon */
  /* TODO: this is too much damage */
  phoc_view_damage_whole (PHOC_VIEW_CHILD (popup)->view);

  popup_unconstrain (popup);
}


static void
phoc_xdg_popup_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PhocXdgPopup *popup = PHOC_XDG_POPUP (object);

  switch (property_id) {
  case PROP_WLR_POPUP:
    popup->wlr_popup = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_xdg_popup_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhocXdgPopup *popup = PHOC_XDG_POPUP (object);

  switch (property_id) {
  case PROP_WLR_POPUP:
    g_value_set_pointer (value, popup->wlr_popup);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_xdg_popup_constructed (GObject *object)
{
  PhocXdgPopup *popup = PHOC_XDG_POPUP (object);

  G_OBJECT_CLASS (phoc_xdg_popup_parent_class)->constructed (object);

  phoc_view_child_setup (PHOC_VIEW_CHILD (popup));

  popup->destroy.notify = popup_handle_destroy;
  wl_signal_add (&popup->wlr_popup->base->events.destroy, &popup->destroy);

  popup->new_popup.notify = popup_handle_new_popup;
  wl_signal_add (&popup->wlr_popup->base->events.new_popup, &popup->new_popup);

  popup->reposition.notify = popup_handle_reposition;
  wl_signal_add (&popup->wlr_popup->events.reposition, &popup->reposition);

  popup_unconstrain (popup);
}


static void
phoc_xdg_popup_finalize (GObject *object)
{
  PhocXdgPopup *popup = PHOC_XDG_POPUP (object);

  wl_list_remove (&popup->reposition.link);
  wl_list_remove (&popup->new_popup.link);
  wl_list_remove (&popup->destroy.link);

  G_OBJECT_CLASS (phoc_xdg_popup_parent_class)->finalize (object);
}


static void
phoc_xdg_popup_class_init (PhocXdgPopupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhocViewChildClass *view_child_class = PHOC_VIEW_CHILD_CLASS (klass);

  object_class->constructed = phoc_xdg_popup_constructed;
  object_class->finalize = phoc_xdg_popup_finalize;
  object_class->get_property = phoc_xdg_popup_get_property;
  object_class->set_property = phoc_xdg_popup_set_property;

  view_child_class->get_pos = popup_get_pos;

  props[PROP_WLR_POPUP] =
    g_param_spec_pointer ("wlr-popup", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_xdg_popup_init (PhocXdgPopup *self)
{
}


PhocXdgPopup *
phoc_xdg_popup_new (PhocView *view, struct wlr_xdg_popup *wlr_xdg_popup)
{
  return g_object_new (PHOC_TYPE_XDG_POPUP,
                       "view", view,
                       "wlr-popup", wlr_xdg_popup,
                       "wlr-surface", wlr_xdg_popup->base->surface,
                       NULL);
}
