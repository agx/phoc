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
  g_autoptr(GOptionContext) opt_context = NULL;
  g_autoptr(GError) err = NULL;
  g_autoptr(GMainLoop) loop = NULL;
  g_autoptr(PhocServer) server = NULL;
  g_autofree gchar *config_path = NULL;
  g_autofree gchar *exec = NULL;
  gboolean debug_damage = false;
  gboolean debug_touch = false;

  setup_signals();

  const GOptionEntry options [] = {
    {"config", 'C', 0, G_OPTION_ARG_STRING, &config_path,
     "Path to the configuration file. (default: phoc.ini).", NULL},
    {"exec", 'E', 0, G_OPTION_ARG_STRING, &exec,
     "Command (session) that will be ran at startup", NULL},
    {"damage-tracking", 'D', 0, G_OPTION_ARG_NONE, &debug_damage,
     "Enable damage tracking debugging", NULL},
    {"touch-debug", 't', 0, G_OPTION_ARG_NONE, &debug_touch,
     "Enable touch point visualization", NULL},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- A phone compositor");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  wlr_log_init(WLR_DEBUG, log_glib);
  server = phoc_server_get_default ();

  loop = g_main_loop_new (NULL, FALSE);
  if (!phoc_server_setup (server, config_path, exec, loop, debug_damage, debug_touch))
    return 1;

  g_main_loop_run (loop);

  return phoc_server_get_session_exit_status (server);
}
