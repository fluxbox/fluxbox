#ifndef DEBUG_HH
#define DEBUG_HH

#ifdef DEBUG
#include <iostream>
#define fbdbg std::cerr<<__FILE__<<"("<<__LINE__<< "): "
#else
#define fbdbg if (false) std::cerr
#endif // DEBUG

#endif // DEBUG_HH
