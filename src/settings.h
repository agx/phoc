#pragma once

#include "keybindings.h"
#include "output.h"

#include <xf86drmMode.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_output_layout.h>

G_BEGIN_DECLS

#define PHOC_CONFIG_DEFAULT_SEAT_NAME "seat0"

typedef struct _PhocOutputModeConfig {
  drmModeModeInfo info;
} PhocOutputModeConfig;

/**
 * PhocOutputConfig: (free-func phoc_config_destroy) (copy-func none)
 *
 * An output configuration. This is used when parsing the config file as well as the
 * outputs states db
 */
typedef struct _PhocOutputConfig {
  char                    *name;
  bool                     enable;
  enum wl_output_transform transform;
  int                      x, y;
  float                    scale;
  PhocOutputScaleFilter    scale_filter;
  bool                     drm_panel_orientation;

  struct PhocMode {
    int   width, height;
    float refresh_rate; /* Hz */
  } mode;
  GSList                  *modes;

  guint                    phys_width, phys_height;
} PhocOutputConfig;

typedef struct _PhocConfig {
  bool             xwayland;
  bool             xwayland_lazy;

  PhocKeybindings *keybindings;

  GSList          *outputs;

  char            *config_path;
  char            *socket;
} PhocConfig;

PhocConfig       *phoc_config_new_from_file (const char *config_path);
PhocConfig       *phoc_config_new_from_data (const char *data);
void              phoc_config_destroy       (PhocConfig *config);
PhocOutputConfig *phoc_config_get_output    (PhocConfig *config, PhocOutput *output);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhocConfig, phoc_config_destroy)

PhocOutputConfig *phoc_output_config_new (const char *name);
void              phoc_output_config_destroy (PhocOutputConfig *oc);
void              phoc_output_config_dump (PhocOutputConfig *oc, const char *prefix);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhocOutputConfig, phoc_output_config_destroy)

#define phoc_output_config_dump(__oc, __prefix) G_STMT_START {          \
  g_debug ("%s%s: (%d,%d) (%dx%d@%.2f), enabled: %d, scale: %f, transform: %s", \
           __prefix ?: "",                                              \
           __oc->name,                                                  \
           __oc->x,                                                     \
           __oc->y,                                                     \
           __oc->mode.width,                                            \
           __oc->mode.height,                                           \
           __oc->mode.refresh_rate,                                     \
           __oc->enable,                                                \
           __oc->scale,                                                 \
           phoc_utils_transform_to_str (__oc->transform));              \
  } G_STMT_END

G_END_DECLS
