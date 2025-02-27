/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "animatable.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_SPINNER (phoc_spinner_get_type ())

G_DECLARE_FINAL_TYPE (PhocSpinner, phoc_spinner, PHOC, SPINNER, GObject)

PhocSpinner *phoc_spinner_new (PhocAnimatable *animatable, int cx, int cy, int size);

G_END_DECLS
