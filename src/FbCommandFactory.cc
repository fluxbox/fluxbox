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

// $Id: FbCommandFactory.cc,v 1.29 2004/04/22 21:12:34 fluxgen Exp $

#include "FbCommandFactory.hh"

#include "CurrentWindowCmd.hh"
#include "FbCommands.hh"
#include "Window.hh"
#include "WorkspaceCmd.hh"
#include "fluxbox.hh"
#include "SimpleCommand.hh"
#include "Screen.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/MacroCommand.hh"

#include <string>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_SSTREAM
#include <sstream>
#define FB_istringstream istringstream
#elif HAVE_STRSTREAM 
#include <strstream>
#define FB_istringstream istrstream
#else
#error "You dont have sstream or strstream headers!"
#endif // HAVE_STRSTREAM

using namespace std;

// autoregister this module to command parser
FbCommandFactory FbCommandFactory::s_autoreg;


FbCommandFactory::FbCommandFactory() {
    // setup commands that we can handle
    const char* commands[] = {
        "arrangewindows",
        "bindkey",
        "close",
        "commanddialog",
        "deiconify",
        "detachclient",
        "exec",
        "execcommand",
        "execute",
        "iconify",
        "killwindow",
        "leftworkspace",
        "lower",
        "macrocmd",
        "maximize",
        "maximizehorizontal",
        "maximizevertical",
        "maximizewindow",
        "minimize",
        "minimizewindow",
        "moveto",
        "move",
        "movedown",
        "moveleft",
        "moveright",
        "movetableft",
        "movetabright",
        "moveup",
        "nextgroup",
        "nexttab",
        "nextwindow",
        "nextworkspace",
        "prevgroup",
        "prevtab",
        "prevwindow",
        "prevworkspace",
        "quit",
        "raise",
        "reconfigure",
        "reloadstyle",
        "resizeto",
        "resize",
        "resizehorizontal",
        "resizevertical",
        "restart",
        "rightworkspace",
        "rootmenu",
        "saverc",
        "sendtoworkspace",
        "sendtonextworkspace",
        "sendtoprevworkspace",
        "setstyle",
        "setworkspacename",
        "setworkspacenamedialog",
        "setresourcevalue",
        "setresourcevaluedialog",
        "shade",
        "shadewindow",
        "showdesktop",
        "stick",
        "stickwindow",
        "toggledecor",
        "windowmenu",
        "workspace",
        "workspacemenu",
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
    else if (command == "reloadstyle")
        return new ReloadStyleCmd();
    else if (command == "saverc")
        return new SaveResources();
    else if (command == "execcommand" || command == "execute" || command == "exec")
        return new ExecuteCmd(arguments); // execute command on key screen
    else if (command == "quit")
        return new FbTk::SimpleCommand<Fluxbox>(*Fluxbox::instance(), &Fluxbox::shutdown);
    else if (command == "commanddialog") // run specified fluxbox command
        return new CommandDialogCmd();
    else if (command == "bindkey")
        return new BindKeyCmd(arguments);
    else if (command == "setresourcevalue") {
        // we need to parse arguments as:
        // <remove whitespace here><resname><one whitespace><value>
        string name = arguments;
        FbTk::StringUtil::removeFirstWhitespace(name);
        size_t pos = name.find_first_of(" \t");
        // we need an argument to resource name
        if (pos == std::string::npos || pos == name.size())
            return 0;
        // +1 so we only remove the first whitespace
        // i.e so users can set space before workspace name and so on
        string value = name.substr(pos + 1);
        name = name.substr(0, pos);
        cerr<<"Creating SetResourceValue "<<name<<" "<<value<<endl;
        return new SetResourceValueCmd(name, value);
    } else if (command == "setresourcevaluedialog")
        return new SetResourceValueDialogCmd();
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
    else if (command == "resize") {
        FB_istringstream is(arguments.c_str()); 
        int dx = 0, dy = 0;
        is >> dx >> dy;
        return new ResizeCmd(dx, dy);
    }
    else if (command == "resizeto") {
        FB_istringstream is(arguments.c_str());
        int dx = 0, dy = 0;
        is >> dx >> dy;
        return new ResizeToCmd(dx, dy);
    }
    else if (command == "resizehorizontal")
        return new ResizeCmd(atoi(arguments.c_str()),0);
    else if (command == "resizevertical")
        return new ResizeCmd(0,atoi(arguments.c_str()));
    else if (command == "moveto") {
       FB_istringstream is(arguments.c_str());
       int dx = 0, dy = 0;
       is >> dx >> dy;
       return new MoveToCmd(dx,dy);    
    }
    else if (command == "move") {
        FB_istringstream is(arguments.c_str());
        int dx = 0, dy = 0;
        is >> dx >> dy;
        return new MoveCmd(dx, dy);
    }
    else if (command == "moveright")
        return new MoveCmd(atoi(arguments.c_str()),0);
    else if (command == "moveleft")
        return new MoveCmd(-atoi(arguments.c_str()),0);
    else if (command == "moveup")
        return new MoveCmd(0,-atoi(arguments.c_str()));
    else if (command == "movedown")
        return new MoveCmd(0,atoi(arguments.c_str()));
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
    else if (command == "sendtonextworkspace")
        return new SendToNextWorkspaceCmd(atoi(arguments.c_str()));
    else if (command == "sendtoprevworkspace")
        return new SendToPrevWorkspaceCmd(atoi(arguments.c_str()));
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
    else if (command == "windowmenu")
        return new CurrentWindowCmd(&FluxboxWindow::popupMenu);
    // 
    // Workspace commands
    //
    else if (command == "nextworkspace" && arguments.empty())
        return new NextWorkspaceCmd();
    else if (command == "prevworkspace" && arguments.empty())
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
    else if (command == "workspacemenu")
        return new ShowWorkspaceMenuCmd();
    else if (command == "setworkspacename") {
        if (arguments.empty())
            return new SetWorkspaceNameCmd("empty");
        else
            return new SetWorkspaceNameCmd(arguments);
    }
    else if (command == "setworkspacenamedialog")
        return new WorkspaceNameDialogCmd();
    //
    // special commands
    //
    else if (command == "deiconify") {

        FB_istringstream iss(arguments);
        string mode;
        string d;
        DeiconifyCmd::Destination dest;

        iss >> mode;
        if (iss.fail())
            mode="lastworkspace";
        mode= FbTk::StringUtil::toLower(mode);
        
        iss >> d;
        if (iss.fail())
            d="current";
        d= FbTk::StringUtil::toLower(d);
        if (d == "origin" )
          dest= DeiconifyCmd::ORIGIN;
        else if (d == "originquiet")
          dest= DeiconifyCmd::ORIGINQUIET;
        else
          dest= DeiconifyCmd::CURRENT;
          
        if ( mode == "all" )
            return new DeiconifyCmd(DeiconifyCmd::ALL, dest); 
        else if ( mode == "allworkspace" )
            return new DeiconifyCmd(DeiconifyCmd::ALLWORKSPACE, dest);
        else if ( mode == "last" )
            return new DeiconifyCmd(DeiconifyCmd::LAST, dest);
        else // lastworkspace, default
            return new DeiconifyCmd(DeiconifyCmd::LASTWORKSPACE, dest);

    } else if (command == "macrocmd") {
      std::string cmd;
      int   err= 0;
      int   parse_pos= 0;
      FbTk::MacroCommand* macro= new FbTk::MacroCommand();

      while (true) {
        parse_pos+= err;
        err= FbTk::StringUtil::getStringBetween(cmd, arguments.c_str() + parse_pos,
                                              '{', '}', " \t\n", true);
        if ( err > 0 ) {
          std::string c, a;
          std::string::size_type first_pos = FbTk::StringUtil::removeFirstWhitespace(cmd);
          std::string::size_type second_pos= cmd.find_first_of(" \t", first_pos);
          if (second_pos != std::string::npos) {
            a= cmd.substr(second_pos);
            FbTk::StringUtil::removeFirstWhitespace(a);
            cmd.erase(second_pos);
          }
          c= FbTk::StringUtil::toLower(cmd);

          FbTk::Command* fbcmd= stringToCommand(c,a);
          if ( fbcmd ) {
            FbTk::RefCount<FbTk::Command> rfbcmd(fbcmd);
            macro->add(rfbcmd);
          }
        }
        else
          break;
      }

      if ( macro->size() > 0 )
        return macro;

      delete macro;
    }
    return 0;
}
