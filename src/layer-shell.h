#pragma once

#include "gesture.h"

#include <stdbool.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/util/box.h>
#include "output.h"
#include "layer-surface.h"

G_BEGIN_DECLS

enum layer_parent {
  LAYER_PARENT_LAYER,
  LAYER_PARENT_POPUP,
  LAYER_PARENT_SUBSURFACE
};


gboolean                phoc_layer_shell_arrange                 (PhocOutput *output);
void                    phoc_layer_shell_update_focus            (void);
void                    phoc_layer_shell_update_osk              (PhocOutput *output,
                                                                  gboolean    arrange);
PhocLayerSurface *      phoc_layer_shell_find_osk                (PhocOutput *output);

void                    phoc_handle_layer_shell_surface          (struct wl_listener *listener,
                                                                  void       *data);

G_END_DECLS
