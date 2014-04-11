// FileUtil.hh
// Copyright (c) 2002 - 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_FILEUTIL_HH
#define FBTK_FILEUTIL_HH

#ifdef HAVE_CTIME
  #include <ctime>
#else
  #include <time.h>
#endif
#include <sys/types.h>
#include <dirent.h>

#include <string>

#include "NotCopyable.hh"

namespace FbTk {

/// Wrapper for file routines

namespace FileUtil {

    /// @return true if file is a directory
    bool isDirectory(const char* filename);
    /// @return true if a file is a regular file
    bool isRegularFile(const char* filename);
    /// @return true if a file executable for user
    bool isExecutable(const char* filename);

    /// gets timestamp of last status change
    /// @return timestamp
    /// @return -1 (failure)
    time_t getLastStatusChangeTimestamp(const char* filename);

    /// copies file 'from' to 'to'
    bool copyFile(const char* from, const char* to);

} // end of File namespace

///  Wrapper class for DIR * routines
class Directory : private FbTk::NotCopyable {
public:
    explicit Directory(const char *dir = 0);
    ~Directory();
    const std::string &name() const { return m_name; }
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
    std::string m_name;
    DIR *m_dir;
    size_t m_num_entries; ///< number of file entries in directory
};

} // end namespace FbTk

#endif // FBTK_FILEUTIL_HH
