#ifndef DEBUG_HH
#define DEBUG_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef DEBUG
#include <iostream>
#define fbdbg std::cerr<<__FILE__<<"("<<__LINE__<< "): "
#else
#define fbdbg if (false) std::cerr
#endif // DEBUG

#endif // DEBUG_HH
