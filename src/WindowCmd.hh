// WindowCmd.hh
// Copyright (c) 2005 - 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef WINDOWCMD_HH
#define WINDOWCMD_HH

#include "FbTk/Command.hh"
#include "Window.hh"

/// holds context for WindowCmd
class WindowCmd_base {
public:
    static void setWindow(FluxboxWindow *win) { s_win = win; }
    static FluxboxWindow *window() { return s_win; }
protected:
    static FluxboxWindow *s_win;
};


/// executes action for a dynamic context set in WindowCmd_base
template <typename ReturnType=void>
class WindowCmd: public WindowCmd_base, public FbTk::Command {
public:
    typedef ReturnType (FluxboxWindow::* Action)();
    WindowCmd(Action a):m_action(a) {}
    void execute() { 
        if (window() != 0)
            (*window().*m_action)();
    }
private:
    Action m_action;
};


#endif // WINDOWCMD_HH
