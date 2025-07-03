/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-tracing"

#include "phoc-config.h"
#include "phoc-tracing.h"

#ifdef PHOC_USE_SYSPROF

void
phoc_trace_mark (gint64       begin_time_nsec,
                 gint64       duration_nsec,
                 const gchar *group,
                 const gchar *name,
                 const gchar *message_format,
                 ...)
{
  va_list args;

  va_start (args, message_format);
  sysprof_collector_mark_vprintf (begin_time_nsec,
                                  duration_nsec,
                                  group,
                                  name,
                                  message_format,
                                  args);
  va_end (args);
}

#endif  /* PHOC_USE_SYSPROF */
