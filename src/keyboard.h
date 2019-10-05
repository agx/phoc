#pragma once

#include <xkbcommon/xkbcommon.h>
#include "input.h"

#define PHOC_KEYBOARD_PRESSED_KEYSYMS_CAP 32

#define PHOC_TYPE_KEYBOARD (phoc_keyboard_get_type())

G_DECLARE_FINAL_TYPE (PhocKeyboard, phoc_keyboard, PHOC, KEYBOARD, GObject);

struct roots_input;

/* TODO: we keep the struct public due to the list links and
   notifiers but we should avoid other member access */
struct _PhocKeyboard {
  GObject parent;

  struct wl_listener device_destroy;
  struct wl_listener keyboard_key;
  struct wl_listener keyboard_modifiers;
  struct wl_list link;

  /* private */
  struct roots_input *input;
  struct roots_seat *seat;
  struct wlr_input_device *device;
  struct roots_keyboard_config *config;

  xkb_keysym_t pressed_keysyms_translated[PHOC_KEYBOARD_PRESSED_KEYSYMS_CAP];
  xkb_keysym_t pressed_keysyms_raw[PHOC_KEYBOARD_PRESSED_KEYSYMS_CAP];
};

PhocKeyboard *phoc_keyboard_create(struct wlr_input_device *device,
                                   struct roots_input *input);
PhocKeyboard *phoc_keyboard_new (void);
void          phoc_keyboard_destroy(PhocKeyboard *self);
void          phoc_keyboard_handle_key(PhocKeyboard *self,
                                       struct wlr_event_keyboard_key *event);
void          phoc_keyboard_handle_modifiers(PhocKeyboard *self);

