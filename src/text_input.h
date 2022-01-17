#pragma once

#include <wlr/types/wlr_text_input_v3.h>
#include <wlr/types/wlr_input_method_v2.h>
#include <wlr/types/wlr_surface.h>
#include "seat.h"

G_BEGIN_DECLS

/**
 * PhocInputMethodRelay:
 *
 * The relay structure manages the relationship between text-input and
 * input_method interfaces on a given seat.
 *
 * Multiple text-input interfaces may
 * be bound to a relay, but at most one will be focused (reveiving events) at
 * a time. At most one input-method interface may be bound to the seat. The
 * relay manages life cycle of both sides. When both sides are present and
 * focused, the relay passes messages between them.
 *
 * Text input focus is a subset of keyboard focus - if the text-input is
 * in the focused state, wl_keyboard sent an enter as well. However, having
 * wl_keyboard focused doesn't mean that text-input will be focused.
 */
typedef struct _PhocInputMethodRelay {
	PhocSeat *seat;

	struct wl_list text_inputs; // PhocTextInput::link
	struct wlr_input_method_v2 *input_method; // doesn't have to be present

	struct wl_listener text_input_new;

	struct wl_listener input_method_new;
	struct wl_listener input_method_commit;
	struct wl_listener input_method_destroy;
} PhocInputMethodRelay;

/**
 * PhocTextInput:
 * @pending_focused_surface: The surface getting seat's focus. Stored for when text-input cannot
 *    be sent an enter event immediately after getting focus, e.g. when
 *    there's no input method available. Cleared once text-input is entered.
 */
typedef struct _PhocTextInput {
	PhocInputMethodRelay *relay;

	struct wlr_text_input_v3 *input;
	struct wlr_surface *pending_focused_surface;

	struct wl_list link;

	struct wl_listener pending_focused_surface_destroy;
	struct wl_listener enable;
	struct wl_listener commit;
	struct wl_listener disable;
	struct wl_listener destroy;
} PhocTextInput;

void phoc_input_method_relay_init      (PhocSeat *seat, PhocInputMethodRelay *relay);
void phoc_input_method_relay_destroy   (PhocInputMethodRelay *relay);
void phoc_input_method_relay_set_focus (PhocInputMethodRelay *relay,
                                        struct wlr_surface   *surface);

G_END_DECLS
