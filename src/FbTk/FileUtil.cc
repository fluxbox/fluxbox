// FileUtil.cc
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "FileUtil.hh"

#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

using std::ifstream;
using std::ofstream;
using std::cerr;
using std::endl;

namespace FbTk {

time_t FileUtil::getLastStatusChangeTimestamp(const char* filename) {
    struct stat buf;
    if (filename && !stat(filename, &buf)) {
        return buf.st_ctime;
    } else 
        return (time_t)-1;
}

bool FileUtil::isDirectory(const char* filename) {
    struct stat buf;
    if (!filename || stat(filename, &buf))
        return false;

    return S_ISDIR(buf.st_mode);
}

bool FileUtil::isRegularFile(const char* filename) {
    struct stat buf;
    if (!filename || stat(filename, &buf))
        return false;

    return S_ISREG(buf.st_mode);
}

bool FileUtil::isExecutable(const char* filename) {
    struct stat buf;
    if (!filename || stat(filename, &buf))
        return false;

    return buf.st_mode & S_IXUSR
#ifdef S_IXGRP
        || buf.st_mode & S_IXGRP
#endif
#ifdef S_IXOTH
        || buf.st_mode & S_IXOTH
#endif
    ;
}

bool FileUtil::copyFile(const char* from, const char* to) {
    ifstream from_file(from);
    ofstream to_file(to);

    if (!to_file.good())
        cerr << "Can't write file '"<<to<<"'."<<endl;
    else if (from_file.good()) {
        to_file<<from_file.rdbuf();
        return true;
    } else
        cerr << "Can't copy from '"<<from<<"' to '"<<to<<"'."<<endl;
    
    return false;
}

Directory::Directory(const char *dir):m_dir(0),
m_num_entries(0) {
    if (dir != 0)
        open(dir);
}

Directory::~Directory() {
    close();
}

void Directory::rewind() {
    if (m_dir != 0)
        rewinddir(m_dir);
}

struct dirent *Directory::read() {
    if (m_dir == 0)
        return 0;

    return readdir(m_dir);
}

std::string Directory::readFilename() {
    dirent *ent = read();
    const char* name = 0;
    if (ent) {
        name = ent->d_name;
    }
    return (name ? name : "");
}

void Directory::close() {
    if (m_dir != 0) { 
        closedir(m_dir);
        m_name = "";
        m_dir = 0;
        m_num_entries = 0;
    }
}


bool Directory::open(const char *dir) {
    if (dir == 0)
        return false;

    if (m_dir != 0)
        close();

    m_dir = opendir(dir);
    if (m_dir == 0) // successfull loading?
        return false;

    m_name= dir;

    // get number of entries
    while (read())
        m_num_entries++;

    rewind(); // go back to start

    return true;
}


} // end namespace FbTk
