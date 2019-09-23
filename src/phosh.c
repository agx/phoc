/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-phosh"

#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server.h>
#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>
#include <phosh-private-protocol.h>
#include "config.h"
#include "server.h"
#include "desktop.h"
#include "phosh.h"

#define PHOSH_PRIVATE_VERSION 3

static void
xdg_switcher_handle_list_xdg_surfaces(struct wl_client *client,
				      struct wl_resource *resource)
{
  struct phosh_private_xdg_switcher *xdg_switcher =
    phosh_private_xdg_switcher_from_resource(resource);
  struct phosh_private *phosh = xdg_switcher->phosh;
  PhocDesktop *desktop = phosh->desktop;
  struct roots_view *view;

  wl_list_for_each(view, &desktop->views, link) {
    const char *app_id = NULL;
    const char *title = NULL;

    switch (view->type) {
    case ROOTS_XDG_SHELL_VIEW: {
      struct roots_xdg_surface *xdg_surface =
	roots_xdg_surface_from_view(view);
      if (xdg_surface->xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
	continue;
      app_id = xdg_surface->xdg_surface->toplevel->app_id;
      title = xdg_surface->xdg_surface->toplevel->title;
      break;
    }
    case ROOTS_XDG_SHELL_V6_VIEW: {
      struct roots_xdg_surface_v6 *xdg_surface_v6 =
	roots_xdg_surface_v6_from_view(view);

      if (xdg_surface_v6->xdg_surface_v6->role != WLR_XDG_SURFACE_V6_ROLE_TOPLEVEL)
	continue;
      app_id = xdg_surface_v6->xdg_surface_v6->toplevel->app_id;
      title = xdg_surface_v6->xdg_surface_v6->toplevel->title;
      break;
    }
    default:
      /* other surface types would go here */
      break;
    }

    if (app_id) {
      phosh_private_xdg_switcher_send_xdg_surface (resource, app_id, title);
      app_id = NULL;
      title = NULL;
    }
  }
  phosh_private_xdg_switcher_send_list_xdg_surfaces_done (resource);
}


static void
xdg_switcher_handle_raise_xdg_surfaces(struct wl_client *client,
				       struct wl_resource *resource,
				       const char *app_id,
				       const char *title)
{
  struct phosh_private_xdg_switcher *xdg_switcher =
    phosh_private_xdg_switcher_from_resource(resource);
  struct phosh_private *phosh = xdg_switcher->phosh;
  PhocDesktop *desktop = phosh->desktop;
  struct roots_view *view, *found_view = NULL;
  struct roots_input *input = desktop->server->input;
  struct roots_seat *seat = input_last_active_seat(input);

  g_debug ("will raise view %s", app_id);
  wl_list_for_each(view, &desktop->views, link) {
    switch (view->type) {
    case ROOTS_XDG_SHELL_VIEW: {
      struct roots_xdg_surface *xdg_surface =
	roots_xdg_surface_from_view(view);

      if (xdg_surface->xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
	continue;
      if (!strcmp(app_id, xdg_surface->xdg_surface->toplevel->app_id) &&
	  !strcmp(title, xdg_surface->xdg_surface->toplevel->title))
	found_view = view;
      break;
    }
    case ROOTS_XDG_SHELL_V6_VIEW: {
      struct roots_xdg_surface_v6 *xdg_surface_v6 =
	roots_xdg_surface_v6_from_view(view);

      if (xdg_surface_v6->xdg_surface_v6->role != WLR_XDG_SURFACE_V6_ROLE_TOPLEVEL)
	continue;
      if (!strcmp(app_id, xdg_surface_v6->xdg_surface_v6->toplevel->app_id) &&
	  !strcmp(title, xdg_surface_v6->xdg_surface_v6->toplevel->title))
	found_view = view;
      break;
    }
    default:
      /* other surface types would go here */
      break;
    }
  }

  /* TODO: check if view belongs to this seat */
  if (found_view) {
    roots_seat_set_focus(seat, found_view);
  }
}

static void xdg_switcher_handle_close_xdg_surfaces(struct wl_client *client,
						   struct wl_resource *resource,
						   const char *app_id,
						   const char *title) {
  struct phosh_private_xdg_switcher *xdg_switcher =
    phosh_private_xdg_switcher_from_resource(resource);
  struct phosh_private *phosh = xdg_switcher->phosh;
  PhocDesktop *desktop = phosh->desktop;
  struct roots_view *view, *found_view;

  g_debug ("Will close view %s: %s", app_id, title ? :"");
  wl_list_for_each(view, &desktop->views, link) {
    found_view = NULL;

    switch (view->type) {
    case ROOTS_XDG_SHELL_VIEW: {
      struct roots_xdg_surface *xdg_surface =
	roots_xdg_surface_from_view(view);
      if (xdg_surface->xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
	continue;

      if (!g_strcmp0 (app_id, xdg_surface->xdg_surface->toplevel->app_id)) {
	if (title) {
	  if (!g_strcmp0 (title, xdg_surface->xdg_surface->toplevel->title))
	    found_view = view;
	} else {
	  found_view = view;
	}
      }
      break;
    }
    case ROOTS_XDG_SHELL_V6_VIEW: {
      struct roots_xdg_surface_v6 *xdg_surface_v6 =
	roots_xdg_surface_v6_from_view(view);

      if (xdg_surface_v6->xdg_surface_v6->role != WLR_XDG_SURFACE_V6_ROLE_TOPLEVEL)
	continue;
      if (!strcmp(app_id, xdg_surface_v6->xdg_surface_v6->toplevel->app_id)) {
	if (title) {
	  if (!strcmp(title, xdg_surface_v6->xdg_surface_v6->toplevel->title))
	    found_view = view;
	} else {
	  found_view = view;
	}
      }
      break;
    }
    default:
      /* other surface types would go here */
      break;
    }

    if (found_view) {
      view_close(found_view);
      /* we might have toplevels with same app_id and
	 title so close all of them */
    }
  }
}

static void
xdg_switcher_handle_destroy(struct wl_client *client,
			    struct wl_resource *resource)
{
  wl_resource_destroy(resource);
}


static void
xdg_switcher_handle_resource_destroy(struct wl_resource *resource)
{
  struct phosh_private_xdg_switcher *xdg_switcher =
    phosh_private_xdg_switcher_from_resource(resource);

  g_debug ("Destroying xdg_switcher %p (res %p)", xdg_switcher,
	   xdg_switcher->resource);
  wl_list_remove(&xdg_switcher->link);
  free(xdg_switcher);
}


static const struct phosh_private_xdg_switcher_interface phosh_private_xdg_switcher_impl = {
  .destroy = xdg_switcher_handle_destroy,
  .list_xdg_surfaces = xdg_switcher_handle_list_xdg_surfaces,
  .raise_xdg_surface = xdg_switcher_handle_raise_xdg_surfaces,
  .close_xdg_surface = xdg_switcher_handle_close_xdg_surfaces,
};


static
void phosh_rotate_display(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *surface_resource,
			  uint32_t degrees) {
  struct phosh_private *phosh = wl_resource_get_user_data(resource);
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;

  g_debug ("rotation: %d", degrees);
  if (degrees % 90 != 0) {
    wl_resource_post_error(resource,
			   PHOSH_PRIVATE_ERROR_INVALID_ARGUMENT,
			   "Can only rotate in 90 degree steps");
  }
  degrees %= 360;

  switch (degrees) {
  case 0:
    transform =  WL_OUTPUT_TRANSFORM_NORMAL;
    break;
  case 90:
    transform = WL_OUTPUT_TRANSFORM_90;
    break;
  case 180:
    transform = WL_OUTPUT_TRANSFORM_180;
    break;
  case 270:
    transform = WL_OUTPUT_TRANSFORM_270;
    break;
  }

  if (!phosh->panel) {
    g_warning ("Tried to rotate inexistent panel");
    return;
  }

  wlr_output_set_transform(phosh->panel->output, transform);
}


static void
handle_phosh_panel_surface_destroy (struct wl_listener *listener, void *data)
{
  struct phosh_private *phosh =
    wl_container_of(listener, phosh, listeners.panel_surface_destroy);

  if (phosh->panel) {
    phosh->panel = NULL;
    wl_list_remove(&phosh->listeners.panel_surface_destroy.link);
  }
}


static
void handle_phosh_layer_shell_new_surface(struct wl_listener *listener, void *data)
{
  struct wlr_layer_surface_v1 *surface = data;
  struct phosh_private *phosh =
    wl_container_of(listener, phosh, listeners.layer_shell_new_surface);

  /* We're only interested in the panel */
  if (strcmp(surface->namespace, "phosh"))
    return;

  phosh->panel = surface;
  wl_signal_add(&surface->events.destroy,
		&phosh->listeners.panel_surface_destroy);
  phosh->listeners.panel_surface_destroy.notify = handle_phosh_panel_surface_destroy;
}


static void
handle_get_xdg_switcher(struct wl_client *client,
			struct wl_resource *phosh_private_resource,
			uint32_t id)
{
  struct phosh_private *phosh =
    phosh_private_from_resource(phosh_private_resource);

  struct phosh_private_xdg_switcher *xdg_switcher =
    calloc(1, sizeof(struct phosh_private_xdg_switcher));
  if (xdg_switcher == NULL) {
    wl_client_post_no_memory(client);
    return;
  }

  int version = wl_resource_get_version(phosh_private_resource);
  xdg_switcher->resource = wl_resource_create(client,
					      &phosh_private_xdg_switcher_interface, version, id);
  if (xdg_switcher->resource == NULL) {
    free(xdg_switcher);
    wl_client_post_no_memory(client);
    return;
  }

  g_debug ("new phosh_private_xdg_switcher %p (res %p)", xdg_switcher,
	   xdg_switcher->resource);
  wl_resource_set_implementation(xdg_switcher->resource,
				 &phosh_private_xdg_switcher_impl, xdg_switcher, xdg_switcher_handle_resource_destroy);

  xdg_switcher->phosh = phosh;
  wl_signal_init(&xdg_switcher->events.destroy);
  wl_list_insert(&phosh->xdg_switchers, &xdg_switcher->link);
}



static void
phosh_handle_resource_destroy(struct wl_resource *resource)
{
  struct phosh_private *phosh = wl_resource_get_user_data(resource);

  phosh->resource = NULL;
  phosh->panel = NULL;
  g_debug ("Destroying phosh %p (res %p)", phosh, resource);
}


static const struct phosh_private_interface phosh_private_impl = {
  phosh_rotate_display,
  handle_get_xdg_switcher,
};


static void
phosh_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
  struct phosh_private *phosh = data;
  struct wl_resource *resource  = wl_resource_create(client, &phosh_private_interface,
						     version, id);

  if (phosh->resource) {
    wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			   "Only a single client can bind to phosh's private protocol");
    return;
  }

  /* FIXME: unsafe, needs client == shell->child.client */
  if (true) {
    g_warning ("FIXME: allowing every client to bind as phosh");
    wl_resource_set_implementation(resource,
				   &phosh_private_impl,
				   phosh, phosh_handle_resource_destroy);
    phosh->resource = resource;
    return;
  }

  wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			 "permission to bind phosh denied");
}


struct phosh_private*
phosh_create(PhocDesktop *desktop, struct wl_display *display)
{
  struct phosh_private *phosh = calloc(1, sizeof (struct phosh_private));
  if (!phosh)
    return NULL;

  phosh->desktop = desktop;

  wl_signal_add(&desktop->layer_shell->events.new_surface,
		&phosh->listeners.layer_shell_new_surface);
  phosh->listeners.layer_shell_new_surface.notify = handle_phosh_layer_shell_new_surface;
  wl_list_init(&phosh->xdg_switchers);

  g_info ("Initializing phosh private interface");
  phosh->global = wl_global_create(display, &phosh_private_interface, PHOSH_PRIVATE_VERSION, phosh, phosh_bind);

  if (!phosh->global) {
    return NULL;
  }

  return phosh;
}


void
phosh_destroy(struct phosh_private *phosh)
{
  wl_list_remove(&phosh->listeners.layer_shell_new_surface.link);
  wl_global_destroy(phosh->global);
}


struct phosh_private
*phosh_private_from_resource(struct wl_resource *resource)
{
  assert(wl_resource_instance_of(resource, &phosh_private_interface,
				 &phosh_private_impl));
  return wl_resource_get_user_data(resource);
}


struct phosh_private_xdg_switcher
*phosh_private_xdg_switcher_from_resource(struct wl_resource *resource) {
  assert(wl_resource_instance_of(resource, &phosh_private_xdg_switcher_interface,
				 &phosh_private_xdg_switcher_impl));
  return wl_resource_get_user_data(resource);
}
