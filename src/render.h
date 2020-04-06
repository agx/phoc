/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include "output.h"

void output_render(struct roots_output *output);
void view_render_to_buffer (struct roots_view *view, int width, int height, int stride, uint32_t *flags, void* data);
