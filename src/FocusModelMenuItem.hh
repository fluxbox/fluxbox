// FocusModelMenuItem.hh for Fluxbox
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef FOCUSMODELMENUITEM_HH
#define FOCUSMODELMENUITEM_HH


#include "FbTk/RadioMenuItem.hh"
#include "FbTk/RefCount.hh"

namespace FbTk {
template <class T> class Command;
}

#include "FocusControl.hh"

class FocusModelMenuItem : public FbTk::RadioMenuItem {
public:
    FocusModelMenuItem(const FbTk::FbString &label, FocusControl &focus_control, 
                       FocusControl::FocusModel model,
                       FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd),
        m_focus_control(focus_control), 
        m_focusmodel(model) {
        setCloseOnClick(false);
    }

    bool isSelected() const { return m_focus_control.focusModel() == m_focusmodel; }

    void click(int button, int time, unsigned int mods) {
        m_focus_control.setFocusModel(m_focusmodel);
        FbTk::RadioMenuItem::click(button, time, mods);
    }

private:
    FocusControl &m_focus_control;
    FocusControl::FocusModel m_focusmodel;
};

class TabFocusModelMenuItem : public FbTk::RadioMenuItem {
public:
    TabFocusModelMenuItem(const FbTk::FbString &label, 
                          FocusControl &focus_control,
                          FocusControl::TabFocusModel model, 
                          FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd),
        m_focus_control(focus_control), 
        m_tabfocusmodel(model) {
        setCloseOnClick(false);
    }

    bool isSelected() const { return m_focus_control.tabFocusModel() == m_tabfocusmodel; }

    void click(int button, int time, unsigned int mods) {
        m_focus_control.setTabFocusModel(m_tabfocusmodel);
        FbTk::RadioMenuItem::click(button, time, mods);
    }

private:
    FocusControl &m_focus_control;
    FocusControl::TabFocusModel m_tabfocusmodel;
};


#endif // FOCUSMODELMENUITEM_HH
