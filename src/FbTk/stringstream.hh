#ifndef FBTK_STRINGSTREAM_HH
#define FBTK_STRINGSTREAM_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_SSTREAM
#include <sstream>
#define FbTk_istringstream std::istringstream
#elif HAVE_STRSTREAM 
#include <strstream>
#define FbTk_istringstream std::istrstream
#else
#error "You dont have sstream or strstream headers!"
#endif // HAVE_STRSTREAM


#endif // FBTK_STRINGSTREAM_HH
