/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-keybindings"

/**
 * PhocKeybindings:
 *
 * Keybindings stored in gsettings
 */
#include "phoc-config.h"
#include "keybindings.h"
#include "seat.h"
#include "server.h"
#include "keyboard.h"

#include <wlr/types/wlr_keyboard.h>

#include <gio/gio.h>

#define KEYBINDINGS_SCHEMA_ID "org.gnome.desktop.wm.keybindings"
#define MUTTER_KEYBINDINGS_SCHEMA_ID "org.gnome.mutter.keybindings"

typedef void (*PhocKeyHandlerFunc) (PhocSeat *seat, GVariant *param);

/**
 * PhocKeybinding:
 *
 * A keybinding represents a handler with params that will be
 * invoked on the given keybinding combinations.
 */
typedef struct
{
  gchar              *name;
  PhocKeyHandlerFunc  func;
  GVariant           *param;

  GSList             *combos;
} PhocKeybinding;


typedef struct _PhocKeybindings
{
  GObject parent;

  GSList *bindings;
  GSettings *settings;
  GSettings *mutter_settings;
} PhocKeybindings;

G_DEFINE_TYPE (PhocKeybindings, phoc_keybindings, G_TYPE_OBJECT);


static void
handle_always_on_top (PhocSeat *seat, GVariant *param)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocView *view = phoc_seat_get_focus_view (seat);
  gboolean on_top;

  if (!view)
    return;

  on_top = !phoc_view_is_always_on_top (view);
  g_debug ("always-on-top for %s: %d", phoc_view_get_app_id (view), on_top);
  phoc_desktop_set_view_always_on_top (desktop, view, on_top);
}


static void
handle_maximize (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);

  if (!view)
    return;

  phoc_view_maximize (view, NULL);
}

static void
handle_unmaximize (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);

  if (!view)
    return;

  phoc_view_restore (view);
}


static void
handle_tile (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);
  PhocViewTileDirection dir;

  if (!view)
    return;

  dir = g_variant_get_int32 (param);
  if (phoc_view_is_tiled (view) && phoc_view_get_tile_direction (view) == dir)
    phoc_view_restore (view);
  else
    phoc_view_tile (view, dir, NULL);
}


static void
handle_toggle_maximized (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);

  if (!view)
    return;

  if (phoc_view_is_maximized (view))
    phoc_view_restore (view);
  else
    phoc_view_maximize (view, NULL);
}


static void
handle_toggle_fullscreen (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);

  if (!view)
    return;

  phoc_view_set_fullscreen (view, !phoc_view_is_fullscreen (view), NULL);
}


static void
handle_cycle_windows (PhocSeat *seat, GVariant *param)
{
  phoc_seat_cycle_focus (seat, TRUE);
}


static void
handle_cycle_windows_backwards (PhocSeat *seat, GVariant *param)
{
  phoc_seat_cycle_focus (seat, FALSE);
}


static void
handle_close (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);

  if (!view)
    return;

  phoc_view_close (view);
}


static void
handle_move_to_monitor (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);
  enum wlr_direction dir;

  if (!view)
    return;

  dir = g_variant_get_int32 (param);
  phoc_view_move_to_next_output (view, dir);
}


static void
handle_move_to_corner (PhocSeat *seat, GVariant *param)
{
  PhocView *view = phoc_seat_get_focus_view (seat);
  PhocViewCorner corner;

  corner = g_variant_get_int32 (param);

  if (!view)
    return;

  phoc_view_move_to_corner (view, corner);
}


static void
handle_switch_input_source (PhocSeat *seat, GVariant *param)
{
  struct wlr_keyboard *wlr_keyboard = wlr_seat_get_keyboard (seat->seat);
  PhocKeyboard *keyboard;

  if (wlr_keyboard == NULL)
    return;

  keyboard = wlr_keyboard->data;
  g_return_if_fail (PHOC_IS_KEYBOARD (keyboard));

  phoc_keyboard_next_layout (keyboard);
}


/* This is copied from mutter which in turn got it form GTK+ */
static inline gboolean
is_alt (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'a' || string[1] == 'A') &&
          (string[2] == 'l' || string[2] == 'L') &&
          (string[3] == 't' || string[3] == 'T') &&
          (string[4] == '>'));
}

static inline gboolean
is_ctl (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'c' || string[1] == 'C') &&
          (string[2] == 't' || string[2] == 'T') &&
          (string[3] == 'l' || string[3] == 'L') &&
          (string[4] == '>'));
}

static inline gboolean
is_modx (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'm' || string[1] == 'M') &&
          (string[2] == 'o' || string[2] == 'O') &&
          (string[3] == 'd' || string[3] == 'D') &&
          (string[4] >= '1' && string[4] <= '5') &&
          (string[5] == '>'));
}

static inline gboolean
is_ctrl (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'c' || string[1] == 'C') &&
          (string[2] == 't' || string[2] == 'T') &&
          (string[3] == 'r' || string[3] == 'R') &&
          (string[4] == 'l' || string[4] == 'L') &&
          (string[5] == '>'));
}

static inline gboolean
is_shft (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 's' || string[1] == 'S') &&
          (string[2] == 'h' || string[2] == 'H') &&
          (string[3] == 'f' || string[3] == 'F') &&
          (string[4] == 't' || string[4] == 'T') &&
          (string[5] == '>'));
}

static inline gboolean
is_shift (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 's' || string[1] == 'S') &&
          (string[2] == 'h' || string[2] == 'H') &&
          (string[3] == 'i' || string[3] == 'I') &&
          (string[4] == 'f' || string[4] == 'F') &&
          (string[5] == 't' || string[5] == 'T') &&
          (string[6] == '>'));
}

static inline gboolean
is_control (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'c' || string[1] == 'C') &&
          (string[2] == 'o' || string[2] == 'O') &&
          (string[3] == 'n' || string[3] == 'N') &&
          (string[4] == 't' || string[4] == 'T') &&
          (string[5] == 'r' || string[5] == 'R') &&
          (string[6] == 'o' || string[6] == 'O') &&
          (string[7] == 'l' || string[7] == 'L') &&
          (string[8] == '>'));
}

static inline gboolean
is_meta (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'm' || string[1] == 'M') &&
          (string[2] == 'e' || string[2] == 'E') &&
          (string[3] == 't' || string[3] == 'T') &&
          (string[4] == 'a' || string[4] == 'A') &&
          (string[5] == '>'));
}

static inline gboolean
is_super (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 's' || string[1] == 'S') &&
          (string[2] == 'u' || string[2] == 'U') &&
          (string[3] == 'p' || string[3] == 'P') &&
          (string[4] == 'e' || string[4] == 'E') &&
          (string[5] == 'r' || string[5] == 'R') &&
          (string[6] == '>'));
}

static inline gboolean
is_hyper (const gchar *string)
{
  return ((string[0] == '<') &&
          (string[1] == 'h' || string[1] == 'H') &&
          (string[2] == 'y' || string[2] == 'Y') &&
          (string[3] == 'p' || string[3] == 'P') &&
          (string[4] == 'e' || string[4] == 'E') &&
          (string[5] == 'r' || string[5] == 'R') &&
          (string[6] == '>'));
}

static inline gboolean
is_keycode (const gchar *string)
{
  return (string[0] == '0' &&
          string[1] == 'x' &&
          g_ascii_isxdigit (string[2]) &&
          g_ascii_isxdigit (string[3]));
}

/**
 * phoc_parse_accelerator: (skip)
 *
 * Parse strings representing keybindings into modifier
 * and symbols.
 */
PhocKeyCombo *
phoc_parse_accelerator (const gchar *accelerator)
{
  PhocKeyCombo *combo;
  xkb_keysym_t keyval;
  guint32 mods;
  gint len;

  if (accelerator == NULL)
    return FALSE;

  keyval = 0;
  mods = 0;
  len = strlen (accelerator);
  while (len) {
    if (*accelerator == '<') {
      if (len >= 9 && is_control (accelerator)) {
        accelerator += 9;
        len -= 9;
        mods |= WLR_MODIFIER_CTRL;
      } else if (len >= 7 && is_shift (accelerator)) {
        accelerator += 7;
        len -= 7;
        mods |= WLR_MODIFIER_SHIFT;
      } else if (len >= 6 && is_shft (accelerator)) {
        accelerator += 6;
        len -= 6;
        mods |= WLR_MODIFIER_SHIFT;
      } else if (len >= 6 && is_ctrl (accelerator)) {
        accelerator += 6;
        len -= 6;
        mods |= WLR_MODIFIER_CTRL;
      } else if (len >= 6 && is_modx (accelerator)) {
        static const guint mod_vals[] = {
          WLR_MODIFIER_ALT,
          WLR_MODIFIER_MOD2,
          WLR_MODIFIER_MOD3,
          WLR_MODIFIER_LOGO,
          WLR_MODIFIER_MOD5,
        };

        len -= 6;
        accelerator += 4;
        mods |= mod_vals[*accelerator - '1'];
        accelerator += 2;
      }  else if (len >= 5 && is_ctl (accelerator)) {
        accelerator += 5;
        len -= 5;
        mods |= WLR_MODIFIER_CTRL;
      } else if (len >= 5 && is_alt (accelerator)) {
        accelerator += 5;
        len -= 5;
        mods |= WLR_MODIFIER_ALT;
      } else if (len >= 6 && is_meta (accelerator)) {
        g_warning ("Unhandled modifier meta");
        return FALSE;
      } else if (len >= 7 && is_hyper (accelerator)) {
        g_warning ("Unhandled modifier hyper");
        return FALSE;
      } else if (len >= 7 && is_super (accelerator)) {
        accelerator += 7;
        len -= 7;
        mods |= WLR_MODIFIER_LOGO;
      } else {
        gchar last_ch;

        last_ch = *accelerator;
        while (last_ch && last_ch != '>') {
          last_ch = *accelerator;
          accelerator += 1;
          len -= 1;
        }
      }
    } else {
      if (len >= 4 && is_keycode (accelerator)) {
        //keycode = strtoul (accelerator, NULL, 16);
        g_warning ("Unhandled keycode accelerator'");
        goto out;
      } else if (strcmp (accelerator, "Above_Tab") == 0) {
        g_warning ("Unhandled key 'Above_Tab'");
        return FALSE;
      } else {
        keyval = xkb_keysym_from_name (accelerator, XKB_KEYSYM_CASE_INSENSITIVE);
        if (keyval == XKB_KEY_NoSymbol) {
          g_autofree gchar *with_xf86 = g_strconcat ("XF86", accelerator, NULL);
          keyval = xkb_keysym_from_name (with_xf86, XKB_KEYSYM_CASE_INSENSITIVE);

          if (keyval == XKB_KEY_NoSymbol)
            return FALSE;
        }
      }

      accelerator += len;
      len -= len;
    }
  }

 out:
  combo = g_new0 (PhocKeyCombo, 1);
  combo->modifiers = mods;
  combo->keysym = keyval;
  return combo;
}


static void
phoc_keybinding_free (PhocKeybinding *keybinding)
{
  g_slist_free_full (keybinding->combos, (GDestroyNotify)g_free);
  g_free (keybinding->name);
  if (keybinding->param)
    g_variant_unref (keybinding->param);
  g_free (keybinding);
}


static gboolean
key_combo_eq (const PhocKeyCombo *sym1, const PhocKeyCombo *sym2)
{
  return (sym1->modifiers == sym2->modifiers && sym1->keysym == sym2->keysym);
}


static gboolean
keybinding_by_name (const PhocKeybinding *keybinding, const gchar *name)
{
  return g_strcmp0 (keybinding->name, name);
}


static gboolean
keybinding_by_key_combo (const PhocKeybinding *keybinding, const PhocKeyCombo *combo)
{
  for (GSList *elem = keybinding->combos; elem; elem = elem->next) {
    if (key_combo_eq (elem->data, combo))
      return FALSE;
  }

  return TRUE;
}


static void
on_keybinding_setting_changed (PhocKeybindings *self,
                               const gchar     *key,
                               GSettings       *settings)
{
  g_auto(GStrv) accelerators = NULL;
  PhocKeybinding *keybinding;
  GSList *elem;
  int i;

  g_return_if_fail (PHOC_IS_KEYBINDINGS (self));
  g_return_if_fail (G_IS_SETTINGS (settings));

  accelerators = g_settings_get_strv (settings, key);

  elem = g_slist_find_custom (self->bindings,
                              key,
                              (GCompareFunc)keybinding_by_name);
  if (!elem) {
    g_warning ("Changed keybinding %s not known", key);
    return;
  }

  keybinding = elem->data;

  g_slist_free_full (keybinding->combos, g_free);
  keybinding->combos = NULL;

  for (i = 0; accelerators && accelerators[i]; i++) {
    PhocKeyCombo *combo;

    g_debug ("New keybinding %s for %s", key, accelerators[i]);
    combo = phoc_parse_accelerator (accelerators[i]);
    if (combo)
      keybinding->combos = g_slist_append (keybinding->combos, combo);
  }
}


static gboolean
phoc_add_keybinding (PhocKeybindings    *self,
                     GSettings          *settings,
                     const gchar        *name,
                     PhocKeyHandlerFunc  func,
                     GVariant           *param)
{
  g_autofree gchar *signal_name = NULL;
  PhocKeybinding *binding;

  if (g_slist_find_custom (self->bindings, name, (GCompareFunc)keybinding_by_name)) {
    g_warning ("Keybinding '%s' already exists", name);
    return FALSE;
  }

  binding = g_new0 (PhocKeybinding, 1);
  binding->name = g_strdup (name);
  binding->func = func;
  if (param)
    binding->param = g_variant_ref_sink (param);

  signal_name = g_strdup_printf ("changed::%s", name);
  g_signal_connect_swapped (settings, signal_name,
                            G_CALLBACK (on_keybinding_setting_changed), self);

  self->bindings = g_slist_append (self->bindings, binding);
  /* Fill in initial values */
  on_keybinding_setting_changed (self, name, settings);

  return TRUE;
}

static void
phoc_keybindings_dispose (GObject *object)
{
  PhocKeybindings *self = PHOC_KEYBINDINGS (object);

  g_slist_free_full (self->bindings, (GDestroyNotify)phoc_keybinding_free);
  self->bindings = NULL;

  G_OBJECT_CLASS (phoc_keybindings_parent_class)->dispose (object);
}


static void
phoc_keybindings_finalize (GObject *object)
{
  PhocKeybindings *self = PHOC_KEYBINDINGS (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->mutter_settings);

  G_OBJECT_CLASS (phoc_keybindings_parent_class)->finalize (object);
}


static void
phoc_keybindings_constructed (GObject *object)
{
  PhocKeybindings *self = PHOC_KEYBINDINGS (object);

  G_OBJECT_CLASS (phoc_keybindings_parent_class)->constructed (object);

  self->settings = g_settings_new (KEYBINDINGS_SCHEMA_ID);
  phoc_add_keybinding (self, self->settings, "always-on-top", handle_always_on_top, NULL);
  phoc_add_keybinding (self, self->settings, "close", handle_close, NULL);
  phoc_add_keybinding (self, self->settings, "cycle-windows", handle_cycle_windows, NULL);
  phoc_add_keybinding (self, self->settings,
                       "cycle-windows-backward", handle_cycle_windows_backwards,
                       NULL);
  phoc_add_keybinding (self, self->settings, "maximize", handle_maximize, NULL);
  phoc_add_keybinding (self, self->settings, "toggle-fullscreen", handle_toggle_fullscreen, NULL);
  phoc_add_keybinding (self, self->settings, "toggle-maximized", handle_toggle_maximized, NULL);
  phoc_add_keybinding (self, self->settings,
                       "move-to-monitor-up", handle_move_to_monitor,
                       g_variant_new_int32 (WLR_DIRECTION_UP));
  phoc_add_keybinding (self, self->settings,
                       "move-to-monitor-down", handle_move_to_monitor,
                       g_variant_new_int32 (WLR_DIRECTION_DOWN));
  phoc_add_keybinding (self, self->settings,
                       "move-to-monitor-right", handle_move_to_monitor,
                       g_variant_new_int32 (WLR_DIRECTION_RIGHT));
  phoc_add_keybinding (self, self->settings,
                       "move-to-monitor-left", handle_move_to_monitor,
                       g_variant_new_int32 (WLR_DIRECTION_LEFT));
  phoc_add_keybinding (self, self->settings,
                       "move-to-corner-nw", handle_move_to_corner,
                       g_variant_new_int32 (PHOC_VIEW_CORNER_NORTH_WEST));
  phoc_add_keybinding (self, self->settings,
                       "move-to-corner-ne", handle_move_to_corner,
                       g_variant_new_int32 (PHOC_VIEW_CORNER_NORTH_EAST));
  phoc_add_keybinding (self, self->settings,
                       "move-to-corner-se", handle_move_to_corner,
                       g_variant_new_int32 (PHOC_VIEW_CORNER_SOUTH_EAST));
  phoc_add_keybinding (self, self->settings,
                       "move-to-corner-sw", handle_move_to_corner,
                       g_variant_new_int32 (PHOC_VIEW_CORNER_SOUTH_WEST));
  /* TODO: we need a real switch-applications but ALT-TAB should do s.th.
   * useful */
  phoc_add_keybinding (self, self->settings, "switch-applications", handle_cycle_windows, NULL);
  phoc_add_keybinding (self, self->settings,
                       "switch-applications-backward", handle_cycle_windows_backwards,
                       NULL);
  phoc_add_keybinding (self, self->settings,"unmaximize", handle_unmaximize, NULL);
  phoc_add_keybinding (self, self->settings,
                       "switch-input-source", handle_switch_input_source,
                       NULL);

  self->mutter_settings = g_settings_new (MUTTER_KEYBINDINGS_SCHEMA_ID);
  phoc_add_keybinding (self, self->mutter_settings,
                       "toggle-tiled-left", handle_tile,
                       g_variant_new_int32 (PHOC_VIEW_TILE_LEFT));
  phoc_add_keybinding (self, self->mutter_settings,
                       "toggle-tiled-right", handle_tile,
                       g_variant_new_int32 (PHOC_VIEW_TILE_RIGHT));
}


static void
phoc_keybindings_class_init (PhocKeybindingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_keybindings_constructed;
  object_class->dispose = phoc_keybindings_dispose;
  object_class->finalize = phoc_keybindings_finalize;
}


static void
phoc_keybindings_init (PhocKeybindings *self)
{
  self->bindings = NULL;
}


PhocKeybindings *
phoc_keybindings_new (void)
{
  return g_object_new (PHOC_TYPE_KEYBINDINGS, NULL);
}

/**
 * phoc_keybindings_handle_pressed:
 * @self: The keybindings
 * @modifiers: The currently pressed modifiers
 * @pressed_keysyms: The currently pressed keysyms
 * @length: The number of pressed keysyms
 * @seat: The seat this is happening on
 *
 * Check if a keybinding is known and run the associated action
 *
 * Returns: Whether the keybinding was handled.
 */
gboolean
phoc_keybindings_handle_pressed (PhocKeybindings *self,
                                 guint32          modifiers,
                                 xkb_keysym_t    *pressed_keysyms,
                                 guint32          length,
                                 PhocSeat        *seat)
{
  PhocKeybinding *keybinding;
  GSList *elem;
  PhocKeyCombo combo;

  if (length != 1)
    return FALSE;

  combo.keysym = pressed_keysyms[0];
  combo.modifiers = modifiers;

  elem = g_slist_find_custom (self->bindings,
                              &combo,
                              (GCompareFunc)keybinding_by_key_combo);
  if (!elem)
    return FALSE;

  g_return_val_if_fail (elem->data, FALSE);
  keybinding = elem->data;

  (*keybinding->func) (seat, keybinding->param);
  return TRUE;
}
