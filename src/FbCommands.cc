// FbCommands.cc for Fluxbox
// Copyright (c) 2003 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "FbCommands.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "CommandDialog.hh"
#include "FocusControl.hh"
#include "Workspace.hh"
#include "Window.hh"
#include "Keys.hh"
#include "MenuCreator.hh"

#include "FbTk/Theme.hh"
#include "FbTk/Menu.hh"
#include "FbTk/CommandParser.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/stringstream.hh"

#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <set>
#include <cstdlib>
#include <cstring>

#if defined(__EMX__) && defined(HAVE_PROCESS_H)
#include <process.h> // for P_NOWAIT
#endif // __EMX__

using std::string;
using std::pair;
using std::set;
using std::ofstream;
using std::endl;
using std::ios;

namespace {

void showMenu(BScreen &screen, FbTk::Menu &menu) {

    // check if menu has changed
    if (typeid(menu) == typeid(FbMenu)) {
        FbMenu *fbmenu = static_cast<FbMenu *>(&menu);
        if (fbmenu->reloadHelper())
            fbmenu->reloadHelper()->checkReload();
    }

    FbMenu::setWindow(FocusControl::focusedFbWindow());

    union {
        Window w;
        int i;
        unsigned int ui;
    } ignored;

    int x = 0;
    int y = 0;

    XQueryPointer(menu.fbwindow().display(),
                  screen.rootWindow().window(), &ignored.w, &ignored.w,
                  &x, &y, &ignored.i, &ignored.i, &ignored.ui);

    int head = screen.getHead(x, y);
	const bool mouseInStrut = y < static_cast<signed>(screen.maxTop(head)) ||
                              y > static_cast<signed>(screen.maxBottom(head)) ||
                              x < static_cast<signed>(screen.maxLeft(head)) ||
                              x > static_cast<signed>(screen.maxRight(head));

    screen.placementStrategy().placeAndShowMenu(menu, x, y, mouseInStrut);
}

}

namespace FbCommands {

using FbTk::Command;

REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(exec, FbCommands::ExecuteCmd, void);
REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(execute, FbCommands::ExecuteCmd, void);
REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(execcommand, FbCommands::ExecuteCmd, void);

ExecuteCmd::ExecuteCmd(const string &cmd, int screen_num):m_cmd(cmd), m_screen_num(screen_num) {

}

void ExecuteCmd::execute() {
    run();
}

int ExecuteCmd::run() {
#if defined(__EMX__) || defined(_WIN32)
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
    char comspec[PATH_MAX] = {0};
    char * env_var = getenv("COMSPEC");
    if (env_var != NULL) {
        strncpy(comspec, env_var, PATH_MAX - 1);
        comspec[PATH_MAX - 1] = '\0';
    } else {
        strncpy(comspec, "cmd.exe", 7);
        comspec[7] = '\0';
    }

    return spawnlp(P_NOWAIT, comspec, comspec, "/c", m_cmd.c_str(), static_cast<void*>(NULL));
#else
    pid_t pid = fork();
    if (pid)
        return pid;

    // 'display' is given as 'host:number.screen'. we want to give the
    // new app a good home, so we remove '.screen' from what is given
    // us from the xserver and replace it with the screen_num of the Screen
    // the user currently points at with the mouse
    string display = DisplayString(FbTk::App::instance()->display());
    int screen_num = m_screen_num;
    if (screen_num < 0) {
        if (Fluxbox::instance()->mouseScreen() == 0)
            screen_num = 0;
        else
            screen_num = Fluxbox::instance()->mouseScreen()->screenNumber();
    }

    // strip away the '.screen'
    size_t dot = display.rfind(':');
    dot = display.find('.', dot);
    if (dot != string::npos) { // 'display' has actually a '.screen' part
        display.erase(dot);
    }
    display += '.';
    display += FbTk::StringUtil::number2String(screen_num);

    FbTk::App::setenv("DISPLAY", display.c_str());

    // get shell path from the environment
    // this process exits immediately, so we don't have to worry about memleaks
    const char *shell = getenv("SHELL");
    if (!shell)
        shell = "/bin/sh";

    setsid();
    execl(shell, shell, "-c", m_cmd.c_str(), static_cast<void*>(NULL));
    exit(EXIT_SUCCESS);

    return pid; // compiler happy -> we are happy ;)
#endif
}

FbTk::Command<void> *ExportCmd::parse(const string &command, const string &args,
                                bool trusted) {
    string name = args;
    FbTk::StringUtil::removeFirstWhitespace(name);
    if (command != "setresourcevalue")
        FbTk::StringUtil::removeTrailingWhitespace(name);
    size_t pos = name.find_first_of(command == "export" ? "=" : " \t");
    if (pos == string::npos || pos == name.size())
        return 0;

    string value = name.substr(pos + 1);
    name = name.substr(0, pos);
    if (command == "setresourcevalue")
        return new SetResourceValueCmd(name, value);
    return new ExportCmd(name, value);
}

REGISTER_COMMAND_PARSER(setenv, ExportCmd::parse, void);
REGISTER_COMMAND_PARSER(export, ExportCmd::parse, void);
REGISTER_COMMAND_PARSER(setresourcevalue, ExportCmd::parse, void);

ExportCmd::ExportCmd(const string& name, const string& value) :
    m_name(name), m_value(value) {
}

void ExportCmd::execute() {

    FbTk::App::instance()->setenv(m_name.c_str(), m_value.c_str());
}

REGISTER_COMMAND(exit, FbCommands::ExitFluxboxCmd, void);
REGISTER_COMMAND(quit, FbCommands::ExitFluxboxCmd, void);

void ExitFluxboxCmd::execute() {
    Fluxbox::instance()->shutdown();
}

REGISTER_COMMAND(saverc, FbCommands::SaveResources, void);

void SaveResources::execute() {
    Fluxbox::instance()->save_rc();
}

REGISTER_COMMAND_PARSER(restart, RestartFluxboxCmd::parse, void);

FbTk::Command<void> *RestartFluxboxCmd::parse(const string &command,
        const string &args, bool trusted) {
    if (!trusted && !args.empty())
        return 0;
    return new RestartFluxboxCmd(args);
}

RestartFluxboxCmd::RestartFluxboxCmd(const string &cmd):m_cmd(cmd){
}

void RestartFluxboxCmd::execute() {
    Fluxbox::instance()->restart(m_cmd.c_str());
}

REGISTER_COMMAND(reconfigure, FbCommands::ReconfigureFluxboxCmd, void);
REGISTER_COMMAND(reconfig, FbCommands::ReconfigureFluxboxCmd, void);

void ReconfigureFluxboxCmd::execute() {
    Fluxbox::instance()->reconfigure();
}

REGISTER_COMMAND(reloadstyle, FbCommands::ReloadStyleCmd, void);

void ReloadStyleCmd::execute() {
    SetStyleCmd cmd(Fluxbox::instance()->getStyleFilename());
    cmd.execute();
}

REGISTER_COMMAND_WITH_ARGS(setstyle, FbCommands::SetStyleCmd, void);

SetStyleCmd::SetStyleCmd(const string &filename):m_filename(filename) {

}

void SetStyleCmd::execute() {
    if (FbTk::ThemeManager::instance().load(m_filename,
        Fluxbox::instance()->getStyleOverlayFilename())) {
        Fluxbox::instance()->reconfigThemes();
        Fluxbox::instance()->saveStyleFilename(m_filename.c_str());
        Fluxbox::instance()->save_rc();
    }
}

REGISTER_COMMAND_WITH_ARGS(keymode, FbCommands::KeyModeCmd, void);

KeyModeCmd::KeyModeCmd(const string &arguments):m_keymode(arguments),m_end_args("None Escape") {
    string::size_type second_pos = m_keymode.find_first_of(" \t", 0);
    if (second_pos != string::npos) {
        // ok we have arguments, parsing them here
        m_end_args = m_keymode.substr(second_pos);
        m_keymode.erase(second_pos); // remove argument from command
    }
    if (m_keymode != "default")
        Fluxbox::instance()->keys()->addBinding(m_keymode + ": " + m_end_args + " :keymode default");
}

void KeyModeCmd::execute() {
    Fluxbox::instance()->keys()->keyMode(m_keymode);
}

REGISTER_COMMAND(hidemenus, FbCommands::HideMenuCmd, void);

void HideMenuCmd::execute() {
    FbTk::Menu::hideShownMenu();
}

FbTk::Command<void> *ShowClientMenuCmd::parse(const string &command,
                                        const string &args, bool trusted) {
    int opts;
    string pat;
    FocusableList::parseArgs(args, opts, pat);
    return new ShowClientMenuCmd(opts, pat);
}

REGISTER_COMMAND_PARSER(clientmenu, ShowClientMenuCmd::parse, void);

void ShowClientMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    // TODO: ClientMenu only accepts lists of FluxboxWindows for now
    //       when that's fixed, use a FocusableList for m_list
    const FocusableList *list =
        FocusableList::getListFromOptions(*screen, m_option);
    m_list.clear();
    FocusControl::Focusables::const_iterator it = list->clientList().begin(),
                                             it_end = list->clientList().end();
    for (; it != it_end; ++it) {
        Focusable* f = *it;
        if (typeid(*f) == typeid(FluxboxWindow) && m_pat.match(*f))
            m_list.push_back(static_cast<FluxboxWindow *>(f));
    }

    m_menu.reset(new ClientMenu(*screen, m_list,
                                false)); // dont listen to list changes
    ::showMenu(*screen, *m_menu.get());
}

REGISTER_COMMAND_WITH_ARGS(custommenu, FbCommands::ShowCustomMenuCmd, void);

ShowCustomMenuCmd::ShowCustomMenuCmd(const string &arguments) : custom_menu_file(arguments) {}

void ShowCustomMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    if (!m_menu.get() || screen->screenNumber() != m_menu->screenNumber()) {
        m_menu.reset(MenuCreator::createMenu("", *screen));
        m_menu->setReloadHelper(new FbTk::AutoReloadHelper());
        m_menu->reloadHelper()->setReloadCmd(FbTk::RefCount<FbTk::Command<void> >(new FbTk::SimpleCommand<ShowCustomMenuCmd>(*this, &ShowCustomMenuCmd::reload)));
        m_menu->reloadHelper()->setMainFile(custom_menu_file);
    } else
        m_menu->reloadHelper()->checkReload();

    ::showMenu(*screen, *m_menu.get());
}

void ShowCustomMenuCmd::reload() {
    m_menu->removeAll();
    m_menu->setLabel(FbTk::BiDiString(""));
    MenuCreator::createFromFile(custom_menu_file, *m_menu.get(), m_menu->reloadHelper());
}

REGISTER_COMMAND(rootmenu, FbCommands::ShowRootMenuCmd, void);

void ShowRootMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    ::showMenu(*screen, screen->rootMenu());
}

REGISTER_COMMAND(workspacemenu, FbCommands::ShowWorkspaceMenuCmd, void);

void ShowWorkspaceMenuCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    ::showMenu(*screen, screen->workspaceMenu());
}

REGISTER_COMMAND_WITH_ARGS(setworkspacename, FbCommands::SetWorkspaceNameCmd, void);

SetWorkspaceNameCmd::SetWorkspaceNameCmd(const string &name, int spaceid):
    m_name(name), m_workspace(spaceid) {
    if (name.empty())
        m_name = "empty";
}

void SetWorkspaceNameCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0) {
        screen = Fluxbox::instance()->keyScreen();
        if (screen == 0)
            return;
    }

    if (m_workspace < 0) {
        screen->currentWorkspace()->setName(m_name);
    } else {
        Workspace *space = screen->getWorkspace(m_workspace);
        if (space == 0)
            return;
        space->setName(m_name);
    }
}

REGISTER_COMMAND(setworkspacenamedialog, FbCommands::WorkspaceNameDialogCmd, void);

void WorkspaceNameDialogCmd::execute() {

    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    CommandDialog *win = new CommandDialog(*screen, "Set Workspace Name:", "SetWorkspaceName ");
    win->setText(screen->currentWorkspace()->name());
    win->show();
}

REGISTER_COMMAND(commanddialog, FbCommands::CommandDialogCmd, void);

void CommandDialogCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    FbTk::FbWindow *win = new CommandDialog(*screen, "Fluxbox Command");
    win->show();
}


SetResourceValueCmd::SetResourceValueCmd(const string &resname,
                                         const string &value):
    m_resname(resname),
    m_value(value) {

}

void SetResourceValueCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;
    screen->resourceManager().setResourceValue(m_resname, m_value);
    Fluxbox::instance()->save_rc();
}

REGISTER_COMMAND(setresourcevaluedialog, FbCommands::SetResourceValueDialogCmd, void);

void SetResourceValueDialogCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    FbTk::FbWindow *win = new CommandDialog(*screen,  "Type resource name and the value", "SetResourceValue ");
    win->show();
}

REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(bindkey, FbCommands::BindKeyCmd, void);

BindKeyCmd::BindKeyCmd(const string &keybind):m_keybind(keybind) { }

void BindKeyCmd::execute() {
    Keys* keys = Fluxbox::instance()->keys();
    if (keys && keys->addBinding(m_keybind)) {
        ofstream ofile(keys->filename().c_str(), ios::app);
        if (!ofile)
            return;
        ofile<<m_keybind<<endl;
    }
}

FbTk::Command<void> *DeiconifyCmd::parse(const string &command, const string &args,
                                   bool trusted) {
    FbTk_istringstream iss(args.c_str());
    string mode;
    string d;
    Destination dest;

    iss >> mode;
    if (iss.fail())
        mode="lastworkspace";
    mode= FbTk::StringUtil::toLower(mode);

    iss >> d;
    if (iss.fail())
        d="current";
    d = FbTk::StringUtil::toLower(d);
    if (d == "origin" )
        dest = ORIGIN;
    else if (d == "originquiet")
        dest = ORIGINQUIET;
    else
        dest = CURRENT;

    if (mode == "all")
        return new DeiconifyCmd(DeiconifyCmd::ALL, dest);
    else if (mode == "allworkspace")
        return new DeiconifyCmd(DeiconifyCmd::ALLWORKSPACE, dest);
    else if (mode == "last")
        return new DeiconifyCmd(DeiconifyCmd::LAST, dest);
    // lastworkspace, default
    return new DeiconifyCmd(DeiconifyCmd::LASTWORKSPACE, dest);
}

REGISTER_COMMAND_PARSER(deiconify, DeiconifyCmd::parse, void);

DeiconifyCmd::DeiconifyCmd(Mode mode,
                           Destination dest) : m_mode(mode), m_dest(dest) { }

void DeiconifyCmd::execute() {
    BScreen *screen = Fluxbox::instance()->mouseScreen();
    if (screen == 0)
        return;

    // we need to make a copy of the list of icons, or else our iterator can
    // become invalid
    BScreen::Icons icon_list = screen->iconList();
    BScreen::Icons::reverse_iterator it = icon_list.rbegin();
    BScreen::Icons::reverse_iterator itend= icon_list.rend();
    unsigned int workspace_num= screen->currentWorkspaceID();
    unsigned int old_workspace_num;

    const bool change_ws= m_dest == ORIGIN;

    switch(m_mode) {

    case ALL:
    case ALLWORKSPACE:
        for(; it != itend; ++it) {
            old_workspace_num= (*it)->workspaceNumber();
            if (m_mode == ALL || old_workspace_num == workspace_num ||
                (*it)->isStuck()) {
                if (m_dest == ORIGIN || m_dest == ORIGINQUIET)
                    screen->sendToWorkspace(old_workspace_num, (*it), change_ws);
                (*it)->deiconify();
            }
        }
        break;

    case LAST:
    case LASTWORKSPACE:
    default:
        for (; it != itend; ++it) {
            old_workspace_num= (*it)->workspaceNumber();
            if(m_mode == LAST || old_workspace_num == workspace_num ||
               (*it)->isStuck()) {
                if ((m_dest == ORIGIN || m_dest == ORIGINQUIET) &&
                    m_mode != LASTWORKSPACE)
                    screen->sendToWorkspace(old_workspace_num, (*it), change_ws);
                else
                    (*it)->deiconify();
                break;
            }
        }
        break;
    };
}


REGISTER_COMMAND_WITH_ARGS(clientpatterntest, FbCommands::ClientPatternTestCmd, void);

void ClientPatternTestCmd::execute() {

    std::vector< const FluxboxWindow* > matches;
    std::string                         result;
    std::string                         pat;
    int                                 opts;
    Display*                            dpy;
    Atom                                atom_utf8;
    Atom                                atom_fbcmd_result;
    Fluxbox::ScreenList::const_iterator screen;
    const Fluxbox::ScreenList           screens(Fluxbox::instance()->screenList());

    dpy = Fluxbox::instance()->display();
    atom_utf8 = XInternAtom(dpy, "UTF8_STRING", False);
    atom_fbcmd_result = XInternAtom(dpy, "_FLUXBOX_ACTION_RESULT", False);

    FocusableList::parseArgs(m_args, opts, pat);
    ClientPattern cp(pat.c_str());

    if (!cp.error()) {

        const FocusableList*                        windows;
        FocusControl::Focusables::const_iterator    wit;
        FocusControl::Focusables::const_iterator    wit_end;

        for (screen = screens.begin(); screen != screens.end(); screen++) {

            windows = FocusableList::getListFromOptions(**screen, opts|FocusableList::LIST_GROUPS);
            wit = windows->clientList().begin();
            wit_end = windows->clientList().end();

            for ( ; wit != wit_end; wit++) {
                Focusable* f = *wit;
                if (typeid(*f) == typeid(FluxboxWindow) && cp.match(*f)) {
                    matches.push_back(static_cast<const FluxboxWindow*>(f));
                }
            }
        }

        if (!matches.empty()) {
            std::vector< const FluxboxWindow* >::const_iterator win;
            for (win = matches.begin(); win != matches.end(); win++) {
                result += "0x";
                result += FbTk::StringUtil::number2HexString((*win)->clientWindow());
                result += "\t";
                result += (*win)->title().logical();
                result += "\n";
            }
        } else {
            result += "0\n";
        }
    } else {
        result = "-1\t";
        result += FbTk::StringUtil::number2String(cp.error_col());
        result += "\n";
    }


    // write result to _FLUXBOX_ACTION_RESULT property
    for (screen = screens.begin(); screen != screens.end(); screen++) {
        (*screen)->rootWindow().changeProperty(atom_fbcmd_result, atom_utf8, 8,
            PropModeReplace, (unsigned char*)result.c_str(), result.size());
    }
}


} // end namespace FbCommands
