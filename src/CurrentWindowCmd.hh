// CurrentWindowCmd.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen{<a*t>}users.sourceforge.net)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id: CurrentWindowCmd.hh,v 1.2 2003/07/28 15:06:33 rathnor Exp $

#ifndef CURRENTWINDOWCMD_HH
#define CURRENTWINDOWCMD_HH

#include "Command.hh"

class FluxboxWindow;
class WinClient;

/// command that calls FluxboxWindow::<the function> on execute()
/// similar to FbTk::SimpleCommand<T>
class CurrentWindowCmd: public FbTk::Command {
public:
    typedef void (FluxboxWindow::* Action)();
    explicit CurrentWindowCmd(Action action);
    void execute();
private:
    Action m_action;
};

/// helper class for window commands
/// calls real_execute if there's a focused window or a window in button press/release window
class WindowHelperCmd: public FbTk::Command {
public:
    void execute();

protected:

    WinClient &winclient();
    FluxboxWindow &fbwindow();
    virtual void real_execute() = 0;

};

class KillWindowCmd: public WindowHelperCmd {
protected:
    void real_execute();
};

class SendToWorkspaceCmd: public WindowHelperCmd {
public:
    explicit SendToWorkspaceCmd(int workspace_num):m_workspace_num(workspace_num) { }
protected:
    void real_execute();
private:
    const int m_workspace_num;
};

class MoveHelper: public WindowHelperCmd {
public:
    explicit MoveHelper(int step_size):m_step_size(step_size) { }
protected:
    int stepSize() const { return m_step_size; }

private:
    const int m_step_size;
};
/// move window to left
class MoveLeftCmd: public MoveHelper {
public:
    explicit MoveLeftCmd(int step_size);
protected:
    void real_execute();
};

/// move window to right
class MoveRightCmd: public MoveHelper {
public:
    explicit MoveRightCmd(int step_size);
protected:
    void real_execute();
};

/// move window up
class MoveUpCmd: public MoveHelper {
public:
    explicit MoveUpCmd(int step_size);
protected:
    void real_execute();
};

/// move window down
class MoveDownCmd: public MoveHelper {
public:
    explicit MoveDownCmd(int step_size);
protected:
    void real_execute();
};

#endif // CURRENTWINDOWCMD_HH
