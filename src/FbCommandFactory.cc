// FbCommandFactory.cc for Fluxbox Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: FbCommandFactory.cc,v 1.11 2003/07/28 12:05:27 fluxgen Exp $

#include "FbCommandFactory.hh"

#include "CurrentWindowCmd.hh"
#include "FbCommands.hh"
#include "Window.hh"
#include "WorkspaceCmd.hh"
#include "fluxbox.hh"
#include "SimpleCommand.hh"
#include "Screen.hh"

// autoregister this module to command parser
FbCommandFactory FbCommandFactory::s_autoreg;


FbCommandFactory::FbCommandFactory() {
    // setup commands that we can handle
    const char commands[][30] = {
        "setstyle",
        "saverc",
        "reconfigure",
        "execcommand",
        "exec",
        "execute",
        "quit",
        "restart",
        "minimizewindow",
        "minimize",
        "iconfiy",
        "maximizewindow",
        "maximize",
        "maximizevertical",
        "maximizehorizontal",
        "moveright",
        "moveleft",
        "moveup",
        "movedown",
        "raise",
        "lower",
        "close",
        "shade",
        "shadewindow",
        "stick",
        "stickwindow",
        "toggledecor",
        "sendtoworkspace",
        "killwindow",
        "nexttab",
        "prevtab",
        "movetableft",
        "movetabright",
        "detachclient",
        "nextworkspace",
        "rightworkspace",
        "leftworkspace",
        "prevworkspace",
        "workspace",
        "nextwindow",
        "prevwindow",
        "nextgroup",
        "prevgroup",
        "showdesktop",
        "arrangewindows",
        "rootmenu",
        ""
    };

    for (int i=0;; ++i) {
        if (strcmp(commands[i], "") == 0)
            break;
        addCommand(commands[i]);
    }
                     
}

FbTk::Command *FbCommandFactory::stringToCommand(const std::string &command,
                                                 const std::string &arguments) {
    using namespace FbCommands;
    //
    // WM commands
    //
    if (command == "restart")
        return new RestartFluxboxCmd(arguments);
    else if (command == "reconfigure")
        return new ReconfigureFluxboxCmd();
    else if (command == "setstyle")
        return new SetStyleCmd(arguments);
    else if (command == "saverc")
        return new SaveResources();
    else if (command == "execcommand" || command == "execute" || command == "exec")
        return new ExecuteCmd(arguments); // execute command on key screen
    else if (command == "quit")
        return new FbTk::SimpleCommand<Fluxbox>(*Fluxbox::instance(), &Fluxbox::shutdown);
    //
    // Current focused window commands
    //
    else if (command == "minimizewindow" || command == "minimize" || command == "iconify")
        return new CurrentWindowCmd(&FluxboxWindow::iconify);
    else if (command == "maximizewindow" || command == "maximize")
        return new CurrentWindowCmd(&FluxboxWindow::maximizeFull);
    else if (command == "maximizevertical")
        return new CurrentWindowCmd(&FluxboxWindow::maximizeVertical);
    else if (command == "maximizehorizontal")
        return new CurrentWindowCmd(&FluxboxWindow::maximizeHorizontal);
    else if (command == "moveright")
        return new MoveRightCmd(atoi(arguments.c_str()));
    else if (command == "moveleft")
        return new MoveLeftCmd(atoi(arguments.c_str()));    
    else if (command == "moveup")
        return new MoveUpCmd(atoi(arguments.c_str()));
    else if (command == "movedown")
        return new MoveDownCmd(atoi(arguments.c_str()));
    else if (command == "raise")
        return new CurrentWindowCmd(&FluxboxWindow::raise);
    else if (command == "lower")
        return new CurrentWindowCmd(&FluxboxWindow::lower);
    else if (command == "close")
        return new CurrentWindowCmd(&FluxboxWindow::close);
    else if (command == "shade" || command == "shadewindow")
        return new CurrentWindowCmd(&FluxboxWindow::shade);
    else if (command == "stick" || command == "stickwindow")
        return new CurrentWindowCmd(&FluxboxWindow::stick);
    else if (command == "toggledecor")
        return new CurrentWindowCmd(&FluxboxWindow::toggleDecoration);
    else if (command == "sendtoworkspace")
        return new SendToWorkspaceCmd(atoi(arguments.c_str()) - 1); // make 1-indexed to user
    else if (command == "killwindow")
        return new KillWindowCmd();
     else if (command == "nexttab")
        return new CurrentWindowCmd(&FluxboxWindow::nextClient);
    else if (command == "prevtab")
        return new CurrentWindowCmd(&FluxboxWindow::prevClient);
    else if (command == "movetableft")
        return new CurrentWindowCmd(&FluxboxWindow::moveClientLeft);
    else if (command == "movetabright")
        return new CurrentWindowCmd(&FluxboxWindow::moveClientRight);
    else if (command == "detachclient")
        return new CurrentWindowCmd(&FluxboxWindow::detachCurrentClient);
    // 
    // Workspace commands
    //
    else if (command == "nextworkspace" && arguments.size() == 0)
        return new NextWorkspaceCmd();
    else if (command == "prevworkspace" && arguments.size() == 0)
        return new PrevWorkspaceCmd();
    else if (command == "rightworkspace")
        return new RightWorkspaceCmd(atoi(arguments.c_str()));
    else if (command == "leftworkspace")
        return new LeftWorkspaceCmd(atoi(arguments.c_str()));
    else if (command == "workspace") {
        int num = 1; // workspaces appear 1-indexed to the user
        if (!arguments.empty())
            num = atoi(arguments.c_str());
        return new JumpToWorkspaceCmd(num-1);
    } else if (command == "nextwindow")
        return new NextWindowCmd(atoi(arguments.c_str()));
    else if (command == "prevwindow")
        return new PrevWindowCmd(atoi(arguments.c_str()));
    else if (command == "nextgroup")
        return new NextWindowCmd(atoi(arguments.c_str()) ^ BScreen::CYCLEGROUPS);
    else if (command == "prevgroup")
        return new PrevWindowCmd(atoi(arguments.c_str()) ^ BScreen::CYCLEGROUPS);
    else if (command == "arrangewindows")
        return new ArrangeWindowsCmd();
    else if (command == "showdesktop")
        return new ShowDesktopCmd();
    else if (command == "rootmenu")
        return new ShowRootMenuCmd();
    return 0;
}
