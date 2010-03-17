#ifndef DEBUG_HH
#define DEBUG_HH

#include "config.h"
#include <iostream>

#ifdef DEBUG
#define fbdbg std::cerr<<__FILE__<<"("<<__LINE__<< "): "
#else
#define fbdbg if (false) std::cerr
#endif // DEBUG

#endif // DEBUG_HH
