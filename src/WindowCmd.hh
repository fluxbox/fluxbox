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
