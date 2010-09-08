// MultiButtonMenuItem.hh for FbTk
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef FBTK_MULTIBUTTONMENUITEM_HH
#define FBTK_MULTIBUTTONMENUITEM_HH

#include "MenuItem.hh"

namespace FbTk {

/// Handles commands for the specified numbers of buttons
class MultiButtonMenuItem: public FbTk::MenuItem {
public:
    MultiButtonMenuItem(int buttons, const FbTk::BiDiString& label);  
    MultiButtonMenuItem(int buttons, const FbTk::BiDiString& label, Menu *submenu);
    virtual ~MultiButtonMenuItem();
    /// sets command to specified button
    void setCommand(int button, FbTk::RefCount<FbTk::Command<void> > &cmd);
    /// executes command for the button click
    virtual void click(int button, int time, unsigned int mods);
    /// @return number of buttons this instance handles
    unsigned int buttons() const { return m_buttons; }

private:
    void init(int buttons);

    FbTk::RefCount<FbTk::Command<void> > *m_button_exe;
    unsigned int m_buttons;
};

} // end namespace FbTk

#endif // FBTK_MULTIBUTTONMENUITEM_HH
