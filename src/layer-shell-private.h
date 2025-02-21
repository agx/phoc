/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "layer-shell.h"

G_BEGIN_DECLS

void                 phoc_layer_shell_update_cursors (PhocLayerSurface *layer_surface,
                                                      GSList           *seats);

G_END_DECLS
