/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-server"

#include "config.h"
#include "server.h"

#include <errno.h>

G_DEFINE_TYPE(PhocServer, phoc_server, G_TYPE_OBJECT);

typedef struct {
  GSource source;
  struct wl_display *display;
} WaylandEventSource;

static gboolean
wayland_event_source_prepare (GSource *base,
                              int     *timeout)
{
  WaylandEventSource *source = (WaylandEventSource *)base;

  *timeout = -1;

  wl_display_flush_clients (source->display);

  return FALSE;
}

static gboolean
wayland_event_source_dispatch (GSource     *base,
                               GSourceFunc callback,
                               void        *data)
{
  WaylandEventSource *source = (WaylandEventSource *)base;
  struct wl_event_loop *loop = wl_display_get_event_loop (source->display);

  wl_event_loop_dispatch (loop, 0);

  return TRUE;
}

static GSourceFuncs wayland_event_source_funcs = {
  wayland_event_source_prepare,
  NULL,
  wayland_event_source_dispatch,
  NULL
};

static GSource *
wayland_event_source_new (struct wl_display *display)
{
  WaylandEventSource *source;
  struct wl_event_loop *loop = wl_display_get_event_loop (display);

  source = (WaylandEventSource *) g_source_new (&wayland_event_source_funcs,
                                                sizeof (WaylandEventSource));
  source->display = display;
  g_source_add_unix_fd (&source->source,
                        wl_event_loop_get_fd (loop),
                        G_IO_IN | G_IO_ERR);

  return &source->source;
}

static void
phoc_wayland_init (PhocServer *server)
{
  GSource *wayland_event_source;

  wayland_event_source = wayland_event_source_new (server->wl_display);
  g_source_attach (wayland_event_source, NULL);
}


static void
on_session_exit (GPid pid, gint status, PhocServer *self)
{
  g_autoptr(GError) err = NULL;

  g_return_if_fail (PHOC_IS_SERVER (self));
  g_spawn_close_pid (pid);
  if (g_spawn_check_exit_status (status, &err)) {
    self->exit_status = 0;
  } else {
    if (err->domain ==  G_SPAWN_EXIT_ERROR)
      self->exit_status = err->code;
    else
      g_warning ("Session terminated: %s (%d)", err->message, self->exit_status);
  }
  g_main_loop_quit (self->mainloop);
}


static gboolean
phoc_startup_session_in_idle(PhocServer *self)
{
  GPid pid;
  g_autoptr(GError) err = NULL;
  gchar *cmd[] = { "/bin/sh", "-c", self->session, NULL };

  if (g_spawn_async (NULL, cmd, NULL,
		      G_SPAWN_DO_NOT_REAP_CHILD,
		      NULL, self, &pid, &err)) {
    g_child_watch_add (pid, (GChildWatchFunc)on_session_exit, self);
  } else {
    g_warning ("Failed to launch session: %s", err->message);
    g_main_loop_quit (self->mainloop);
  }
  return FALSE;
}

static void
phoc_startup_session (PhocServer *server)
{
  gint id;

  id = g_idle_add ((GSourceFunc) phoc_startup_session_in_idle, server);
  g_source_set_name_by_id (id, "[phoc] phoc_startup_session");
}


static void
phoc_server_constructed (GObject *object)
{
  PhocServer *self = PHOC_SERVER (object);

  self->wl_display = wl_display_create();
  if (self->wl_display == NULL)
    g_error("Could not create wayland display");

  self->backend = wlr_backend_autocreate(self->wl_display, NULL);
  if (self->backend == NULL)
    g_error("Could not start backend");

  self->renderer = wlr_backend_get_renderer(self->backend);
  if (self->renderer == NULL)
    g_error("Could not create renderer");

  self->data_device_manager =
    wlr_data_device_manager_create(self->wl_display);
  wlr_renderer_init_wl_display(self->renderer, self->wl_display);

  G_OBJECT_CLASS (phoc_server_parent_class)->constructed (object);
}


static void
phoc_server_dispose (GObject *object)
{
  PhocServer *self = PHOC_SERVER (object);

#ifdef PHOC_XWAYLAND
  // We need to shutdown Xwayland before disconnecting all clients, otherwise
  // wlroots will restart it automatically.
  g_clear_pointer (&self->desktop->xwayland, wlr_xwayland_destroy);
#endif

  g_clear_pointer (&self->wl_display, &wl_display_destroy_clients);
  g_clear_pointer (&self->wl_display, &wl_display_destroy);
  g_clear_object (&self->desktop);
  g_clear_pointer (&self->session, g_free);

  G_OBJECT_CLASS (phoc_server_parent_class)->finalize (object);
}


static void
phoc_server_class_init (PhocServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_server_constructed;
  object_class->finalize = phoc_server_dispose;
}

static void
phoc_server_init (PhocServer *self)
{
}

PhocServer *
phoc_server_get_default (void)
{
  static PhocServer *instance;
  static gboolean initialized;

  if (G_UNLIKELY (instance == NULL)) {
    if (G_UNLIKELY (initialized)) {
      g_error ("PhocServer can only be initialized once");
    }
    g_debug("Creating server");
    instance = g_object_new (PHOC_TYPE_SERVER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
    initialized = TRUE;
  }

  return instance;
}

/**
 * phoc_server_setup:
 *
 * Perform wayland server intialization: parse command line and config,
 * create the wayland socket, setup env vars.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
phoc_server_setup (PhocServer *server, const char *config_path,
		   const char *session, GMainLoop *mainloop,
		   PhocServerDebugFlags debug_flags)
{
  server->config = roots_config_create(config_path);
  if (!server->config) {
    g_warning("Failed to parse config");
    return FALSE;
  }

  server->mainloop = mainloop;
  server->exit_status = 1;
  server->desktop = phoc_desktop_new (server->config);
  server->input = input_create(server->config);
  server->session = g_strdup (session);
  server->mainloop = mainloop;
  server->debug_flags = debug_flags;

  const char *socket = wl_display_add_socket_auto(server->wl_display);
  if (!socket) {
    g_warning("Unable to open wayland socket: %s", strerror(errno));
    wlr_backend_destroy(server->backend);
    return FALSE;
  }

  g_info("Running compositor on wayland display '%s'", socket);
  setenv("_WAYLAND_DISPLAY", socket, true);

  if (!wlr_backend_start(server->backend)) {
    g_warning("Failed to start backend");
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);
    return FALSE;
  }

  setenv("WAYLAND_DISPLAY", socket, true);
#ifdef PHOC_XWAYLAND
  if (server->desktop->xwayland != NULL) {
    struct roots_seat *xwayland_seat =
      input_get_seat(server->input, ROOTS_CONFIG_DEFAULT_SEAT_NAME);
    wlr_xwayland_set_seat(server->desktop->xwayland, xwayland_seat->seat);
  }
#endif

  phoc_wayland_init (server);
  if (server->session)
    phoc_startup_session (server);

  return TRUE;
}

/**
 * phoc_server_get_exit_status:
 *
 * Return the session's exit status. This is only meaningful
 * if the session has ended.
 *
 * Returns: The session's exit status.
 */
gint
phoc_server_get_session_exit_status (PhocServer *self)
{
  return self->exit_status;
}
