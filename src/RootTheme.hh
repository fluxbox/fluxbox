// RootTheme.hh 
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: RootTheme.hh,v 1.2 2003/05/10 13:45:50 fluxgen Exp $

#ifndef ROOTTHEME_HH
#define ROOTTHEME_HH

#include "FbTk/Theme.hh"
#include "Color.hh"

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

    const FbTk::Color &borderColor() const { return *m_border_color; }
    int borderWidth() const { return *m_border_width; }
    int bevelWidth() const { return *m_bevel_width; }
    int handleWidth() const { return *m_handle_width; }
    GC opGC() const { return m_opgc; }

private:
    FbTk::ThemeItem<std::string> m_root_command;
    FbTk::ThemeItem<int> m_border_width, m_bevel_width, m_handle_width;
    FbTk::ThemeItem<FbTk::Color> m_border_color;    
    std::string &m_screen_root_command; ///< string to execute and override theme rootCommand
    GC m_opgc;
};

#endif // ROOTTHEME_HH
