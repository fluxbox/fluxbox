// fbpch.hh for Fluxbox - precompiled header
//
// Only used with ./configure --enable-pch: then it is force-included first
// into every compile (see AM_CPPFLAGS in Makefile.am) and precompiled once
// per distinct set of compile flags, so the system headers that nearly every
// file needs are parsed once instead of in each of ~130 files. Without
// --enable-pch, config.h is force-included directly.
//
// Keep this to stable system headers: changing anything included here
// rebuilds everything.

#ifndef FBPCH_HH
#define FBPCH_HH

// config.h first: it defines _GNU_SOURCE & co., which change what the system
// headers declare
#include "config.h"

#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#endif // FBPCH_HH
