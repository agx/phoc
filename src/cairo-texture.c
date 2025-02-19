/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cairo-texture.h"
#include "render-private.h"
#include "server.h"

#include <cairo.h>
#include <drm_fourcc.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/render/wlr_texture.h>

enum {
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_LAST_PROP
};

static GParamSpec *props[PROP_LAST_PROP];

struct _PhocCairoTexture {
  GObject parent;

  int                 width;
  int                 height;
  struct wlr_buffer   buffer;
  cairo_surface_t    *surface;
  cairo_t            *cairo;
  struct wlr_texture *texture;
};

G_DEFINE_TYPE (PhocCairoTexture, phoc_cairo_texture, G_TYPE_OBJECT)


static void
buffer_destroy (struct wlr_buffer *wlr_buffer)
{
  /* noop - we handle this in phoc_cairo_texture_finalize */
}


static bool
buffer_begin_data_ptr_access (struct wlr_buffer *wlr_buffer, uint32_t flags, void **data,
                              uint32_t *format, size_t * stride)
{
  PhocCairoTexture *self = wl_container_of (wlr_buffer, self, buffer);

  if (flags & WLR_BUFFER_DATA_PTR_ACCESS_WRITE)
    return false;

  *format = DRM_FORMAT_ARGB8888;
  *data = cairo_image_surface_get_data (self->surface);
  *stride = cairo_image_surface_get_stride (self->surface);
  return true;
}

static void
buffer_end_data_ptr_access (struct wlr_buffer *wlr_buffer)
{
}


static const struct wlr_buffer_impl buffer_impl = {
  .destroy = buffer_destroy,
  .begin_data_ptr_access = buffer_begin_data_ptr_access,
  .end_data_ptr_access = buffer_end_data_ptr_access
};

static void
phoc_cairo_texture_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhocCairoTexture *self = PHOC_CAIRO_TEXTURE (object);

  switch (property_id) {
  case PROP_WIDTH:
    self->width = g_value_get_int (value);
    break;
  case PROP_HEIGHT:
    self->height = g_value_get_int (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_cairo_texture_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhocCairoTexture *self = PHOC_CAIRO_TEXTURE (object);

  switch (property_id) {
  case PROP_WIDTH:
    g_value_set_int (value, self->width);
    break;
  case PROP_HEIGHT:
    g_value_set_int (value, self->height);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_cairo_texture_constructed (GObject *object)
{
  PhocCairoTexture *self = PHOC_CAIRO_TEXTURE (object);
  PhocServer *server = phoc_server_get_default ();
  PhocRenderer *renderer = phoc_server_get_renderer (server);
  cairo_status_t surface_status;

  G_OBJECT_CLASS (phoc_cairo_texture_parent_class)->constructed (object);

  wlr_buffer_init (&self->buffer, &buffer_impl, self->width, self->height);

  self->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, self->width, self->height);
  surface_status = cairo_surface_status (self->surface);
  if (surface_status != CAIRO_STATUS_SUCCESS) {
    g_warning ("Failed to create cairo surface: %d\n", surface_status);
    return;
  }
  self->cairo = cairo_create (self->surface);
  self->texture = wlr_texture_from_buffer (phoc_renderer_get_wlr_renderer (renderer),
                                           &self->buffer);
}


static void
phoc_cairo_texture_finalize (GObject *object)
{
  PhocCairoTexture *self = PHOC_CAIRO_TEXTURE (object);

  g_clear_pointer (&self->texture, wlr_texture_destroy);
  g_clear_pointer (&self->cairo, cairo_destroy);
  g_clear_pointer (&self->surface, cairo_surface_destroy);
  wlr_buffer_drop (&self->buffer);

  G_OBJECT_CLASS (phoc_cairo_texture_parent_class)->finalize (object);
}

static void
phoc_cairo_texture_class_init (PhocCairoTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_cairo_texture_constructed;
  object_class->finalize = phoc_cairo_texture_finalize;
  object_class->get_property = phoc_cairo_texture_get_property;
  object_class->set_property = phoc_cairo_texture_set_property;

  /**
   * PhocCairoTexture:width:
   *
   * Width of the texture and backing Cairo surface
   */
  props[PROP_WIDTH] =
    g_param_spec_int ("width", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |  G_PARAM_STATIC_STRINGS);

  /**
   * PhocCairoTexture:height:
   *
   * Height of the texture and backing Cairo surface
   */
  props[PROP_HEIGHT] =
    g_param_spec_int ("height", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_cairo_texture_init (PhocCairoTexture *self)
{
}


PhocCairoTexture *
phoc_cairo_texture_new (int width, int height)
{
  return g_object_new (PHOC_TYPE_CAIRO_TEXTURE,
                       "width", width,
                       "height", height,
                       NULL);
}


cairo_t *
phoc_cairo_texture_get_context (PhocCairoTexture *self)
{
  g_assert (PHOC_IS_CAIRO_TEXTURE (self));
  return self->cairo;
}


struct wlr_texture *
phoc_cairo_texture_get_texture (PhocCairoTexture *self)
{
  g_assert (PHOC_IS_CAIRO_TEXTURE (self));
  return self->texture;
}


void
phoc_cairo_texture_update (PhocCairoTexture *self)
{
  pixman_region32_t region;

  g_assert (PHOC_IS_CAIRO_TEXTURE (self));

  if (!self->texture)
    return;

  cairo_surface_flush (self->surface);

  pixman_region32_init_rect (&region, 0, 0, self->width, self->height);
  wlr_texture_update_from_buffer (self->texture, &self->buffer, &region);
  pixman_region32_fini (&region);
}
