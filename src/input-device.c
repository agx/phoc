/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-input-device"

#include "config.h"

#include "input-device.h"
#include "seat.h"

#include <wlr/types/wlr_input_device.h>

enum {
  PROP_0,
  PROP_SEAT,
  PROP_DEVICE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhocInputDevice:
 *
 * Abstract base class for input device like pointers or touch.
 */
typedef struct _PhocInputDevicePrivate {
  struct wlr_input_device *device;
  PhocSeat                *seat;
} PhocInputDevicePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhocInputDevice, phoc_input_device, G_TYPE_OBJECT)


static void
phoc_input_device_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PhocInputDevice *self = PHOC_INPUT_DEVICE (object);
  PhocInputDevicePrivate *priv = phoc_input_device_get_instance_private (self);

  switch (property_id) {
  case PROP_DEVICE:
    priv->device = g_value_get_pointer (value);
    priv->device->data = self;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DEVICE]);
    break;
  case PROP_SEAT:
    priv->seat = g_value_dup_object (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SEAT]);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_input_device_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhocInputDevice *self = PHOC_INPUT_DEVICE (object);
  PhocInputDevicePrivate *priv = phoc_input_device_get_instance_private (self);

  switch (property_id) {
  case PROP_DEVICE:
    g_value_set_pointer (value, priv->device);
    break;
  case PROP_SEAT:
    g_value_set_pointer (value, priv->seat);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_input_device_dispose (GObject *object)
{
  PhocInputDevice *self = PHOC_INPUT_DEVICE(object);
  PhocInputDevicePrivate *priv = phoc_input_device_get_instance_private (self);

  g_clear_object (&priv->seat);

  G_OBJECT_CLASS (phoc_input_device_parent_class)->dispose (object);
}


static void
phoc_input_device_class_init (PhocInputDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_input_device_get_property;
  object_class->set_property = phoc_input_device_set_property;
  object_class->dispose = phoc_input_device_dispose;

  /**
   * PhocInputDevice:device:
   *
   * The underlying wlroots device
   */
  props[PROP_DEVICE] =
    g_param_spec_pointer ("device", "", "",
			  G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PhocInputDevice:seat:
   *
   * The seat this device belongs to
   */
  props[PROP_SEAT] =
    g_param_spec_object ("seat", "", "",
                         PHOC_TYPE_SEAT,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_input_device_init (PhocInputDevice *self)
{
}

/**
 * phoc_input_device_get_seat:
 * @self: The %PhocInputDevice
 *
 * Returns: The seat this input device belongs to.
 */
PhocSeat *
phoc_input_device_get_seat (PhocInputDevice *self)
{
  PhocInputDevicePrivate *priv;

  g_assert (PHOC_IS_INPUT_DEVICE (self));
  priv = phoc_input_device_get_instance_private (self);

  return priv->seat;
}

/**
 * phoc_input_device_get_device:
 * @self: The %PhocInputDevice
 *
 * Returns: (transfer none): The wlr_input_device. Note that
 * %PhocInputDevice device owns this so don't keep references around.
 */
struct wlr_input_device *
phoc_input_device_get_device (PhocInputDevice *self)
{
  PhocInputDevicePrivate *priv;

  g_assert (PHOC_IS_INPUT_DEVICE (self));
  priv = phoc_input_device_get_instance_private (self);

  return priv->device;
}

