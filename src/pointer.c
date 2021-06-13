#define G_LOG_DOMAIN "phoc-pointer"

#include "config.h"

#include "pointer.h"
#include "seat.h"

#include <glib.h>

enum {
  PROP_0,
  PROP_DEVICE,
  PROP_SEAT,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

G_DEFINE_TYPE (PhocPointer, phoc_pointer, G_TYPE_OBJECT);

static void
phoc_pointer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PhocPointer *self = PHOC_POINTER (object);

  switch (property_id) {
  case PROP_DEVICE:
    self->device = g_value_get_pointer (value);
    self->device->data = self;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DEVICE]);
    break;
  case PROP_SEAT:
    self->seat = g_value_get_pointer (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SEAT]);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_pointer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PhocPointer *self = PHOC_POINTER (object);

  switch (property_id) {
  case PROP_DEVICE:
    g_value_set_pointer (value, self->device);
    break;
  case PROP_SEAT:
    g_value_set_pointer (value, self->seat);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_pointer_class_init (PhocPointerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->set_property = phoc_pointer_set_property;
  object_class->get_property = phoc_pointer_get_property;

  props[PROP_DEVICE] =
    g_param_spec_pointer ("device",
			  "Device",
			  "The device object",
			  G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_SEAT] =
    g_param_spec_pointer ("seat",
			  "Seat",
			  "The seat this pointer belongs to",
			  G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
phoc_pointer_init (PhocPointer *self)
{
}

PhocPointer *
phoc_pointer_new (struct wlr_input_device *device, struct roots_seat *seat)
{
  return g_object_new (PHOC_TYPE_POINTER,
                       "device", device,
                       "seat", seat,
                       NULL);
}
