#define G_LOG_DOMAIN "phoc-touch"

#include "config.h"

#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <time.h>
#include <wlr/backend/libinput.h>

#include "input-device.h"
#include "touch.h"
#include "seat.h"

/**
 * PhocTouch:
 *
 * A touch input device
 */
G_DEFINE_TYPE (PhocTouch, phoc_touch, PHOC_TYPE_INPUT_DEVICE);

enum {
  TOUCH_DESTROY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static void
handle_touch_destroy (struct wl_listener *listener, void *data)
{
  PhocTouch *self = wl_container_of (listener, self, touch_destroy);

  g_signal_emit (self, signals[TOUCH_DESTROY], 0);
}

static void
phoc_touch_constructed (GObject *object)
{
  PhocTouch *self = PHOC_TOUCH (object);
  struct wlr_input_device *device = phoc_input_device_get_device (PHOC_INPUT_DEVICE (self));

  self->touch_destroy.notify = handle_touch_destroy;
  wl_signal_add (&device->events.destroy, &self->touch_destroy);

  G_OBJECT_CLASS (phoc_touch_parent_class)->constructed (object);
}

static void
phoc_touch_finalize (GObject *object)
{
  PhocTouch *self = PHOC_TOUCH (object);

  wl_list_remove (&self->touch_destroy.link);

  G_OBJECT_CLASS (phoc_touch_parent_class)->finalize (object);
}

static void
phoc_touch_class_init (PhocTouchClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phoc_touch_constructed;
  object_class->finalize = phoc_touch_finalize;

  signals[TOUCH_DESTROY] = g_signal_new ("touch-destroyed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}


static void
phoc_touch_init (PhocTouch *self)
{
}


PhocTouch *
phoc_touch_new (struct wlr_input_device *device, PhocSeat *seat)
{
  return g_object_new (PHOC_TYPE_TOUCH,
                       "device", device,
                       "seat", seat,
                       NULL);
}
