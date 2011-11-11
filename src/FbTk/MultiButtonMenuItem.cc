// MultiButtonMenuItem.cc for FbTk
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "MultiButtonMenuItem.hh"
#include "PixmapWithMask.hh"
#include "Command.hh"

namespace FbTk {

MultiButtonMenuItem::MultiButtonMenuItem(int buttons, const FbTk::BiDiString &label):
    MenuItem(label),
    m_button_exe(0),
    m_buttons(buttons) {
    init(buttons);
}

MultiButtonMenuItem::MultiButtonMenuItem(int buttons, const FbTk::BiDiString &label, Menu *submenu):
    MenuItem(label, submenu),
    m_button_exe(0),
    m_buttons(buttons) {
    init(buttons);
}

MultiButtonMenuItem::~MultiButtonMenuItem() {
    delete [] m_button_exe;
}

void MultiButtonMenuItem::setCommand(int button, FbTk::RefCount<FbTk::Command<void> > &cmd) {
    if (button <= 0 || button > static_cast<signed>(buttons()) || buttons() == 0)
        return;
    m_button_exe[button - 1] = cmd;
}

void MultiButtonMenuItem::click(int button, int time, unsigned int mods) {
    if (button <= 0 || button > static_cast<signed>(buttons()) || buttons() == 0)
        return;

    if (m_button_exe[button - 1] != 0)
        m_button_exe[button - 1]->execute();
}

void MultiButtonMenuItem::init(int buttons) {
    if (buttons < 0)
        m_buttons = 0;
    else
        m_buttons = buttons;

    if (m_buttons != 0)
        m_button_exe = new FbTk::RefCount<FbTk::Command<void> >[m_buttons];
    else
        m_button_exe = 0;
}

} // end namespace FbTk
