// RootTheme.hh 
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef ROOTTHEME_HH
#define ROOTTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/GContext.hh"

#include <X11/Xlib.h>

#include <string>


/// Contains border color, border size, bevel width and opGC for objects like geometry window in BScreen
class RootTheme: public FbTk::Theme {
public:
    /// constructor
    /// @param screen_num the screen number
    /// @param screen_root_command the string to be executed override theme rootCommand
    RootTheme(int screen_num, std::string &screen_root_command);
    ~RootTheme();

    void reconfigTheme();

    GC opGC() const { return m_opgc.gc(); }

    void setLineAttributes(unsigned int width,
                           int line_style,
                           int cap_style,
                           int join_style) {
        m_opgc.setLineAttributes(width, line_style, cap_style, join_style);
    }
    
    //!! TODO we should need this later
    void lock(bool value) { m_lock = value; }
private:
    FbTk::ThemeItem<std::string> m_root_command;
    std::string &m_screen_root_command; ///< string to execute and override theme rootCommand
    FbTk::GContext m_opgc;
    bool m_lock;
};

#endif // ROOTTHEME_HH
