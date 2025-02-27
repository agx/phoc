/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "output.h"
#include "phoc-animation.h"
#include "render.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_OUTPUT_SHIELD (phoc_output_shield_get_type ())

G_DECLARE_FINAL_TYPE (PhocOutputShield, phoc_output_shield, PHOC, OUTPUT_SHIELD, GObject)

PhocOutputShield   *phoc_output_shield_new                       (PhocOutput *output);
void                phoc_output_shield_raise                     (PhocOutputShield *self,
                                                                  gboolean show_spinner);
void                phoc_output_shield_lower                     (PhocOutputShield *self);
void                phoc_output_shield_set_easing                (PhocOutputShield *self,
                                                                  PhocEasing        easing);
void                phoc_output_shield_set_duration              (PhocOutputShield *self,
                                                                  guint             duration);
gboolean            phoc_output_shield_is_raised                 (PhocOutputShield *self);
G_END_DECLS
