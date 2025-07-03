/*
 * Copyright (C) 2023 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
