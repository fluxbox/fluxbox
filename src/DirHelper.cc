// DirHelper.cc
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: DirHelper.cc,v 1.2 2003/02/15 01:42:17 fluxgen Exp $

#include "DirHelper.hh"

DirHelper::DirHelper(const char *dir):m_dir(0),
m_num_entries(0) {
    if (dir != 0)
        open(dir);
}

DirHelper::~DirHelper() {
    if (m_dir != 0)
        close();
}

void DirHelper::rewind() {
    if (m_dir != 0)
        rewinddir(m_dir);
}

struct dirent *DirHelper::read() {
    if (m_dir == 0)
        return 0;

    return readdir(m_dir);
}

std::string DirHelper::readFilename() {
    dirent *ent = read();
    if (ent == 0)
        return "";
    return (ent->d_name ? ent->d_name : "");
}

void DirHelper::close() {
    if (m_dir != 0) { 
        closedir(m_dir);
        m_dir = 0;
        m_num_entries = 0;
    }
}


bool DirHelper::open(const char *dir) {
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

