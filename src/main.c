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

  if (!phoc_server_setup (server, argc, argv))
    return 1;

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  g_object_unref (server);

  return 0;
}
