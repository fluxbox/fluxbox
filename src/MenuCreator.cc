// MenuCreator.cc for Fluxbox
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "MenuCreator.hh"

#include "defaults.hh"
#include "Screen.hh"
#include "CommandParser.hh"
#include "fluxbox.hh"
#include "Window.hh"
#include "WindowCmd.hh"

#include "FbMenu.hh"
#include "IconMenu.hh"
#include "WorkspaceMenu.hh"
#include "LayerMenu.hh"
#include "SendToMenu.hh"
#include "AlphaMenu.hh"
#include "Layer.hh"
#include "BoolMenuItem.hh"

#include "FbMenuParser.hh"
#include "StyleMenuItem.hh"
#include "RootCmdMenuItem.hh"

#include "FbTk/I18n.hh"
#include "FbTk/MultiButtonMenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/MenuIcon.hh"
#include "FbTk/Transparent.hh"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::list;
using std::less;

list<string> MenuCreator::encoding_stack;
list<size_t> MenuCreator::stacksize_stack;

FbTk::StringConvertor MenuCreator::m_stringconvertor(FbTk::StringConvertor::ToFbString);

static void createStyleMenu(FbTk::Menu &parent, const string &label,
                            const string &directory) {
    // perform shell style ~ home directory expansion
    string stylesdir(FbTk::StringUtil::expandFilename(directory));

    if (!FbTk::FileUtil::isDirectory(stylesdir.c_str()))
        return;

    FbTk::Directory dir(stylesdir.c_str());

    // create a vector of all the filenames in the directory
    // add sort it
    vector<string> filelist(dir.entries());
    for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
        filelist[file_index] = dir.readFilename();

    sort(filelist.begin(), filelist.end(), less<string>());

    // for each file in directory add filename and path to menu
    for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
        string style(stylesdir + '/' + filelist[file_index]);
        // add to menu only if the file is a regular file, and not a
        // .file or a backup~ file
        if ((FbTk::FileUtil::isRegularFile(style.c_str()) &&
             (filelist[file_index][0] != '.') &&
             (style[style.length() - 1] != '~')) ||
            FbTk::FileUtil::isRegularFile((style + "/theme.cfg").c_str()) ||
            FbTk::FileUtil::isRegularFile((style + "/style.cfg").c_str()))
            parent.insert(new StyleMenuItem(filelist[file_index], style));
    }
    // update menu graphics
    parent.updateMenu();
    Fluxbox::instance()->saveMenuFilename(stylesdir.c_str());

}

static void createRootCmdMenu(FbTk::Menu &parent, const string &label,
                                const string &directory, const string &cmd) {
    // perform shell style ~ home directory expansion
    string rootcmddir(FbTk::StringUtil::expandFilename(directory));

    if (!FbTk::FileUtil::isDirectory(rootcmddir.c_str()))
        return;

    FbTk::Directory dir(rootcmddir.c_str());

    // create a vector of all the filenames in the directory
    // add sort it
    vector<string> filelist(dir.entries());
    for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
        filelist[file_index] = dir.readFilename();

    sort(filelist.begin(), filelist.end(), less<string>());

    // for each file in directory add filename and path to menu
    for (size_t file_index = 0; file_index < dir.entries(); file_index++) {

        string rootcmd(rootcmddir+ '/' + filelist[file_index]);
        // add to menu only if the file is a regular file, and not a
        // .file or a backup~ file
        if ((FbTk::FileUtil::isRegularFile(rootcmd.c_str()) &&
             (filelist[file_index][0] != '.') &&
             (rootcmd[rootcmd.length() - 1] != '~')))
            parent.insert(new RootCmdMenuItem(filelist[file_index], rootcmd, cmd));
    }
    // update menu graphics
    parent.updateMenu();
    Fluxbox::instance()->saveMenuFilename(rootcmddir.c_str());

}


class ParseItem {
public:
    explicit ParseItem(FbTk::Menu *menu):m_menu(menu) {}

    inline void load(Parser &p, FbTk::StringConvertor &m_labelconvertor) {
        p>>m_key>>m_label>>m_cmd>>m_icon;
        m_label.second = m_labelconvertor.recode(m_label.second);
    }
    inline const string &icon() const { return m_icon.second; }
    inline const string &command() const { return m_cmd.second; }
    inline const string &label() const { return m_label.second; }
    inline const string &key() const { return m_key.second; }
    inline FbTk::Menu *menu() { return m_menu; }
private:
    Parser::Item m_key, m_label, m_cmd, m_icon;
    FbTk::Menu *m_menu;
};

class MenuContext: public LayerObject, public AlphaObject {
public:
    void moveToLayer(int layer_number) {
        if (WindowCmd<void>::window() == 0)
            return;
        WindowCmd<void>::window()->moveToLayer(layer_number);
    }
    int layerNumber() const {
        if (WindowCmd<void>::window() == 0)
            return -1;
        return WindowCmd<void>::window()->layerItem().getLayerNum();
    }

    int getFocusedAlpha() const {
        if (WindowCmd<void>::window() == 0)
            return 255;
        return WindowCmd<void>::window()->getFocusedAlpha();
    }

    int getUnfocusedAlpha() const {
        if (WindowCmd<void>::window() == 0)
            return 255;
        return WindowCmd<void>::window()->getUnfocusedAlpha();
    }

    bool getUseDefaultAlpha() const { 
        if (WindowCmd<void>::window() == 0)
            return true;
        return WindowCmd<void>::window()->getUseDefaultAlpha();
    }

    void setFocusedAlpha(int alpha) {
        if (WindowCmd<void>::window() == 0)
            return;
        WindowCmd<void>::window()->setFocusedAlpha(alpha);
    }

    void setUnfocusedAlpha(int alpha) {
        if (WindowCmd<void>::window() == 0)
            return;
        WindowCmd<void>::window()->setUnfocusedAlpha(alpha);
    }

    void setUseDefaultAlpha(bool use_default) {
        if (WindowCmd<void>::window() == 0)
            return;
        WindowCmd<void>::window()->setUseDefaultAlpha(use_default);
    }

};

static void translateMenuItem(Parser &parse, ParseItem &item, FbTk::StringConvertor &labelconvertor);


static void parseMenu(Parser &pars, FbTk::Menu &menu, FbTk::StringConvertor &label_convertor) {
    ParseItem pitem(&menu);
    while (!pars.eof()) {
        pitem.load(pars, label_convertor);
        if (pitem.key() == "end")
            return;
        translateMenuItem(pars, pitem, label_convertor);
    }
}

static void translateMenuItem(Parser &parse, ParseItem &pitem, FbTk::StringConvertor &labelconvertor) {
    if (pitem.menu() == 0)
        throw string("translateMenuItem: We must have a menu in ParseItem!");

    FbTk::Menu &menu = *pitem.menu();
    const string &str_key = pitem.key();
    const string &str_cmd = pitem.command();
    const string &str_label = pitem.label();

    const int screen_number = menu.screenNumber();
    _FB_USES_NLS;

    if (str_key == "end") {
        return;
    } else if (str_key == "nop") {
        int menuSize = menu.insert(str_label);
        menu.setItemEnabled(menuSize-1, false);
    } else if (str_key == "icons") {
        FbTk::Menu *submenu = MenuCreator::createMenuType("iconmenu", menu.screenNumber());
        if (submenu == 0)
            return;
        if (str_label.empty())
            menu.insert(_FB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"));
        else
            menu.insert(str_label, submenu);
    } else if (str_key == "exit") { // exit
        FbTk::RefCount<FbTk::Command> exit_cmd(CommandParser::instance().parseLine("exit"));
        if (str_label.empty())
            menu.insert(_FB_XTEXT(Menu, Exit, "Exit", "Exit Command"), exit_cmd);
        else
            menu.insert(str_label, exit_cmd);
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
        menu.insert(str_label, exec_and_hide_cmd);
    } else if (str_key == "macrocmd") {
        using namespace FbTk;
        RefCount<Command> macro_cmd(CommandParser::instance().parseLine("macrocmd " + str_cmd));
        RefCount<Command> hide_menu(new SimpleCommand<FbTk::Menu>(menu,
                                                                  &Menu::hide));
        MacroCommand *exec_and_hide = new FbTk::MacroCommand();
        exec_and_hide->add(hide_menu);
        exec_and_hide->add(macro_cmd);
        RefCount<Command> exec_and_hide_cmd(exec_and_hide);
        menu.insert(str_label, exec_and_hide_cmd);
    } else if (str_key == "style") {	// style
        menu.insert(new StyleMenuItem(str_label, str_cmd));
    } else if (str_key == "config") {
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0)
            menu.insert(str_label, &screen->configMenu());
    } // end of config
    else if (str_key == "include") { // include

        // this will make sure we dont get stuck in a loop
        static size_t safe_counter = 0;
        if (safe_counter > 10)
            return;

        safe_counter++;

        string newfile = FbTk::StringUtil::expandFilename(str_label);
        if (FbTk::FileUtil::isDirectory(newfile.c_str())) {
            // inject every file in this directory into the current menu
            FbTk::Directory dir(newfile.c_str());

            vector<string> filelist(dir.entries());
            for (size_t file_index = 0; file_index < dir.entries(); ++file_index)
                filelist[file_index] = dir.readFilename();
            sort(filelist.begin(), filelist.end(), less<string>());

            for (size_t file_index = 0; file_index < dir.entries(); file_index++) {
                string thisfile(newfile + '/' + filelist[file_index]);

                if (FbTk::FileUtil::isRegularFile(thisfile.c_str()) &&
                        (filelist[file_index][0] != '.') &&
                        (thisfile[thisfile.length() - 1] != '~')) {
                    MenuCreator::createFromFile(thisfile, menu, false);
                }
            }

        } else {
            // inject this file into the current menu
            MenuCreator::createFromFile(newfile, menu, false);
        }

        safe_counter--;

    } // end of include
    else if (str_key == "submenu") {

        FbTk::Menu *submenu = MenuCreator::createMenu("", screen_number);
        if (submenu == 0)
            return;

        if (!str_cmd.empty())
            submenu->setLabel(str_cmd);
        else
            submenu->setLabel(str_label);

        parseMenu(parse, *submenu, labelconvertor);
        submenu->updateMenu();
        menu.insert(str_label, submenu);
        // save to screen list so we can delete it later
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0)
            screen->saveMenu(*submenu);

    } // end of submenu
    else if (str_key == "stylesdir" || str_key == "stylesmenu") {
        createStyleMenu(menu, str_label,
                        str_key == "stylesmenu" ? str_cmd : str_label);
    } // end of stylesdir
    else if (str_key == "themesdir" || str_key == "themesmenu") {
        createStyleMenu(menu, str_label,
                        str_key == "themesmenu" ? str_cmd : str_label);
    } // end of themesdir
    else if (str_key == "wallpapers" || str_key == "wallpapermenu" ||
             str_key == "rootcommands") {
         createRootCmdMenu(menu, str_label, str_label,
                          str_cmd == "" ? realProgramName("fbsetbg") : str_cmd);
    } // end of wallpapers
    else if (str_key == "workspaces") {
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0) {
            screen->workspaceMenu().setInternalMenu();
            menu.insert(str_label, &screen->workspaceMenu());
        }
    } else if (str_key == "separator") {
        menu.insert(new FbTk::MenuSeparator());
    } else if (str_key == "encoding") {
        MenuCreator::startEncoding(str_cmd);
    } else if (str_key == "endencoding") {
        MenuCreator::endEncoding();
    }
    else { // ok, if we didn't find any special menu item we try with command parser
        // we need to attach command with arguments so command parser can parse it
        string line = str_key + " " + str_cmd;
        FbTk::RefCount<FbTk::Command> command(CommandParser::instance().parseLine(line));
        if (*command != 0) {
            // special NLS default labels
            if (str_label.empty()) {
                if (str_key == "reconfig" || str_key == "reconfigure") {
                    menu.insert(_FB_XTEXT(Menu, Reconfigure, "Reload Config", "Reload all the configs"), command);
                    return;
                } else if (str_key == "restart") {
                    menu.insert(_FB_XTEXT(Menu, Restart, "Restart", "Restart Command"), command);
                    return;
                }
            }
            menu.insert(str_label, command);
        }
    }
    if (menu.numberOfItems() != 0) {
        FbTk::MenuItem *item = menu.find(menu.numberOfItems() - 1);
        if (item != 0 && !pitem.icon().empty())
            item->setIcon(pitem.icon(), menu.screenNumber());
    }
}


static void parseWindowMenu(Parser &parse, FbTk::Menu &menu, FbTk::StringConvertor &labelconvertor) {

    ParseItem pitem(&menu);
    while (!parse.eof()) {
        pitem.load(parse, labelconvertor);
        if (MenuCreator::createWindowMenuItem(pitem.key(), pitem.label(), menu))
            continue;

        if (pitem.key() == "end") {
            return;
        } else if (pitem.key() == "submenu") {
            FbTk::Menu *submenu = MenuCreator::createMenu(pitem.label(), menu.screenNumber());
            parseWindowMenu(parse, *submenu, labelconvertor);
            submenu->updateMenu();
            menu.insert(pitem.label(), submenu);

        } else { // try non window menu specific stuff
            translateMenuItem(parse, pitem, labelconvertor);
        }
    }
}

FbTk::Menu *MenuCreator::createMenu(const string &label, int screen_number) {
    BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
    if (screen == 0)
        return 0;

    FbTk::Menu *menu = new FbMenu(screen->menuTheme(),
                                  screen->imageControl(),
                                  *screen->layerManager().getLayer(Layer::MENU));
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}

bool getStart(FbMenuParser &parser, string &label, FbTk::StringConvertor &labelconvertor) {
    ParseItem pitem(0);
    while (!parser.eof()) {
        // get first begin line
        pitem.load(parser, labelconvertor);
        if (pitem.key() == "begin") {
            break;
        }
    }
    if (parser.eof())
        return false;

    label = pitem.label();
    return true;
}

FbTk::Menu *MenuCreator::createFromFile(const string &filename, int screen_number, bool require_begin) {
    string real_filename = FbTk::StringUtil::expandFilename(filename);
    Fluxbox::instance()->saveMenuFilename(real_filename.c_str());

    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return 0;

    startFile();
    string label;
    if (require_begin && !getStart(parser, label, m_stringconvertor)) {
        endFile();
        return 0;
    }

    FbTk::Menu *menu = createMenu(label, screen_number);
    if (menu != 0)
        parseMenu(parser, *menu, m_stringconvertor);

    endFile();

    return menu;
}


bool MenuCreator::createFromFile(const string &filename,
                                 FbTk::Menu &inject_into, bool require_begin) {

    string real_filename = FbTk::StringUtil::expandFilename(filename);
    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return false;

    startFile();
    string label;
    if (require_begin && !getStart(parser, label, m_stringconvertor)) {
        endFile();
        return false;
    }

    // save menu filename, so we can check if it changes
    Fluxbox::instance()->saveMenuFilename(real_filename.c_str());

    parseMenu(parser, inject_into, m_stringconvertor);
    endFile();

    return true;
}


bool MenuCreator::createWindowMenuFromFile(const string &filename,
                                           FbTk::Menu &inject_into,
                                           bool require_begin) {
    string real_filename = FbTk::StringUtil::expandFilename(filename);
    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return false;

    string label;

    startFile();
    if (require_begin && !getStart(parser, label, m_stringconvertor)) {
        endFile();
        return false;
    }

    parseWindowMenu(parser, inject_into, m_stringconvertor);
    endFile();

    return true;
}


FbTk::Menu *MenuCreator::createMenuType(const string &type, int screen_num) {
    BScreen *screen = Fluxbox::instance()->findScreen(screen_num);
    if (screen == 0)
        return 0;
    if (type == "iconmenu") {
        return new IconMenu(*screen);
    } else if (type == "workspacemenu") {
        return new WorkspaceMenu(*screen);
    } else if (type == "windowmenu") {
        FbTk::Menu *menu = screen->createMenu("");

        menu->disableTitle(); // not titlebar
        if (screen->windowMenuFilename().empty() ||
            ! createWindowMenuFromFile(screen->windowMenuFilename(), *menu, true)) {
            const char *default_menu[] = {
                "shade",
                "stick",
                "maximize",
                "iconify",
                "raise",
                "lower",
                "sendto",
                "layer",
                "alpha",
                "extramenus",
                "separator",
                "close",
                0
            };
            for (unsigned int i=0; default_menu[i]; ++i)
                createWindowMenuItem(default_menu[i], "", *menu);
        }
        menu->reconfigure(); // update graphics
        return menu;
    }

    return 0;
}

bool MenuCreator::createWindowMenuItem(const string &type,
                                       const string &label,
                                       FbTk::Menu &menu) {
    typedef FbTk::RefCount<FbTk::Command> RefCmd;
    _FB_USES_NLS;

    static MenuContext context;

    if (type == "shade") {
        static ObjectResource<FluxboxWindow, bool> res(&WindowCmd<void>::window, &FluxboxWindow::isShaded, &FluxboxWindow::shade, false);
        menu.insert(new BoolResMenuItem<ObjectResource<FluxboxWindow, bool> >(
                        label.empty()?_FB_XTEXT(Windowmenu, Shade, "Shade", "Shade the window"):label, 
                        res));

    } else if (type == "maximize") {
        RefCmd maximize_cmd(new WindowCmd<void>(&FluxboxWindow::maximizeFull));
        RefCmd maximize_vert_cmd(new WindowCmd<void>(&FluxboxWindow::maximizeVertical));
        RefCmd maximize_horiz_cmd(new WindowCmd<void>(&FluxboxWindow::maximizeHorizontal));
        FbTk::MultiButtonMenuItem *maximize_item =
            new FbTk::MultiButtonMenuItem(3,
                                          label.empty()?
                                          _FB_XTEXT(Windowmenu, Maximize,
                                                  "Maximize", "Maximize the window"):
                                          label);
        // create maximize item with:
        // button1: Maximize normal
        // button2: Maximize Vertical
        // button3: Maximize Horizontal
        maximize_item->setCommand(1, maximize_cmd);
        maximize_item->setCommand(2, maximize_vert_cmd);
        maximize_item->setCommand(3, maximize_horiz_cmd);
        menu.insert(maximize_item);
    } else if (type == "iconify") {
        static ObjectResource<FluxboxWindow, bool> res(&WindowCmd<void>::window, &FluxboxWindow::isIconic, &FluxboxWindow::toggleIconic, false);
        menu.insert(new BoolResMenuItem<ObjectResource<FluxboxWindow, bool> >(
                        label.empty() ?
                        _FB_XTEXT(Windowmenu, Iconify,
                                  "Iconify", "Iconify the window") :
                        label, res));
    } else if (type == "close") {
        RefCmd close_cmd(new WindowCmd<void>(&FluxboxWindow::close));
        menu.insert(label.empty() ?
                    _FB_XTEXT(Windowmenu, Close,
                              "Close", "Close the window") :
                    label, close_cmd);
    } else if (type == "kill" || type == "killwindow") {
        RefCmd kill_cmd(new WindowCmd<void>(&FluxboxWindow::kill));
        menu.insert(label.empty() ?
                    _FB_XTEXT(Windowmenu, Kill,
                              "Kill", "Kill the window"):
                    label, kill_cmd);
    } else if (type == "lower") {
        RefCmd lower_cmd(new WindowCmd<void>(&FluxboxWindow::lower));
        menu.insert( label.empty() ?
                     _FB_XTEXT(Windowmenu, Lower,
                               "Lower", "Lower the window"):
                     label, lower_cmd);
    } else if (type == "raise") {
        RefCmd raise_cmd(new WindowCmd<void>(&FluxboxWindow::raise));
        menu.insert(label.empty() ?
                    _FB_XTEXT(Windowmenu, Raise,
                              "Raise", "Raise the window"):
                    label, raise_cmd);

    } else if (type == "stick") {
        static ObjectResource<FluxboxWindow, bool> res(&WindowCmd<void>::window, &FluxboxWindow::isStuck, &FluxboxWindow::stick, false);
        menu.insert(new BoolResMenuItem<ObjectResource<FluxboxWindow, bool> >(
                        label.empty() ?
                        _FB_XTEXT(Windowmenu, Stick,
                                  "Stick", "Stick the window"):
                        label, res));
#ifdef HAVE_XRENDER
    } else if (type == "alpha") {
        if (FbTk::Transparent::haveComposite() || 
            FbTk::Transparent::haveRender()) {
            BScreen *screen = Fluxbox::instance()->findScreen(menu.screenNumber());
            if (screen == 0)
                return false;

            menu.insert(label.empty() ? _FB_XTEXT(Configmenu, Transparency, "Transparency",
                                                  "Menu containing various transparency options"): label,
                        new AlphaMenu(screen->menuTheme(),
                                      screen->imageControl(),
                                      *screen->layerManager().getLayer(Layer::MENU),
                                      context));
        }
#endif // HAVE_XRENDER
    } else if (type == "extramenus") {
        BScreen *screen = Fluxbox::instance()->findScreen(menu.screenNumber());
        BScreen::ExtraMenus::iterator it = screen->extraWindowMenus().begin();
        BScreen::ExtraMenus::iterator it_end = screen->extraWindowMenus().end();
        for (; it != it_end; ++it) {
            it->second->disableTitle();
            menu.insert(it->first, it->second);
        }

    } else if (type == "sendto") {
        menu.insert(label.empty() ? _FB_XTEXT(Windowmenu, SendTo, "Send To...", "Send to menu item name"):
                    label, new SendToMenu(*Fluxbox::instance()->findScreen(menu.screenNumber())));
    } else if (type == "layer") {
        BScreen *screen = Fluxbox::instance()->findScreen(menu.screenNumber());
        if (screen == 0)
            return false;

        FbTk::Menu *submenu = new LayerMenu(screen->menuTheme(),
                                            screen->imageControl(),
                                            *screen->layerManager().getLayer(Layer::MENU),
                                            &context,
                                            false);
        submenu->disableTitle();
        menu.insert(label.empty()?_FB_XTEXT(Windowmenu, Layer, "Layer ...", "Layer menu"):label, submenu);


    } else if (type == "separator") {
        menu.insert(new FbTk::MenuSeparator());
    } else
        return false;

    return true;
}

/* push our encoding-stacksize onto the stack */
void MenuCreator::startFile() {
    if (encoding_stack.empty())
        m_stringconvertor.setSource("");
    stacksize_stack.push_back(encoding_stack.size());
}

/**
 * Pop necessary encodings from the stack
 * (and endEncoding the final one) to our matching encoding-stacksize.
 */
void MenuCreator::endFile() {
    size_t target_size = stacksize_stack.back();
    size_t curr_size = encoding_stack.size();

    if (target_size != curr_size) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Menu, ErrorEndEncoding, "Warning: unbalanced [encoding] tags", "User menu file had unbalanced [encoding] tags")<<endl;
    }

    for (; curr_size > (target_size+1); --curr_size)
        encoding_stack.pop_back();

    if (curr_size == (target_size+1))
        endEncoding();

    stacksize_stack.pop_back();
}

/**
 * Push the encoding onto the stack, and make it active.
 */
void MenuCreator::startEncoding(const string &encoding) {
    // we push it regardless of whether it's valid, since we
    // need to stay balanced with the endEncodings.
    encoding_stack.push_back(encoding);

    // this won't change if it doesn't succeed
    m_stringconvertor.setSource(encoding);
}

/**
 * Pop the encoding from the stack, unless we are at our stacksize limit.
 * Restore the previous (valid) encoding.
 */
void MenuCreator::endEncoding() {
    size_t min_size = stacksize_stack.back();
    if (encoding_stack.size() <= min_size) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Menu, ErrorEndEncoding, "Warning: unbalanced [encoding] tags", "User menu file had unbalanced [encoding] tags")<<endl;
        return;
    }

    encoding_stack.pop_back();
    m_stringconvertor.reset();

    list<string>::reverse_iterator it = encoding_stack.rbegin();
    list<string>::reverse_iterator it_end = encoding_stack.rend();
    while (it != it_end && !m_stringconvertor.setSource(*it))
        ++it;

    if (it == it_end)
        m_stringconvertor.setSource("");
}

