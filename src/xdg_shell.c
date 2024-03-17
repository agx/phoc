#define G_LOG_DOMAIN "phoc-xdg-shell"

#include "phoc-config.h"
#include "xdg-surface.h"
#include "xdg-surface-private.h"

#include <stdbool.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include "cursor.h"
#include "desktop.h"
#include "input.h"
#include "server.h"
#include "view.h"
#include "view-child-private.h"
#include "utils.h"


enum {
  PROP_0,
  PROP_WLR_POPUP,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


typedef struct _PhocXdgToplevelDecoration {
  struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration;
  PhocXdgSurface *surface;
  struct wl_listener destroy;
  struct wl_listener request_mode;
  struct wl_listener surface_commit;
} PhocXdgToplevelDecoration;


typedef struct _PhocXdgPopup {
  PhocViewChild     parent_instance;

  struct wlr_xdg_popup *wlr_popup;

  struct wl_listener destroy;
  struct wl_listener new_popup;
  struct wl_listener reposition;
} PhocXdgPopup;

#define PHOC_TYPE_XDG_POPUP (phoc_xdg_popup_get_type ())
G_DECLARE_FINAL_TYPE (PhocXdgPopup, phoc_xdg_popup, PHOC, XDG_POPUP, PhocViewChild)
G_DEFINE_FINAL_TYPE (PhocXdgPopup, phoc_xdg_popup, PHOC_TYPE_VIEW_CHILD)


static void
popup_get_pos (PhocViewChild *child, int *sx, int *sy)
{
  PhocXdgPopup *popup = (PhocXdgPopup *)child;
  struct wlr_xdg_popup *wlr_popup = popup->wlr_popup;

  wlr_xdg_popup_get_toplevel_coords (wlr_popup,
                                     wlr_popup->current.geometry.x - wlr_popup->base->current.geometry.x,
                                     wlr_popup->current.geometry.y - wlr_popup->base->current.geometry.y,
                                     sx, sy);
}


static void
popup_unconstrain (PhocXdgPopup* popup)
{
  // get the output of the popup's positioner anchor point and convert it to
  // the toplevel parent's coordinate system and then pass it to
  // wlr_xdg_popup_unconstrain_from_box
  PhocView *view = PHOC_VIEW (PHOC_VIEW_CHILD (popup)->view);

  PhocOutput *output = phoc_desktop_layout_get_output (view->desktop, view->box.x, view->box.y);
  if (output == NULL)
    return;

  struct wlr_box output_box;
  wlr_output_layout_get_box (view->desktop->layout, output->wlr_output, &output_box);
  struct wlr_box usable_area = output->usable_area;
  usable_area.x += output_box.x;
  usable_area.y += output_box.y;

  // the output box expressed in the coordinate system of the toplevel parent
  // of the popup
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


void
handle_xdg_shell_surface (struct wl_listener *listener, void *data)
{
  struct wlr_xdg_surface *surface = data;

  g_assert (surface->role != WLR_XDG_SURFACE_ROLE_NONE);
  if (surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
    g_debug ("new xdg popup");
    return;
  }

  PhocDesktop *desktop = wl_container_of(listener, desktop, xdg_shell_surface);
  g_debug ("new xdg toplevel: title=%s, app_id=%s",
           surface->toplevel->title, surface->toplevel->app_id);

  wlr_xdg_surface_ping (surface);
  PhocXdgSurface *phoc_surface = phoc_xdg_surface_new (surface);

  // Check for app-id override coming from gtk-shell
  PhocGtkShell *gtk_shell = phoc_desktop_get_gtk_shell (desktop);
  PhocGtkSurface *gtk_surface = phoc_gtk_shell_get_gtk_surface_from_wlr_surface (gtk_shell,
                                                                                 surface->surface);
  if (gtk_surface && phoc_gtk_surface_get_app_id (gtk_surface))
    phoc_view_set_app_id (PHOC_VIEW (phoc_surface), phoc_gtk_surface_get_app_id (gtk_surface));
  else
    phoc_view_set_app_id (PHOC_VIEW (phoc_surface), surface->toplevel->app_id);
}


static void
decoration_handle_destroy (struct wl_listener *listener, void *data)
{
  PhocXdgToplevelDecoration *decoration = wl_container_of (listener, decoration, destroy);

  g_debug ("Destroy xdg toplevel decoration %p", decoration);

  if (decoration->surface) {
    phoc_xdg_surface_set_decoration (decoration->surface, NULL);
    phoc_view_set_decorated (PHOC_VIEW (decoration->surface), FALSE);
    g_signal_handlers_disconnect_by_data (decoration->surface, decoration);
  }
  wl_list_remove (&decoration->destroy.link);
  wl_list_remove (&decoration->request_mode.link);
  wl_list_remove (&decoration->surface_commit.link);

  free (decoration);
}

static void
decoration_handle_request_mode (struct wl_listener *listener, void *data)
{
  PhocXdgToplevelDecoration *decoration = wl_container_of (listener, decoration, request_mode);

  enum wlr_xdg_toplevel_decoration_v1_mode mode = decoration->wlr_decoration->requested_mode;

  if (mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE) {
    mode = WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;
  }
  wlr_xdg_toplevel_decoration_v1_set_mode (decoration->wlr_decoration, mode);
}

static void
decoration_handle_surface_commit (struct wl_listener *listener, void *data)
{
  PhocXdgToplevelDecoration *decoration = wl_container_of (listener, decoration, surface_commit);

  bool decorated = decoration->wlr_decoration->current.mode ==
    WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
  phoc_view_set_decorated (PHOC_VIEW (decoration->surface), decorated);
}


static void
on_xdg_surface_destroy (PhocXdgSurface *surface, PhocXdgToplevelDecoration *decoration)
{
  g_assert (PHOC_IS_XDG_SURFACE (surface));

  decoration->surface = NULL;
}


void
handle_xdg_toplevel_decoration (struct wl_listener *listener, void *data)
{
  struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration = data;
  PhocXdgSurface *xdg_surface = PHOC_XDG_SURFACE (wlr_decoration->toplevel->base->data);
  g_assert (xdg_surface != NULL);
  struct wlr_xdg_surface *wlr_xdg_surface = phoc_xdg_surface_get_wlr_xdg_surface (xdg_surface);
  PhocXdgToplevelDecoration *decoration = g_new0 (PhocXdgToplevelDecoration, 1);

  g_debug ("New xdg toplevel decoration %p", decoration);

  decoration->wlr_decoration = wlr_decoration;
  decoration->surface = xdg_surface;
  phoc_xdg_surface_set_decoration (xdg_surface, decoration);

  decoration->destroy.notify = decoration_handle_destroy;
  wl_signal_add (&wlr_decoration->events.destroy, &decoration->destroy);

  decoration->request_mode.notify = decoration_handle_request_mode;
  wl_signal_add (&wlr_decoration->events.request_mode, &decoration->request_mode);

  decoration->surface_commit.notify = decoration_handle_surface_commit;
  wl_signal_add (&wlr_xdg_surface->surface->events.commit, &decoration->surface_commit);

  g_signal_connect (xdg_surface, "surface-destroy", G_CALLBACK (on_xdg_surface_destroy), decoration);

  decoration_handle_request_mode (&decoration->request_mode, wlr_decoration);
}
