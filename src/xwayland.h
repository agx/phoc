#pragma once

#include <wlr/xwayland.h>
#include <xcb/xproto.h>

enum xwayland_atom_name {
	NET_WM_WINDOW_TYPE_NORMAL,
	NET_WM_WINDOW_TYPE_DIALOG,
	XWAYLAND_ATOM_LAST
};
