/*
 * Copyright (C) 2023 The Phosh Developers
 *               2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "phoc-config.h"

#include <glib.h>

#pragma once

#ifdef PHOC_USE_DTRACE

# include <sys/sdt.h>

/**
 * PHOC_TRACE_NO_INLINE:
 *
 * Disable inlineing of this function when tracing is enabled
 */
#define PHOC_TRACE_NO_INLINE G_NO_INLINE

#else /* ! PHOC_USE_DTRACE */

# define PHOC_DTRACE_PROBE(...)  {}
# define PHOC_DTRACE_PROBE1(...) {}
# define PHOC_DTRACE_PROBE2(...) {}
# define PHOC_DTRACE_PROBE3(...) {}
# define PHOC_DTRACE_PROBE4(...) {}
# define PHOC_DTRACE_PROBE5(...) {}
# define PHOC_DTRACE_PROBE6(...) {}

# define PHOC_TRACE_NO_INLINE /* empty */

#endif /* ! PHOC_USE_DTRACE */

#ifdef PHOC_USE_SYSPROF

#include <sysprof-capture.h>

# define PHOC_TRACE_CURRENT_TIME SYSPROF_CAPTURE_CURRENT_TIME

void phoc_trace_mark (gint64       begin_time_nsec,
                      gint64       duration_nsec,
                      const gchar *group,
                      const gchar *name,
                      const gchar *message_format,
                      ...) G_GNUC_PRINTF (5, 6);
#else
# define PHOC_TRACE_CURRENT_TIME 0
/* Optimize out the call */
# define phoc_trace_mark(b, d, g, n, m, ...)
#endif
