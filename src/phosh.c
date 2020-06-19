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
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_matrix.h>
#include <phosh-private-protocol.h>
#include <wlr-screencopy-unstable-v1-protocol.h>
#include "server.h"
#include "desktop.h"
#include "phosh.h"
#include "render.h"

#define PHOSH_PRIVATE_VERSION 4

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
phosh_private_screencopy_frame_handle_resource_destroy (struct wl_resource *resource)
{
  struct phosh_private_screencopy_frame *frame =
    phosh_private_screencopy_frame_from_resource(resource);

  g_debug ("Destroying private_screencopy_frame %p (res %p)", frame, frame->resource);
  free(frame);
}

static void
thumbnail_view_handle_destroy (struct wl_listener *listener, void *data)
{
  struct phosh_private_screencopy_frame *frame =
    wl_container_of(listener, frame, view_destroy);
  frame->view = NULL;
}

static void
thumbnail_frame_handle_copy (struct wl_client *wl_client,
		struct wl_resource *frame_resource,
		struct wl_resource *buffer_resource)
{
  struct phosh_private_screencopy_frame *frame = phosh_private_screencopy_frame_from_resource(frame_resource);
  if (frame == NULL) {
    return;
  }

  if (!frame->view) {
    zwlr_screencopy_frame_v1_send_failed(frame->resource);
    return;
  }

  wl_list_remove(&frame->view_destroy.link);

  struct wl_shm_buffer *buffer = wl_shm_buffer_get(buffer_resource);
  if (buffer == NULL) {
    wl_resource_post_error(frame->resource,
      ZWLR_SCREENCOPY_FRAME_V1_ERROR_INVALID_BUFFER,
      "unsupported buffer type");
    return;
  }

  enum wl_shm_format fmt = wl_shm_buffer_get_format(buffer);
  int32_t width = wl_shm_buffer_get_width(buffer);
  int32_t height = wl_shm_buffer_get_height(buffer);
  int32_t stride = wl_shm_buffer_get_stride(buffer);
  if (fmt != frame->format || width != frame->width ||
      height != frame->height || stride != frame->stride) {
    wl_resource_post_error(frame->resource,
      ZWLR_SCREENCOPY_FRAME_V1_ERROR_INVALID_BUFFER,
      "invalid buffer attributes");
    return;
  }

  if (frame->buffer != NULL) {
    wl_resource_post_error(frame->resource,
      ZWLR_SCREENCOPY_FRAME_V1_ERROR_ALREADY_USED,
      "frame already used");
    return;
  }

  frame->buffer = buffer;

  wl_shm_buffer_begin_access(buffer);
  void *data = wl_shm_buffer_get_data(buffer);

  uint32_t flags = 0;
  view_render_to_buffer(frame->view, width, height, stride, &flags, data);

  wl_shm_buffer_end_access(buffer);

  zwlr_screencopy_frame_v1_send_flags(frame->resource, flags);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  uint32_t tv_sec_hi = (sizeof(now.tv_sec) > 4) ? now.tv_sec >> 32 : 0;
  uint32_t tv_sec_lo = now.tv_sec & 0xFFFFFFFF;
  zwlr_screencopy_frame_v1_send_ready(frame->resource, tv_sec_hi, tv_sec_lo, now.tv_nsec);
}

static void
thumbnail_frame_handle_copy_with_damage (struct wl_client *wl_client,
		struct wl_resource *frame_resource,
		struct wl_resource *buffer_resource)
{
  // XXX: unimplemented
  (void)wl_client;
  (void)buffer_resource;
  zwlr_screencopy_frame_v1_send_failed(frame_resource);
}

static void
thumbnail_frame_handle_destroy (struct wl_client *wl_client,
		struct wl_resource *frame_resource)
{
  wl_resource_destroy(frame_resource);
}

static const struct zwlr_screencopy_frame_v1_interface phosh_private_screencopy_frame_impl = {
  .copy = thumbnail_frame_handle_copy,
  .destroy = thumbnail_frame_handle_destroy,
  .copy_with_damage = thumbnail_frame_handle_copy_with_damage,
};

static void
handle_get_thumbnail (struct wl_client *client,
			struct wl_resource *phosh_private_resource,
			uint32_t id, struct wl_resource *toplevel, uint32_t max_width, uint32_t max_height)
{
  struct phosh_private_screencopy_frame *frame =
    calloc(1, sizeof(struct phosh_private_screencopy_frame));
  if (frame == NULL) {
    wl_client_post_no_memory(client);
    return;
  }

  int version = wl_resource_get_version(phosh_private_resource);
  frame->resource = wl_resource_create(client, &zwlr_screencopy_frame_v1_interface, version, id);
  if (frame->resource == NULL) {
    free(frame);
    wl_client_post_no_memory(client);
    return;
  }

  g_debug ("new phosh_private_screencopy_frame %p (res %p)", frame, frame->resource);
  wl_resource_set_implementation(frame->resource,
    &phosh_private_screencopy_frame_impl, frame, phosh_private_screencopy_frame_handle_resource_destroy);

  struct wlr_foreign_toplevel_handle_v1 *toplevel_handle = wl_resource_get_user_data(toplevel);
  if (!toplevel_handle) {
    zwlr_screencopy_frame_v1_send_failed(frame->resource);
    return;
  }

  struct roots_view *view = toplevel_handle->data;
  if (!view) {
    zwlr_screencopy_frame_v1_send_failed(frame->resource);
    return;
  }

  frame->toplevel = toplevel;
  frame->view = view;

  wl_signal_add(&frame->view->events.destroy, &frame->view_destroy);
  frame->view_destroy.notify = thumbnail_view_handle_destroy;

  // We hold to the current surface size even though it may change before
  // the frame is actually rendered. wlr-screencopy doesn't give much
  // flexibility there, but since the worst thing that may happen in such
  // case is a rescaled thumbnail with wrong aspect ratio we take the liberty
  // to ignore it, at least for now.
  struct wlr_box box;
  view_get_box(view, &box);

  frame->format = WL_SHM_FORMAT_ARGB8888;
  frame->width = box.width * view->wlr_surface->current.scale;
  frame->height = box.height * view->wlr_surface->current.scale;

  double scale = 1.0;
  if (max_width && frame->width > max_width) {
    scale = max_width / (double)frame->width;
  }
  if (max_height && frame->height > max_height) {
    scale = fmin(scale, max_height / (double)frame->height);
  }
  frame->width *= scale;
  frame->height *= scale;

  frame->stride = 4 * frame->width;

  zwlr_screencopy_frame_v1_send_buffer(frame->resource, frame->format,
      frame->width, frame->height, frame->stride);
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
  handle_get_thumbnail
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
    g_info ("FIXME: allowing every client to bind as phosh");
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


struct phosh_private_screencopy_frame *
phosh_private_screencopy_frame_from_resource (struct wl_resource *resource)
{
  assert(wl_resource_instance_of(resource, &zwlr_screencopy_frame_v1_interface,
				 &phosh_private_screencopy_frame_impl));
  return wl_resource_get_user_data(resource);
}
