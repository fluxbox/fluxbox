// IntResMenuItem.cc for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: IntResMenuItem.cc,v 1.4 2004/08/31 15:26:38 rathnor Exp $

#include "IntResMenuItem.hh"
#include "PixmapWithMask.hh"
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

namespace {

std::string appendIntValue(const std::string &label, int value) {
    char *buff = new char[label.size() + 16];
    sprintf(buff, "%s:  %d", label.c_str(), value);
    std::string ret(buff);
    delete [] buff;
    return ret;
}

};

IntResMenuItem::IntResMenuItem(const char *label, FbTk::Resource<int> &res, int min_val, int max_val):
    FbTk::MenuItem(label), m_org_label(FbTk::MenuItem::label()),
        m_max(max_val), m_min(min_val), m_res(res) { 
        setLabel(appendIntValue(m_org_label, *m_res).c_str());
}

void IntResMenuItem::click(int button, int time) {
    static int last_time = -201;
    int inc_val = 1;
    // check double click
    //!! TODO: must have some sort of "global" double click time in FbTk
    if (time - last_time <= 200)
        inc_val = 5;

    last_time = time;

    if ((button == 4 || button == 3)&& *m_res < m_max) // scroll up
        (*m_res) += inc_val;
    else if ((button == 5 || button == 1) && *m_res > m_min) // scroll down
        (*m_res) -= inc_val;

    // clamp value
    if (*m_res > m_max)
        *m_res = m_max;
    else if (*m_res < m_min)
        *m_res = m_min;

    // update label
    setLabel(appendIntValue(m_org_label, *m_res).c_str());
    // call other commands
    MenuItem::click(button, time);
}
