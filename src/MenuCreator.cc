// MenuCreator.cc for Fluxbox
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: MenuCreator.cc,v 1.5 2004/05/03 21:37:01 fluxgen Exp $

#include "MenuCreator.hh"

#include "Screen.hh"
#include "CommandParser.hh"
#include "fluxbox.hh"
#include "CommandParser.hh"
#include "Window.hh"
#include "I18n.hh"

#include "FbMenu.hh"
#include "IconMenu.hh"
#include "WorkspaceMenu.hh"
#include "LayerMenu.hh"
#include "SendToMenu.hh"

#include "FbMenuParser.hh"
#include "StyleMenuItem.hh"

#include "FbTk/MultiButtonMenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Directory.hh"

#include <iostream>
using namespace std;

template <>
void LayerMenuItem<FluxboxWindow>::click(int button, int time) {
    m_object->moveToLayer(m_layernum);
}

static void createStyleMenu(FbTk::Menu &parent, const std::string &label, 
                                   const std::string &directory) {
    // perform shell style ~ home directory expansion
    string stylesdir(FbTk::StringUtil::expandFilename(directory));

    if (!FbTk::Directory::isDirectory(stylesdir))
        return;

    FbTk::Directory dir(stylesdir.c_str());

    // create a vector of all the filenames in the directory
    // add sort it
    std::vector<std::string> filelist(dir.entries());
    for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
        filelist[file_index] = dir.readFilename();

    std::sort(filelist.begin(), filelist.end(), less<string>());

    // for each file in directory add filename and path to menu
    for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
        std::string style(stylesdir + '/' + filelist[file_index]);
        // add to menu only if the file is a regular file, and not a
        // .file or a backup~ file
        if ((FbTk::Directory::isRegularFile(style) &&
             (filelist[file_index][0] != '.') &&
             (style[style.length() - 1] != '~')) ||
            FbTk::Directory::isRegularFile(style + "/theme.cfg"))
            parent.insert(new StyleMenuItem(filelist[file_index], style));
    } 
    // update menu graphics
    parent.update();
    Fluxbox::instance()->saveMenuFilename(stylesdir.c_str());

}

static void translateMenuItem(Parser &parse,
                              const std::string &str_key, 
                              const std::string &str_label, 
                              const std::string &str_cmd,
                              FbTk::Menu &menu);


static void parseMenu(Parser &pars, FbTk::Menu &menu) {
    Parser::Item item, item2, item3;
    while (!pars.eof()) {
        pars>>item>>item2>>item3;
        if (item.second == "end")
            return;
        translateMenuItem(pars,
                          item.second,
                          item2.second,
                          item3.second,
                          menu);

    }
}

static void translateMenuItem(Parser &parse,
                              const std::string &str_key, 
                              const std::string &str_label, 
                              const std::string &str_cmd,
                              FbTk::Menu &menu) {
    
    const int screen_number = menu.screenNumber();
    static I18n &i18n = *I18n::instance();
    using namespace FBNLS;

#define SCREENNLS(a, b) i18n.getMessage(ScreenSet, a, b)

    if (str_key == "end") {
        return;
    } else if (str_key == "nop") { 
        menu.insert(str_label.c_str());
    } else if (str_key == "icons") {
        FbTk::Menu *submenu = MenuCreator::createMenuType("iconmenu", menu.screenNumber());
        if (submenu == 0)
            return;
        if (str_label.empty())
            menu.insert(i18n.getMessage(IconSet, IconIcons, "Icons"));
        else
            menu.insert(str_label.c_str(), submenu);
    } else if (str_key == "exit") { // exit
        FbTk::RefCount<FbTk::Command> exit_cmd(CommandParser::instance().parseLine("exit"));
        if (str_label.empty())
            menu.insert(SCREENNLS(ScreenExit, "Exit"), exit_cmd);
        else
            menu.insert(str_label.c_str(), exit_cmd);
    } else if (str_key == "exec") {
        // execute and hide menu
        using namespace FbTk;
        RefCount<Command> exec_cmd(CommandParser::instance().parseLine("exec " + str_cmd));
        RefCount<Command> hide_menu(new SimpleCommand<FbTk::Menu>(menu, 
                                                                  &Menu::hide));
        MacroCommand *exec_and_hide = new FbTk::MacroCommand();
        exec_and_hide->add(hide_menu);
        exec_and_hide->add(exec_cmd);
        RefCount<Command> exec_and_hide_cmd(exec_and_hide);
        menu.insert(str_label.c_str(), exec_and_hide_cmd);
    }
    else if (str_key == "style") {	// style
        menu.insert(new StyleMenuItem(str_label, str_cmd));
    } else if (str_key == "config") {
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0)
            menu.insert(str_label.c_str(), &screen->configMenu());
    } // end of config                
    else if (str_key == "include") { // include
        string newfile = FbTk::StringUtil::expandFilename(str_label);
        // inject this file into the current menu
        MenuCreator::createFromFile(newfile, menu);
        Fluxbox::instance()->saveMenuFilename(newfile.c_str());
    } // end of include
    else if (str_key == "submenu") {
            
        FbTk::Menu *submenu = MenuCreator::createMenu("", screen_number);
        if (submenu == 0)
            return;

        if (str_cmd.size())
            submenu->setLabel(str_cmd.c_str());
        else
            submenu->setLabel(str_label.c_str());

        parseMenu(parse, *submenu);
        submenu->update();
        menu.insert(str_label.c_str(), submenu);
        // save to screen list so we can delete it later
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0)                    
            screen->saveMenu(*submenu);

    } // end of submenu
    else if (str_key == "restart") {
        FbTk::RefCount<FbTk::Command> restart_fb(CommandParser::instance().
                                                 parseLine("restart"));
        if (str_label.empty())
            menu.insert(SCREENNLS(ScreenRestart, "Restart"), restart_fb);
        else
            menu.insert(str_label.c_str(), restart_fb);
    } // end of restart
    else if (str_key == "reconfig") { // reconf
        //
        //!! TODO: NLS
        //
        FbTk::RefCount<FbTk::Command> 
            reconfig_fb_cmd(CommandParser::instance().
                            parseLine("reconfigure"));
        menu.insert(str_label.c_str(), reconfig_fb_cmd);

    } else if (str_key == "stylesdir" || str_key == "stylesmenu") {
        createStyleMenu(menu, str_label, 
                        str_key == "stylesmenu" ? str_cmd : str_label);
    } // end of stylesdir
    else if (str_key == "workspaces") {
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0) {
            screen->getWorkspacemenu().setInternalMenu();
            menu.insert(str_label.c_str(), &screen->getWorkspacemenu());
        }
    } else if (str_key == "separator") {
        menu.insert("---"); //!! TODO: this will be better in the future
    } else { // ok, if we didn't find any special menu item we try with command parser
        // we need to attach command with arguments so command parser can parse it
        string line = str_key + " " + str_cmd;
        FbTk::RefCount<FbTk::Command> command(CommandParser::instance().parseLine(line));
        if (*command != 0)
            menu.insert(str_label.c_str(), command);
    }
#undef SCREENNLS

}


static void parseWindowMenu(Parser &parse, FbTk::Menu &menu, FluxboxWindow &win) {

    Parser::Item item, item2, item3;
    while (!parse.eof()) {
        parse>>item>>item2>>item3;
        if (MenuCreator::createWindowMenuItem(item.second, item2.second, menu, win))
            continue;

        if (item.second == "end") {
            return;
        } else if (item.second == "submenu") {
            FbTk::Menu *submenu = MenuCreator::createMenu(item2.second, menu.screenNumber());
            parseWindowMenu(parse, *submenu, win);
            submenu->update();
            menu.insert(item2.second.c_str(), submenu);

        } else { // try non window menu specific stuff
            translateMenuItem(parse, item.second, item2.second, item3.second, menu);
        }
    }
}

FbTk::Menu *MenuCreator::createMenu(const std::string &label, int screen_number) {
    BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
    if (screen == 0)
        return 0;

    FbTk::Menu *menu = new FbMenu(screen->menuTheme(), 
                                  screen->imageControl(), 
                                  *screen->layerManager().
                                  getLayer(Fluxbox::instance()->getMenuLayer()));
    if (!label.empty())
        menu->setLabel(label.c_str());

    return menu;
}

bool getStart(FbMenuParser &parser, std::string &label) {
    Parser::Item item, item2, item3;
    while (!parser.eof()) {
        // get first begin line
        parser>>item>>item2>>item3;
        if (item.second == "begin") {
            break;
        }
    }
    if (parser.eof())
        return false;
    label = item2.second;
    return true;
}

FbTk::Menu *MenuCreator::createFromFile(const std::string &filename, int screen_number) {
    std::string real_filename = FbTk::StringUtil::expandFilename(filename);
    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return 0;

    std::string label;
    if (!getStart(parser, label))
        return 0;
    
    FbTk::Menu *menu = createMenu(label, screen_number);
    if (menu != 0)
        parseMenu(parser, *menu);

    return menu;
}


void MenuCreator::createFromFile(const std::string &filename, 
                                 FbTk::Menu &inject_into) {

    std::string real_filename = FbTk::StringUtil::expandFilename(filename);
    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return;

    std::string label;
    if (!getStart(parser, label))
        return;

    parseMenu(parser, inject_into);
}


void MenuCreator::createFromFile(const std::string &filename, 
                                 FbTk::Menu &inject_into, 
                                 FluxboxWindow &win) {
    std::string real_filename = FbTk::StringUtil::expandFilename(filename);
    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return;

    std::string label;

    if (!getStart(parser, label))
        return;

    parseWindowMenu(parser, inject_into, win);
}


FbTk::Menu *MenuCreator::createMenuType(const std::string &type, int screen_num) {
    BScreen *screen = Fluxbox::instance()->findScreen(screen_num);
    if (screen == 0)
        return 0;
    if (type == "iconmenu") {
        return new IconMenu(*screen);
    } else if (type == "workspacemenu") {
        return new WorkspaceMenu(*screen);
    }
    return 0;
}

bool MenuCreator::createWindowMenuItem(const std::string &type, 
                                       const std::string &label,
                                       FbTk::Menu &menu,
                                       FluxboxWindow &win) {
    static I18n &i18n = *I18n::instance();
    typedef FbTk::RefCount<FbTk::Command> RefCmd;
    typedef FbTk::SimpleCommand<FluxboxWindow> WindowCmd;
    using namespace FBNLS;

#define WINDOWNLS(a, b) std::string real_label = label; if (label.empty()) real_label = i18n.getMessage(FBNLS::WindowmenuSet, a, b)

    if (type == "shade") {
        WINDOWNLS(WindowmenuShade, "Shade");
        RefCmd shade_cmd(new WindowCmd(win, &FluxboxWindow::shade));
        menu.insert(real_label.c_str(), shade_cmd);
    } else if (type == "maximize") {
        WINDOWNLS(WindowmenuMaximize, "Maximize");
        RefCmd maximize_cmd(new WindowCmd(win, &FluxboxWindow::maximizeFull));
        RefCmd maximize_vert_cmd(new WindowCmd(win, &FluxboxWindow::maximizeVertical));
        RefCmd maximize_horiz_cmd(new WindowCmd(win, &FluxboxWindow::maximizeHorizontal));
        FbTk::MultiButtonMenuItem *maximize_item = new FbTk::MultiButtonMenuItem(3, real_label.c_str());
        // create maximize item with:
        // button1: Maximize normal
        // button2: Maximize Vertical
        // button3: Maximize Horizontal
        maximize_item->setCommand(1, maximize_cmd);
        maximize_item->setCommand(2, maximize_vert_cmd);
        maximize_item->setCommand(3, maximize_horiz_cmd);
        menu.insert(maximize_item);
    } else if (type == "iconify") {
        WINDOWNLS(WindowmenuIconify, "Iconify");
        RefCmd iconify_cmd(new WindowCmd(win, &FluxboxWindow::iconify));
        menu.insert(real_label.c_str(), iconify_cmd);
    } else if (type == "close") {
        WINDOWNLS(WindowmenuClose, "Close");
        RefCmd close_cmd(new WindowCmd(win, &FluxboxWindow::close));
        menu.insert(real_label.c_str(), close_cmd);
    } else if (type == "lower") {
        WINDOWNLS(WindowmenuLower, "Lower");
        RefCmd lower_cmd(new WindowCmd(win, &FluxboxWindow::lower));
        menu.insert(real_label.c_str(), lower_cmd);
    } else if (type == "raise") {
        WINDOWNLS(WindowmenuRaise, "Raise");
        RefCmd raise_cmd(new WindowCmd(win, &FluxboxWindow::raise));
        menu.insert(real_label.c_str(), raise_cmd);
    } else if (type == "stick") {
        WINDOWNLS(WindowmenuStick, "Stick");
        RefCmd stick_cmd(new WindowCmd(win, &FluxboxWindow::stick));
        menu.insert(real_label.c_str(), stick_cmd);
    } else if (type == "extramenus") {
        FluxboxWindow::ExtraMenus::iterator it = win.extraMenus().begin();
        FluxboxWindow::ExtraMenus::iterator it_end = win.extraMenus().end();
        for (; it != it_end; ++it) {
            it->second->disableTitle();
            menu.insert(it->first, it->second);
        }

    } else if (type == "sendto") {
        WINDOWNLS(WindowmenuSendTo, "Send To ...");
        menu.insert(real_label.c_str(), new SendToMenu(win));
    } else if (type == "layer") {
        WINDOWNLS(WindowmenuLayer, "Layer ...");
        BScreen *screen = Fluxbox::instance()->findScreen(menu.screenNumber());
        if (screen == 0)
            return false;
        FbTk::Menu *submenu = new LayerMenu<FluxboxWindow>(screen->menuTheme(), 
                                                           screen->imageControl(), 
                                                           *screen->layerManager().
                                                           getLayer(Fluxbox::instance()->getMenuLayer()), 
                                                           &win,
                                                           false);
        submenu->disableTitle();
        menu.insert(real_label.c_str(), submenu);


    } else if (type == "separator") {
        menu.insert("---");
    } else
        return false;
#undef WINDOWNLS

    return true;
}
