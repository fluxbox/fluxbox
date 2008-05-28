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

#ifndef WINDOWCMD_HH
#define WINDOWCMD_HH

#include "FbTk/Command.hh"
#include "FbTk/Accessor.hh"
#include "Window.hh"
#include "WinClient.hh"

/// holds context for WindowCmd
class WindowCmd_base {
public:
    // some window commands (e.g. close, kill, detach) need to know which client
    // the command refers to, so we store it here as well, in case it is not the
    // current client (selected from workspace menu, for example)
    static void setWindow(FluxboxWindow *win) {
        s_win = win;
        s_client = (win ? &win->winClient() : 0);
    }
    static void setClient(WinClient *client) {
        s_client = client;
        s_win = (client ? client->fbwindow() : 0);
    }
    static FluxboxWindow *window() { return s_win; }
    static WinClient *client() { return s_client; }
protected:
    static FluxboxWindow *s_win;
    static WinClient *s_client;
};


/// executes action for a dynamic context set in WindowCmd_base
template <typename ReturnType=void>
class WindowCmd: public WindowCmd_base, public FbTk::Command<void> {
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
