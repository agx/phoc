#pragma once

#include <stdbool.h>
#include <wlr/config.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _PhocView PhocView;
typedef struct _PhocDesktop PhocDesktop;
typedef struct _PhocOutput PhocOutput;
typedef struct _PhocXdgSurface PhocXdgSurface;
typedef struct _PhocXWaylandSurface PhocXWaylandSurface;

typedef struct _PhocViewInterface {
	void (*move)(PhocView *view, double x, double y);
	void (*resize)(PhocView *view, uint32_t width, uint32_t height);
	void (*move_resize)(PhocView *view, double x, double y,
		uint32_t width, uint32_t height);
	bool (*want_scaling)(PhocView *view);
	bool (*want_auto_maximize)(PhocView *view);
	void (*set_active)(PhocView *view, bool active);
	void (*set_fullscreen)(PhocView *view, bool fullscreen);
	void (*set_maximized)(PhocView *view, bool maximized);
	void (*set_tiled)(PhocView *view, bool tiled);
	void (*close)(PhocView *view);
	void (*for_each_surface)(PhocView *view,
		wlr_surface_iterator_func_t iterator, void *user_data);
	void (*get_geometry)(PhocView *view, struct wlr_box *box);
} PhocViewInterface;

typedef enum {
	ROOTS_XDG_SHELL_VIEW,
#ifdef PHOC_XWAYLAND
	ROOTS_XWAYLAND_VIEW,
#endif
} PhocViewType;

typedef enum {
  PHOC_VIEW_TILE_LEFT,
  PHOC_VIEW_TILE_RIGHT,
} PhocViewTileDirection;

typedef enum {
  PHOC_VIEW_STATE_FLOATING,
  PHOC_VIEW_STATE_MAXIMIZED,
  PHOC_VIEW_STATE_TILED,
} PhocViewState;

/**
 * PhocView:
 * @type: The type of the toplevel
 * @impl: The interface imlementing the toplevel
 *
 * A `PhocView` represents a toplevel like an xdg-toplevel or a xwayland window.
 */
struct _PhocView {
	GObject parent_instance;

	PhocViewType type;
	const PhocViewInterface *impl;
	PhocDesktop *desktop;
	struct wl_list link; // PhocDesktop::views
	struct wl_list parent_link; // PhocView::stack

	struct wlr_box box;
	float alpha;
	float scale;

	bool decorated;
	int border_width;
	int titlebar_height;

	char *title;
	char *app_id;

	GSettings *settings;

	PhocViewState state;
	PhocViewTileDirection tile_direction;
	PhocOutput *fullscreen_output;
	struct wlr_box saved;

	struct {
		bool update_x, update_y;
		double x, y;
		uint32_t width, height;
	} pending_move_resize;
	bool pending_centering;

	PhocView *parent;
	struct wl_list stack; // PhocView::link

	struct wlr_surface *wlr_surface; // set only when the surface is mapped
	struct wl_list child_surfaces; // PhocViewChild::link

	struct wlr_foreign_toplevel_handle_v1 *toplevel_handle;
	struct wl_listener toplevel_handle_request_maximize;
	struct wl_listener toplevel_handle_request_activate;
	struct wl_listener toplevel_handle_request_fullscreen;
	struct wl_listener toplevel_handle_request_close;

	struct wl_listener surface_new_subsurface;

	struct {
		struct wl_signal unmap;
		struct wl_signal destroy;
	} events;
};

typedef struct _PhocViewClass
{
  GObjectClass parent_class;
} PhocViewClass;


#define PHOC_TYPE_VIEW (phoc_view_get_type ())

GType phoc_view_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhocView, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhocViewClass, g_type_class_unref)
static inline PhocView * PHOC_VIEW (gpointer ptr) {
  return G_TYPE_CHECK_INSTANCE_CAST (ptr, phoc_view_get_type (), PhocView); }
static inline PhocViewClass * PHOC_VIEW_CLASS (gpointer ptr) {
  return G_TYPE_CHECK_CLASS_CAST (ptr, phoc_view_get_type (), PhocViewClass); }
static inline gboolean PHOC_IS_VIEW (gpointer ptr) {
  return G_TYPE_CHECK_INSTANCE_TYPE (ptr, phoc_view_get_type ()); }
static inline gboolean PHOC_IS_VIEW_CLASS (gpointer ptr) {
  return G_TYPE_CHECK_CLASS_TYPE (ptr, phoc_view_get_type ()); }
static inline PhocViewClass * PHOC_VIEW_GET_CLASS (gpointer ptr) {
  return G_TYPE_INSTANCE_GET_CLASS (ptr, phoc_view_get_type (), PhocViewClass); }

struct roots_xdg_toplevel_decoration;

typedef struct _PhocViewChild PhocViewChild;

struct phoc_view_child_interface {
  void (*destroy)(PhocViewChild *child);
};

/**
 * PhocViewChild:
 * @link: Link to PhocView::child_surfaces
 * @view: The [type@PhocView] this child belongs to
 * @parent: (nullable): The parent of this child if another child
 * @children: (nullable): children of this child
 *
 * A child of a [type@PhocView], e.g. a popup or subsurface
 */
typedef struct _PhocViewChild {
  const struct phoc_view_child_interface *impl;

  PhocView *view;
  PhocViewChild *parent;
  GSList *children;
  struct wlr_surface *wlr_surface;
  struct wl_list link;
  bool mapped;

  struct wl_listener commit;
  struct wl_listener new_subsurface;
} PhocViewChild;

typedef struct _PhocSubsurface PhocSubsurface;

typedef struct roots_xdg_popup {
  PhocViewChild child;
  struct wlr_xdg_popup *wlr_popup;

  struct wl_listener destroy;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener new_popup;
} PhocXdgPopup;

struct roots_xdg_toplevel_decoration {
	struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration;
	PhocXdgSurface *surface;
	struct wl_listener destroy;
	struct wl_listener request_mode;
	struct wl_listener surface_commit;
};

void view_init(PhocView *view, const PhocViewInterface *impl,
               PhocViewType type, PhocDesktop *desktop);
void view_appear_activated(PhocView *view, bool activated);
void view_activate(PhocView *view, bool activate);
void phoc_view_apply_damage (PhocView *view);
void phoc_view_damage_whole (PhocView *view);
gboolean view_is_floating(const PhocView *view);
gboolean view_is_maximized(const PhocView *view);
gboolean view_is_tiled(const PhocView *view);
gboolean view_is_fullscreen(const PhocView *view);
void view_update_position(PhocView *view, int x, int y);
void view_update_size(PhocView *view, int width, int height);
void view_update_decorated(PhocView *view, bool decorated);
void view_initial_focus(PhocView *view);
void phoc_view_map (PhocView *view, struct wlr_surface *surface);
void view_unmap(PhocView *view);
void view_arrange_maximized(PhocView *view, struct wlr_output *output);
void view_arrange_tiled(PhocView *view, struct wlr_output *output);
void view_get_box(const PhocView *view, struct wlr_box *box);
void view_get_geometry(PhocView *view, struct wlr_box *box);
void view_move(PhocView *view, double x, double y);
bool view_move_to_next_output (PhocView *view, enum wlr_direction direction);
void view_resize(PhocView *view, uint32_t width, uint32_t height);
void view_move_resize(PhocView *view, double x, double y,
	uint32_t width, uint32_t height);
void view_auto_maximize(PhocView *view);
void view_tile(PhocView *view, PhocViewTileDirection direction, struct wlr_output *output);
void view_maximize(PhocView *view, struct wlr_output *output);
void view_restore(PhocView *view);
void phoc_view_set_fullscreen(PhocView *view, bool fullscreen, struct wlr_output *output);
void view_close(PhocView *view);
bool view_center(PhocView *view, struct wlr_output *output);
void view_send_frame_done_if_not_visible (PhocView *view);
void view_setup(PhocView *view);
void view_set_title(PhocView *view, const char *title);
void view_set_parent(PhocView *view, PhocView *parent);
void view_set_app_id(PhocView *view, const char *app_id);
void view_create_foreign_toplevel_handle(PhocView *view);
void view_get_deco_box(const PhocView *view, struct wlr_box *box);
void view_for_each_surface(PhocView *view,
	wlr_surface_iterator_func_t iterator, void *user_data);
PhocView *phoc_view_from_wlr_surface (struct wlr_surface *wlr_surface);

PhocXdgSurface *phoc_xdg_surface_from_view(PhocView *view);
PhocXWaylandSurface *phoc_xwayland_surface_from_view(PhocView *view);
bool   phoc_view_is_mapped (PhocView *view);

enum roots_deco_part {
	ROOTS_DECO_PART_NONE = 0,
	ROOTS_DECO_PART_TOP_BORDER = (1 << 0),
	ROOTS_DECO_PART_BOTTOM_BORDER = (1 << 1),
	ROOTS_DECO_PART_LEFT_BORDER = (1 << 2),
	ROOTS_DECO_PART_RIGHT_BORDER = (1 << 3),
	ROOTS_DECO_PART_TITLEBAR = (1 << 4),
};

enum roots_deco_part view_get_deco_part(PhocView *view, double sx, double sy);

void phoc_view_child_init(PhocViewChild *child,
                          const struct phoc_view_child_interface *impl,
                          PhocView *view,
                          struct wlr_surface *wlr_surface);
void phoc_view_child_destroy (PhocViewChild *child);
void phoc_view_child_apply_damage (PhocViewChild *child);
void phoc_view_child_damage_whole (PhocViewChild *child);

G_END_DECLS
