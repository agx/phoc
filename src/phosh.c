/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-phosh"

#include "config.h"

#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>
#include <phosh-private-protocol.h>
#include "server.h"
#include "desktop.h"
#include "phosh.h"

#define PHOSH_PRIVATE_VERSION 3

static
void phosh_rotate_display(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *surface_resource,
			  uint32_t degrees) {
  struct phosh_private *phosh = wl_resource_get_user_data(resource);
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;

  g_debug ("rotation: %d", degrees);
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
  default:
    wl_resource_post_error(resource,
			   PHOSH_PRIVATE_ERROR_INVALID_ARGUMENT,
			   "Can only rotate in 90 degree steps");
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
  int version = wl_resource_get_version(phosh_private_resource);
  struct wl_resource *resource  = wl_resource_create(client, &phosh_private_xdg_switcher_interface,
						     version, id);

  wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			 "Use wlr-toplevel-management protocol instead");
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

