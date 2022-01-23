#define G_LOG_DOMAIN "phoc-settings"

#include "config.h"

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <unistd.h>
#include <wlr/config.h>
#include <wlr/types/wlr_box.h>
#include <wlr/util/log.h>
#include "settings.h"
#include "utils.h"

static bool parse_modeline(const char *s, drmModeModeInfo *mode) {
	char hsync[16];
	char vsync[16];
	float fclock;

	mode->type = DRM_MODE_TYPE_USERDEF;

	if (sscanf(s, "%f %hd %hd %hd %hd %hd %hd %hd %hd %15s %15s",
		   &fclock,
		   &mode->hdisplay,
		   &mode->hsync_start,
		   &mode->hsync_end,
		   &mode->htotal,
		   &mode->vdisplay,
		   &mode->vsync_start,
		   &mode->vsync_end,
		   &mode->vtotal, hsync, vsync) != 11) {
		return false;
	}

	mode->clock = fclock * 1000;
	mode->vrefresh = mode->clock * 1000.0 * 1000.0
		/ mode->htotal / mode->vtotal;
	if (strcasecmp(hsync, "+hsync") == 0) {
		mode->flags |= DRM_MODE_FLAG_PHSYNC;
	} else if (strcasecmp(hsync, "-hsync") == 0) {
		mode->flags |= DRM_MODE_FLAG_NHSYNC;
	} else {
		return false;
	}

	if (strcasecmp(vsync, "+vsync") == 0) {
		mode->flags |= DRM_MODE_FLAG_PVSYNC;
	} else if (strcasecmp(vsync, "-vsync") == 0) {
		mode->flags |= DRM_MODE_FLAG_NVSYNC;
	} else {
		return false;
	}

	snprintf(mode->name, sizeof(mode->name), "%dx%d@%d",
		 mode->hdisplay, mode->vdisplay, mode->vrefresh / 1000);

	return true;
}

static const char *output_prefix = "output:";
static const char *device_prefix = "device:";
static const char *cursor_prefix = "cursor:";
static const char *switch_prefix = "switch:";

static int config_ini_handler(PhocConfig *config, const char *section, const char *name,
		const char *value) {
	if (strcmp(section, "core") == 0) {
		if (strcmp(name, "xwayland") == 0) {
			if (strcasecmp(value, "true") == 0) {
				config->xwayland = true;
			} else if (strcasecmp(value, "immediate") == 0) {
				config->xwayland = true;
				config->xwayland_lazy = false;
			} else if (strcasecmp(value, "false") == 0) {
				config->xwayland = false;
			} else {
				g_critical ("got unknown xwayland value: %s", value);
			}
		} else {
			g_critical ("got unknown core config: %s", name);
		}
	} else if (strncmp(output_prefix, section, strlen(output_prefix)) == 0) {
		const char *output_name = section + strlen(output_prefix);
		struct roots_output_config *oc;
		bool found = false;

		wl_list_for_each(oc, &config->outputs, link) {
			if (strcmp(oc->name, output_name) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			oc = calloc(1, sizeof(struct roots_output_config));
			oc->name = strdup(output_name);
			oc->transform = WL_OUTPUT_TRANSFORM_NORMAL;
			oc->scale = 1;
			oc->enable = true;
			wl_list_init(&oc->modes);
			wl_list_insert(&config->outputs, &oc->link);
		}

		if (strcmp(name, "enable") == 0) {
			if (strcasecmp(value, "true") == 0) {
				oc->enable = true;
			} else if (strcasecmp(value, "false") == 0) {
				oc->enable = false;
			} else {
				g_critical ("got invalid output enable value: %s", value);
			}
		} else if (strcmp(name, "x") == 0) {
			oc->x = strtol(value, NULL, 10);
		} else if (strcmp(name, "y") == 0) {
			oc->y = strtol(value, NULL, 10);
		} else if (strcmp(name, "scale") == 0) {
			oc->scale = strtof(value, NULL);
			assert(oc->scale > 0);
		} else if (strcmp(name, "rotate") == 0) {
			if (strcmp(value, "normal") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_NORMAL;
			} else if (strcmp(value, "90") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_90;
			} else if (strcmp(value, "180") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_180;
			} else if (strcmp(value, "270") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_270;
			} else if (strcmp(value, "flipped") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_FLIPPED;
			} else if (strcmp(value, "flipped-90") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_FLIPPED_90;
			} else if (strcmp(value, "flipped-180") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_FLIPPED_180;
			} else if (strcmp(value, "flipped-270") == 0) {
				oc->transform = WL_OUTPUT_TRANSFORM_FLIPPED_270;
			} else {
				g_critical ("got unknown transform value: %s", value);
			}
			/* Make sure we rotate clockwise */
			phoc_utils_fix_transform(&oc->transform);
		} else if (strcmp(name, "mode") == 0) {
			char *end;
			oc->mode.width = strtol(value, &end, 10);
			assert(*end == 'x');
			++end;
			oc->mode.height = strtol(end, &end, 10);
			if (*end) {
				assert(*end == '@');
				++end;
				oc->mode.refresh_rate = strtof(end, &end);
				assert(strcmp("Hz", end) == 0);
			}
			g_debug ("Configured output %s with mode %dx%d@%f",
					oc->name, oc->mode.width, oc->mode.height,
					oc->mode.refresh_rate);
		} else if (strcmp(name, "modeline") == 0) {
			struct roots_output_mode_config *mode = calloc(1, sizeof(*mode));

			if (parse_modeline(value, &mode->info)) {
				wl_list_insert(&oc->modes, &mode->link);
			} else {
				free(mode);
				g_critical ("Invalid modeline: %s", value);
			}
		}
	} else if (strncmp(cursor_prefix, section, strlen(cursor_prefix)) == 0) {
		g_warning ("Found unused 'cursor:' config section. Please remove");
	} else if (strcmp(section, "cursor") == 0) {
		g_warning ("Found unused 'cursor' config section. Please remove");
	} else if (strncmp(device_prefix, section, strlen(device_prefix)) == 0) {
		g_warning ("Found unused 'device:' config section. Please remove");
	} else if (strncmp(switch_prefix, section, strlen(switch_prefix)) == 0) {
		g_warning ("Found unused 'switch:' config section. Please remove");
	} else {
		g_critical ("got unknown config section: %s", section);
	}

	return 1;
}


/**
 * phoc_config_create:
 * @config_path: (nullable): The config file location
 *
 * Parse the file at the given location into a configuration.
 */
PhocConfig *
phoc_config_create (const char *config_path)
{
  PhocConfig *config = g_new0 (PhocConfig, 1);
  g_autoptr (GKeyFile) keyfile = g_key_file_new ();
  g_autoptr (GError) err = NULL;
  g_auto (GStrv) sections = NULL;

  config->xwayland = true;
  config->xwayland_lazy = true;
  wl_list_init(&config->outputs);

  config->config_path = g_strdup (config_path);

  if (!config->config_path) {
    // get the config path from the current directory
    char cwd[MAXPATHLEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      char buf[MAXPATHLEN];
      if (snprintf(buf, MAXPATHLEN, "%s/%s", cwd, "phoc.ini") >= MAXPATHLEN) {
        g_critical ("config path too long");
        exit(1);
      }
      config->config_path = g_strdup (buf);
    } else {
      g_critical ("could not get cwd");
      exit(1);
    }
  }

  if (!g_key_file_load_from_file (keyfile, config->config_path, G_KEY_FILE_NONE, &err)) {
    if (g_error_matches (err, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
      g_debug ("No config file found. Using sensible defaults.");
      goto out;
    }

    g_critical ("Failed to parse config %s: %s", config->config_path, err->message);
    return NULL;
  }

  sections = g_key_file_get_groups (keyfile, NULL);

  for (int i = 0; i < g_strv_length (sections); i++) {
    const char *section = sections[i];
    g_auto (GStrv) keys = g_key_file_get_keys (keyfile, section, NULL, &err);

    if (!keys) {
      g_critical ("Failed to get keys for %s: %s", section, err->message);
      g_clear_error (&err);
      continue;
    }

    for (int j = 0; j < g_strv_length (keys); j++)  {
      const char *key = keys[j];
      g_autofree char *value = NULL;

      value = g_key_file_get_value (keyfile, section, key, &err);
      if (value == NULL) {
        g_critical ("Failed to key value for %s.%s: %s", section, key, err->message);
        g_clear_error (&err);
        continue;
      }

      config_ini_handler (config, section, key, value);
    }
  }

 out:
  config->keybindings = phoc_keybindings_new ();

  return config;
}


/**
 * phoc_config_destroy:
 * config: The #PhocConfig.
 *
 * Destroy the config and free its resources.
 */
void phoc_config_destroy (PhocConfig *config)
{
  struct roots_output_config *oc, *otmp = NULL;
  wl_list_for_each_safe(oc, otmp, &config->outputs, link) {
    struct roots_output_mode_config *omc, *omctmp = NULL;
    wl_list_for_each_safe(omc, omctmp, &oc->modes, link) {
      free(omc);
    }
    free(oc->name);
    free(oc);
  }

  g_object_unref (config->keybindings);

  free(config->config_path);
  free(config);
}

/**
 * phoc_config_get_output:
 * config: The #PhocConfig
 * output: The wlr output to get the configuration for
 *
 * Get configuration for the output. If the output is not configured, returns
 * NULL.
 */
PhocOutputConfig *
phoc_config_get_output (PhocConfig *config, struct wlr_output *output)
{
  char name[88];
  snprintf(name, sizeof(name), "%s %s %s", output->make, output->model,
           output->serial);

  PhocOutputConfig *oc;
  wl_list_for_each(oc, &config->outputs, link) {
    if (strcmp(oc->name, output->name) == 0 ||
        strcmp(oc->name, name) == 0) {
      return oc;
    }
  }

  return NULL;
}
