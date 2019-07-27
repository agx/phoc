#define G_LOG_DOMAIN "phoc"

#include "config.h"

#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <gio/gio.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/headless.h>
#include <wlr/backend/multi.h>
#include <wlr/config.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/util/log.h>
#include "settings.h"
#include "server.h"

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


static gboolean
phoc_startup_cmd_in_idle(PhocServer *server)
{
  const char *cmd = server->config->startup_cmd;
  pid_t pid = fork();

  g_return_val_if_fail (cmd, FALSE);

  if (pid < 0) {
    wlr_log(WLR_ERROR, "cannot execute binding command: fork() failed");
  } else if (pid == 0) {
    execl("/bin/sh", "/bin/sh", "-c", cmd, (void *)NULL);
  }

  return FALSE;
}


static void
phoc_startup_cmd (PhocServer *server)
{
  gint id;

  id = g_idle_add ((GSourceFunc) phoc_startup_cmd_in_idle, server);
  g_source_set_name_by_id (id, "[phoc] phoc_startup_cmd");
}


static void
setup_signals (void)
{
  sigset_t mask;

  /* wlroots uses this to talk to xwayland, block it before
     we spawn other threads */
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, NULL);
}


static void
log_glib(enum wlr_log_importance verbosity, const char *fmt, va_list args) {
  int level;

  switch (verbosity) {
  case WLR_ERROR:
    level = G_LOG_LEVEL_CRITICAL;
    break;
  case WLR_INFO:
    level = G_LOG_LEVEL_INFO;
    break;
  case WLR_DEBUG:
    level = G_LOG_LEVEL_DEBUG;
    break;
  default:
    g_assert_not_reached ();
  }

  g_logv("phoc-wlroots", level, fmt, args);
}


int
main(int argc, char **argv)
{
  GMainLoop *loop;
  PhocServer *server;

  setup_signals();

  wlr_log_init(WLR_DEBUG, log_glib);
  server = phoc_server_get_default ();

  server->config = roots_config_create_from_args(argc, argv);
  assert(server->config);

  server->desktop = phoc_desktop_new (server->config);
  server->input = input_create(server->config);

  const char *socket = wl_display_add_socket_auto(server->wl_display);
  if (!socket) {
    wlr_log_errno(WLR_ERROR, "Unable to open wayland socket");
    wlr_backend_destroy(server->backend);
    return 1;
  }

  wlr_log(WLR_INFO, "Running compositor on wayland display '%s'", socket);
  setenv("_WAYLAND_DISPLAY", socket, true);

  if (!wlr_backend_start(server->backend)) {
    wlr_log(WLR_ERROR, "Failed to start backend");
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);
    return 1;
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
  if (server->config->startup_cmd)
    phoc_startup_cmd (server);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  g_object_unref (server);

  return 0;
}
