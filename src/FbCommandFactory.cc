// FbCommandFactory.cc for Fluxbox Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

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
#include "FbTk/stringstream.hh"

#include <string>

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

using std::string;
using std::vector;
using std::cerr;
using std::endl;

// autoregister this module to command parser
FbCommandFactory FbCommandFactory::s_autoreg;

namespace {

static int getint(const char *str, int defaultvalue) {
    sscanf(str, "%d", &defaultvalue);
    return defaultvalue;
}

void parseNextWindowArgs(const string &in, int &opts, string &pat) {
    string options;
    int err = FbTk::StringUtil::getStringBetween(options, in.c_str(), '{', '}');

    // the rest of the string is a ClientPattern
    pat = in.c_str() + err;

    // now parse the options
    vector<string> args;
    FbTk::StringUtil::stringtok(args, options);
    vector<string>::iterator it = args.begin(), it_end = args.end();
    opts = 0;
    for (; it != it_end; ++it) {
        if (strcasecmp((*it).c_str(), "static") == 0)
            opts |= FocusControl::CYCLELINEAR;
        else if (strcasecmp((*it).c_str(), "groups") == 0)
            opts |= FocusControl::CYCLEGROUPS;
    }
}

}; // end anonymous namespace

FbCommandFactory::FbCommandFactory() {
    // setup commands that we can handle
    const char* commands[] = {
	"addworkspace",
        "arrangewindows",
	"attach",
        "bindkey",
        "clientmenu",
        "close",
        "closeallwindows",
        "commanddialog",
        "custommenu",
        "deiconify",
        "detachclient",
        "export",
        "exec",
        "execcommand",
        "execute",
        "exit",
        "focusup",
        "focusdown",
        "focusleft",
        "focusright",
        "fullscreen",
        "gotowindow",
        "hidemenus",
        "iconify",
        "keymode",
        "kill",
        "killwindow",
        "leftworkspace",
        "lower",
        "lowerlayer",
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
        "raiselayer",
        "reconfig",
        "reconfigure",
        "reloadstyle",
	"removelastworkspace",
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
        "setalpha",
        "setenv",
        "sethead",
        "setmodkey",
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
        "tab",
        "taketoworkspace",
        "taketonextworkspace",
        "taketoprevworkspace",
        "togglecmd",
        "toggledecor",
        "typeaheadfocus",
        "windowmenu",
        "workspace",
        /* NOTE: The following are DEPRECATED and subject to removal */
        "workspace1",
        "workspace2",
        "workspace3",
        "workspace4",
        "workspace5",
        "workspace6",
        "workspace7",
        "workspace8",
        "workspace9",
        "workspace10",
        "workspace11",
        "workspace12",
        /* end note */
        "workspacemenu",
        0
    };

    for (int i=0; commands[i]; ++i)
        addCommand(commands[i]);
}

FbTk::Command *FbCommandFactory::stringToCommand(const std::string &command,
        const std::string &arguments, bool trusted) {
    using namespace FbCommands;
    //
    // WM commands
    //
    if (command == "restart" && trusted)
        return new RestartFluxboxCmd(arguments);
    else if (command == "reconfigure" || command == "reconfig")
        return new ReconfigureFluxboxCmd();
    else if (command == "setstyle")
        return new SetStyleCmd(arguments);
    else if (command == "reloadstyle")
        return new ReloadStyleCmd();
    else if (command == "keymode")
        return new KeyModeCmd(arguments);
    else if (command == "saverc")
        return new SaveResources();
    else if (command == "execcommand" || command == "execute" || command == "exec") {
        if (!trusted) return 0;
        return new ExecuteCmd(arguments); // execute command on key screen
    } else if (command == "exit" || command == "quit")
        return new ExitFluxboxCmd();
    else if ((command == "setenv" || command == "export") && trusted) {

        string name = arguments;
        FbTk::StringUtil::removeFirstWhitespace(name);
        FbTk::StringUtil::removeTrailingWhitespace(name);
        size_t pos = name.find_first_of(command == "setenv" ? " \t" : "=");
        if (pos == string::npos || pos == name.size())
            return 0;

        string value = name.substr(pos + 1);
        name = name.substr(0, pos);
        return new ExportCmd(name, value);
    }
    else if (command == "setmodkey") {
        string modkey(arguments);
        FbTk::StringUtil::removeFirstWhitespace(modkey);
        FbTk::StringUtil::removeTrailingWhitespace(modkey);

        return new SetModKeyCmd(modkey);
    }
    else if (command == "commanddialog") // run specified fluxbox command
        return new CommandDialogCmd();
    else if (command == "bindkey" && trusted)
        return new BindKeyCmd(arguments);
    else if (command == "setresourcevalue" && trusted) {
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
        return new SetResourceValueCmd(name, value);
    } else if (command == "setresourcevaluedialog")
        return new SetResourceValueDialogCmd();
    else if (command == "addworkspace")
        return new AddWorkspaceCmd();
    else if (command == "removelastworkspace")
        return new RemoveLastWorkspaceCmd();
    //
    // Current focused window commands
    //
    else if (command == "fullscreen")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new FullscreenCmd()), arguments);
    else if (command == "minimizewindow" || command == "minimize" || command == "iconify")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::iconify)), arguments);
    else if (command == "maximizewindow" || command == "maximize")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::maximizeFull)), arguments);
    else if (command == "maximizevertical")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::maximizeVertical)), arguments);
    else if (command == "maximizehorizontal")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::maximizeHorizontal)), arguments);
    else if (command == "setalpha") {
        typedef vector<string> StringTokens;
        StringTokens tokens;
        FbTk::StringUtil::stringtok<StringTokens>(tokens, arguments);

        int focused, unfocused;
        bool relative, un_rel;

        if (tokens.empty()) { // set default alpha
            focused = unfocused = 256;
            relative = un_rel = false;
        } else {
            relative = un_rel = (tokens[0][0] == '+' || tokens[0][0] == '-');
            focused = unfocused = atoi(tokens[0].c_str());
        }

        if (tokens.size() > 1) { // set different unfocused alpha
            un_rel = (tokens[1][0] == '+' || tokens[1][0] == '-');
            unfocused = atoi(tokens[1].c_str());
        }

        string pat;
        string::size_type pos = arguments.find('(');
        if (pos != string::npos && pos != arguments.size())
            pat = arguments.c_str() + pos;

        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new SetAlphaCmd(focused, relative, unfocused, un_rel)), pat);
    } else if (command == "resize" || command == "resizeto" ||
               command == "resizehorizontal" || command == "resizevertical") {
        FbTk_istringstream is(arguments.c_str());
        int dx = 0, dy = 0;
        is >> dx >> dy;
        if (command == "resizehorizontal")
            dy = 0;
        else if (command == "resizevertical") {
            dy = dx;
            dx = 0;
        }

        string pat;
        string::size_type pos = arguments.find('(');
        if (pos != string::npos && pos != arguments.size())
            pat = arguments.c_str() + pos;

        FbTk::RefCount<WindowHelperCmd> cmd;
        if (command == "resizeto")
            cmd = new ResizeToCmd(dx, dy);
        else
            cmd = new ResizeCmd(dx, dy);

        return new WindowListCmd(cmd, pat);
    } else if (command == "moveto") {
        typedef vector<string> StringTokens;
        StringTokens tokens;
        FbTk::StringUtil::stringtok<StringTokens>(tokens, arguments);

        if (tokens.size() < 2) {
            cerr<<"*** WARNING: missing arguments for MoveTo\n";
            return NULL;
        }

        unsigned int refc = MoveToCmd::UPPER|MoveToCmd::LEFT;
        int dx = 0;
        int dy = 0;

        if (tokens[0][0] == '*')
            refc |= MoveToCmd::IGNORE_X;
        else
            dx = atoi(tokens[0].c_str());

        if (tokens[1][0] == '*' && ! (refc & MoveToCmd::IGNORE_X))
            refc |= MoveToCmd::IGNORE_Y;
        else
            dy = atoi(tokens[1].c_str());

        if (tokens.size() >= 3) {
            tokens[2] = FbTk::StringUtil::toLower(tokens[2]);
            if (tokens[2] == "left" || tokens[2] == "upperleft" || tokens[2] == "lowerleft") {
                refc |= MoveToCmd::LEFT;
                refc &= ~MoveToCmd::RIGHT;
            } else if (tokens[2] == "right" || tokens[2] == "upperright" || tokens[2] == "lowerright") {
                refc |= MoveToCmd::RIGHT;
                refc &= ~MoveToCmd::LEFT;
            }

            if (tokens[2] == "upper" || tokens[2] == "upperleft" || tokens[2] == "upperright") {
                refc |= MoveToCmd::UPPER;
                refc &= ~MoveToCmd::LOWER;
            } else if (tokens[2] == "lower" || tokens[2] == "lowerleft" || tokens[2] == "lowerright") {
                refc |= MoveToCmd::LOWER;
                refc &= ~MoveToCmd::UPPER;
            }
        }

        string pat;
        string::size_type pos = arguments.find('(');
        if (pos != string::npos && pos != arguments.size())
            pat = arguments.c_str() + pos;

        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new MoveToCmd(dx, dy, refc)), pat);
    } else if (command == "move" || command == "moveright" ||
               command == "moveleft" || command == "moveup" ||
               command == "movedown") {
        FbTk_istringstream is(arguments.c_str());
        int dx = 0, dy = 0;
        is >> dx >> dy;

        if (command == "moveright")
            dy = 0;
        else if (command == "moveleft") {
            dy = 0;
            dx = -dx;
        } else if (command == "movedown") {
            dy = dx;
            dx = 0;
        } else if (command == "moveup") {
            dy = -dx;
            dx = 0;
        }
            
        string pat;
        string::size_type pos = arguments.find('(');
        if (pos != string::npos && pos != arguments.size())
            pat = arguments.c_str() + pos;

        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new MoveCmd(dx, dy)), pat);
    } else if (command == "raise")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::raise)), arguments);
    else if (command == "raiselayer")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::raiseLayer)), arguments);
    else if (command == "lower")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::lower)), arguments);
    else if (command == "lowerlayer")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::lowerLayer)), arguments);
    else if (command == "close")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::close)), arguments);
    else if (command == "closeallwindows")
        return new CloseAllWindowsCmd();
    else if (command == "killwindow" || command == "kill")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::kill)), arguments);
    else if (command == "shade" || command == "shadewindow")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::shade)), arguments);
    else if (command == "stick" || command == "stickwindow")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::stick)), arguments);
    else if (command == "toggledecor")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::toggleDecoration)), arguments);
    else if (command == "sethead") {
        int num = 0;
        string pat;
        FbTk_istringstream iss(arguments.c_str());
        iss >> num;
        string::size_type pos = arguments.find('(');
        if (pos != string::npos && pos != arguments.size())
            pat = arguments.c_str() + pos;
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new SetHeadCmd(num)), pat);
    } else if (command == "tab" || command == "sendtonextworkspace" ||
               command == "sendtoprevworkspace" ||
               command == "taketonextworkspace" ||
               command == "taketoprevworkspace" ||
               command == "sendtoworkspace" || command == "taketoworkspace") {
        // workspaces appear 1-indexed to the user, hence the minus 1
        int num = 1;
        string pat;
        FbTk_istringstream iss(arguments.c_str());
        iss >> num;
        string::size_type pos = arguments.find('(');
        if (pos != string::npos && pos != arguments.size())
            pat = arguments.c_str() + pos;
        FbTk::RefCount<WindowHelperCmd> cmd;

        if (command == "tab")
            cmd = new GoToTabCmd(num);
        else if (command == "sendtonextworkspace")
            cmd = new SendToNextWorkspaceCmd(num);
        else if (command == "sendtoprevworkspace")
            cmd = new SendToPrevWorkspaceCmd(num);
        else if (command == "taketonextworkspace")
            cmd = new TakeToNextWorkspaceCmd(num);
        else if (command == "taketoprevworkspace")
            cmd = new TakeToPrevWorkspaceCmd(num);
        else if (command == "sendtoworkspace")
            cmd = new SendToWorkspaceCmd(num-1);
        else
            cmd = new TakeToWorkspaceCmd(num-1);
        return new WindowListCmd(cmd, pat);
    } else if (command == "nexttab")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::nextClient)), arguments);
    else if (command == "prevtab")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::prevClient)), arguments);
    else if (command == "movetableft")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::moveClientLeft)), arguments);
    else if (command == "movetabright")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::moveClientRight)), arguments);
    else if (command == "detachclient")
        return new WindowListCmd(FbTk::RefCount<WindowHelperCmd>(new CurrentWindowCmd(&FluxboxWindow::detachCurrentClient)), arguments);
    else if (command == "windowmenu")
        return new CurrentWindowCmd(&FluxboxWindow::popupMenu);
    //
    // Workspace commands
    //
    else if (command == "nextworkspace")
        return new NextWorkspaceCmd(getint(arguments.c_str(), 1));
    else if (command == "prevworkspace")
        return new PrevWorkspaceCmd(getint(arguments.c_str(), 1));
    else if (command == "rightworkspace")
        return new RightWorkspaceCmd(getint(arguments.c_str(), 1));
    else if (command == "leftworkspace")
        return new LeftWorkspaceCmd(getint(arguments.c_str(), 1));
    else if (command == "workspace")
        // workspaces appear 1-indexed to the user, hence the minus 1
        return new JumpToWorkspaceCmd(getint(arguments.c_str(), 1) - 1);
    else if (command.substr(0, 9) == "workspace" && command[9] >= '0' && command[9] <= '9') {
        cerr<<"*** WARNING: 'Workspace<n>' actions are deprecated! Use 'Workspace <n>' instead"<<endl;
        return new JumpToWorkspaceCmd(getint(command.substr(9).c_str(), 1) - 1);

    } else if (command == "attach") {
        int opts; // not used
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        return new AttachCmd(pat);
    } else if (command == "nextwindow") {
        int opts;
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        return new NextWindowCmd(opts, pat);
    } else if (command == "nextgroup") {
        int opts;
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        opts |= FocusControl::CYCLEGROUPS;
        return new NextWindowCmd(opts, pat);
    } else if (command == "prevwindow") {
        int opts;
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        return new PrevWindowCmd(opts, pat);
    } else if (command == "prevgroup") {
        int opts;
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        opts |= FocusControl::CYCLEGROUPS;
        return new PrevWindowCmd(opts, pat);
    } else if (command == "typeaheadfocus") {
        int opts;
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        return new TypeAheadFocusCmd(opts, pat);
    } else if (command == "gotowindow") {
        int num, opts;
        string args, pat;
        FbTk_istringstream iss(arguments.c_str());
        iss >> num;
        string::size_type pos = arguments.find_first_of("({");
        if (pos != string::npos && pos != arguments.size())
            args = arguments.c_str() + pos;
        parseNextWindowArgs(args, opts, pat);
        return new GoToWindowCmd(num, opts, pat);
    } else if (command == "clientmenu") {
        int opts;
        string pat;
        parseNextWindowArgs(arguments, opts, pat);
        return new ShowClientMenuCmd(opts, pat);
    } else if (command == "focusup")
        return new DirFocusCmd(FocusControl::FOCUSUP);
    else if (command == "focusdown")
        return new DirFocusCmd(FocusControl::FOCUSDOWN);
    else if (command == "focusleft")
        return new DirFocusCmd(FocusControl::FOCUSLEFT);
    else if (command == "focusright")
        return new DirFocusCmd(FocusControl::FOCUSRIGHT);
    else if (command == "arrangewindows")
        return new ArrangeWindowsCmd();
    else if (command == "showdesktop")
        return new ShowDesktopCmd();
    else if (command == "hidemenus")
        return new HideMenuCmd();
    else if (command == "rootmenu")
        return new ShowRootMenuCmd();
    else if (command == "custommenu")
        return new ShowCustomMenuCmd(arguments.c_str());
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

        FbTk_istringstream iss(arguments.c_str());
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
      string cmd;
      int   err= 0;
      int   parse_pos= 0;
      FbTk::MacroCommand* macro= new FbTk::MacroCommand();

      while (true) {
        parse_pos+= err;
        err= FbTk::StringUtil::getStringBetween(cmd, arguments.c_str() +
                                                parse_pos,
                                                '{', '}', " \t\n", true);
        if ( err > 0 ) {
          string c, a;
          string::size_type first_pos =
              FbTk::StringUtil::removeFirstWhitespace(cmd);
          string::size_type second_pos =
              cmd.find_first_of(" \t", first_pos);
          if (second_pos != string::npos) {
            a= cmd.substr(second_pos);
            FbTk::StringUtil::removeFirstWhitespace(a);
            cmd.erase(second_pos);
          }
          c= FbTk::StringUtil::toLower(cmd);

          FbTk::Command* fbcmd= stringToCommand(c,a,trusted);
          if (fbcmd) {
            FbTk::RefCount<FbTk::Command> rfbcmd(fbcmd);
            macro->add(rfbcmd);
          }
        } else
          break;
      }

      if ( macro->size() > 0 )
        return macro;

      delete macro;
    } else if (command == "togglecmd") {
      string cmd;
      int   err= 0;
      int   parse_pos= 0;
      FbTk::ToggleCommand* macro= new FbTk::ToggleCommand();

      while (true) {
        parse_pos+= err;
        err= FbTk::StringUtil::getStringBetween(cmd, arguments.c_str() +
                                                parse_pos,
                                                '{', '}', " \t\n", true);
        if ( err > 0 ) {
          string c, a;
          string::size_type first_pos =
              FbTk::StringUtil::removeFirstWhitespace(cmd);
          string::size_type second_pos=
              cmd.find_first_of(" \t", first_pos);
          if (second_pos != string::npos) {
            a= cmd.substr(second_pos);
            FbTk::StringUtil::removeFirstWhitespace(a);
            cmd.erase(second_pos);
          }
          c= FbTk::StringUtil::toLower(cmd);

          FbTk::Command* fbcmd= stringToCommand(c,a,trusted);
          if (fbcmd) {
            FbTk::RefCount<FbTk::Command> rfbcmd(fbcmd);
            macro->add(rfbcmd);
          }
        } else
          break;
      }

      if ( macro->size() > 0 )
        return macro;

      delete macro;
    }
    return 0;
}
