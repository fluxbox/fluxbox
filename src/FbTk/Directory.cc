// Directory.cc
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

// $Id: Directory.cc,v 1.1 2003/05/18 22:06:59 fluxgen Exp $

#include "Directory.hh"

namespace FbTk {

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
    if (ent == 0)
        return "";
    return (ent->d_name ? ent->d_name : "");
}

void Directory::close() {
    if (m_dir != 0) { 
        closedir(m_dir);
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

    // get number of entries
    while (read())
        m_num_entries++;

    rewind(); // go back to start

    return true;
}

}; // end namespace FbTk
