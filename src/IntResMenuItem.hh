// IntResMenuItem.hh for Fluxbox Window Manager
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

// $Id$

#ifndef INTRESMENUITEM_HH
#define INTRESMENUITEM_HH

#include "MenuItem.hh"
#include "Resource.hh"

/// Changes an resource integer value between min and max
class IntResMenuItem: public FbTk::MenuItem {
public:
    IntResMenuItem(const char *label, FbTk::Resource<int> &res, int min_val, int max_val);

    void click(int button, int time);

private:
    std::string m_org_label; ///< original label
    const int m_max; ///< maximum value the integer can have
    const int m_min; ///< minimum value the integer can have
    FbTk::Resource<int> &m_res; ///< resource item to be changed
};

#endif // INTRESMENUITEM_HH
