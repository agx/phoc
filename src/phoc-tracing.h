/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include <glib.h>

#pragma once

#ifdef PHOC_USE_DTRACE

# include <sys/sdt.h>

#else
# define DTRACE_PROBE(...)  {}
# define DTRACE_PROBE1(...) {}
# define DTRACE_PROBE2(...) {}
# define DTRACE_PROBE3(...) {}
# define DTRACE_PROBE4(...) {}
# define DTRACE_PROBE5(...) {}
# define DTRACE_PROBE6(...) {}
#endif
