// XrmDatabaseHelper.hh
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef FBTK_XRMDATABASEHELPER_HH
#define FBTK_XRMDATABASEHELPER_HH

#include <X11/Xresource.h>

/**
  This is a helper for XrmDatabase
  when database goes out of scope
  the XrmDatabase variable will be destroyed.
*/
namespace FbTk {

class XrmDatabaseHelper
{
public:
    XrmDatabaseHelper(char const * filename=0)
        : m_database(0)
    { if (filename != 0) load(filename); }

    ~XrmDatabaseHelper() {
        close();
    }

    /// assignment operator
    XrmDatabaseHelper& operator=(const XrmDatabase& database) {
        if (database == m_database) // assignment to self
            return *this;

        if (m_database!=0)
            close();
        m_database = database; 
        return *this;
    }
    bool load(const char *filename) {
        if (filename == 0)
            return false;
        XrmDatabase db = XrmGetFileDatabase(filename);
        if (db == 0)
            return false;
        close(); // close old database 
        m_database = db; // set new and return true
        return true;
    }
    void close() {
        if (m_database != 0) {
            XrmDestroyDatabase(m_database);
            m_database = 0;
        }
    }

    bool operator == (const XrmDatabase& database) const { return m_database == database; }
    XrmDatabase & operator*() { return m_database; }

private:
    XrmDatabase m_database;
};

} // namespace FbTk

#endif // FBTK_XRMDATABASEHELPER_HH
