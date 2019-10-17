#define G_LOG_DOMAIN "phoc-virtual-keyboard"

#include "config.h"

#define _POSIX_C_SOURCE 199309L
#include <wlr/util/log.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include "virtual_keyboard.h"
#include "seat.h"

void handle_virtual_keyboard(struct wl_listener *listener, void *data) {
        PhocServer *server = phoc_server_get_default ();
	PhocDesktop *desktop =
		wl_container_of(listener, desktop, virtual_keyboard_new);
	struct wlr_virtual_keyboard_v1 *keyboard = data;

	struct roots_seat *seat = input_seat_from_wlr_seat(server->input,
		keyboard->seat);
	if (!seat) {
		wlr_log(WLR_ERROR, "could not find roots seat");
		return;
	}

	roots_seat_add_device(seat, &keyboard->input_device);
}
