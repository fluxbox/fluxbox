// Directory.hh
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Directory.hh,v 1.1 2003/05/18 22:06:59 fluxgen Exp $

#ifndef FBTK_DIRECTORY_HH
#define FBTK_DIRECTORY_HH

#include "NotCopyable.hh"

#include <sys/types.h>
#include <dirent.h>
#include <string>

namespace FbTk {

///  Wrapper class for DIR * routines
class Directory: private FbTk::NotCopyable {
public:
    explicit Directory(const char *dir = 0);
    ~Directory();
    /// go to start of filelist
    void rewind();
    /// gets next dirent info struct in directory and 
    /// jumps to next directory entry
    struct dirent * read();
    /// reads next filename in directory
    std::string readFilename();
    /// close directory
    void close();    
    /// open directory
    /// @param dir the directory name
    bool open(const char *dir);
    /// @return number of entries in the directory
    size_t entries() const { return m_num_entries; }

private:
    DIR *m_dir;
    size_t m_num_entries; ///< number of file entries in directory
};

};

#endif // FBTK_DIRECTORY_HH
