// MenuCreator.cc for Fluxbox
// Copyright (c) 2004-2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "MenuCreator.hh"

#include "defaults.hh"
#include "Screen.hh"
#include "FbTk/CommandParser.hh"
#include "fluxbox.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "CurrentWindowCmd.hh"
#include "WindowMenuAccessor.hh"

#include "ClientMenu.hh"
#include "WorkspaceMenu.hh"
#include "LayerMenu.hh"
#include "SendToMenu.hh"
#include "AlphaMenu.hh"
#include "Layer.hh"

#include "FbMenuParser.hh"
#include "StyleMenuItem.hh"
#include "RootCmdMenuItem.hh"

#include "FbTk/I18n.hh"
#include "FbTk/MultiButtonMenuItem.hh"
#include "FbTk/BoolMenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/Transparent.hh"

#include <iostream>
#include <algorithm>

#ifdef REMEMBER
#include "Remember.hh"
#endif // REMEMBER

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::list;
using std::less;
using FbTk::AutoReloadHelper;

namespace {

FbTk::StringConvertor s_stringconvertor(FbTk::StringConvertor::ToFbString);

list<string> s_encoding_stack;
list<size_t> s_stacksize_stack;



enum {
    L_SHADE = 0,
    L_MAXIMIZE,
    L_ICONIFY,
    L_CLOSE,
    L_KILL,
    L_LOWER,
    L_RAISE,
    L_STICK,
    L_TITLE,
    L_SENDTO,
    L_LAYER,

    L_ALPHA,

    L_REMEMBER,

    L_MENU_EXIT,
    L_MENU_ICONS,
};

// returns 'label' if not empty, otherwise a (translated) default
// value based upon 'type'
const FbTk::FbString& _l(const FbTk::FbString& label, size_t type) {

    _FB_USES_NLS;
    static const FbTk::FbString _default_labels[] = {
        _FB_XTEXT(Windowmenu, Shade, "Shade", "Shade the window"),
        _FB_XTEXT(Windowmenu, Maximize, "Maximize", "Maximize the window"),
        _FB_XTEXT(Windowmenu, Iconify, "Iconify", "Iconify the window"),
        _FB_XTEXT(Windowmenu, Close, "Close", "Close the window"),
        _FB_XTEXT(Windowmenu, Kill, "Kill", "Kill the window"),
        _FB_XTEXT(Windowmenu, Lower, "Lower", "Lower the window"),
        _FB_XTEXT(Windowmenu, Raise, "Raise", "Raise the window"),
        _FB_XTEXT(Windowmenu, Stick, "Stick", "Stick the window"),
        _FB_XTEXT(Windowmenu, SetTitle, "Set Title", "Change the title of the window"),
        _FB_XTEXT(Windowmenu, SendTo, "Send To...", "Send to menu item name"),
        _FB_XTEXT(Windowmenu, Layer, "Layer ...", "Layer menu"),

        _FB_XTEXT(Configmenu, Transparency, "Transparency", "Menu containing various transparency options"),

        _FB_XTEXT(Remember, MenuItemName, "Remember...", "Remember item in menu"),

        _FB_XTEXT(Menu, Exit, "Exit", "Exit Command"),
        _FB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"),
    };

    if (label.empty()) {
        return _default_labels[type];
    }

    return label;
}



/**
 * Push the encoding onto the stack, and make it active.
 */
void startEncoding(const string &encoding) {
    // we push it regardless of whether it's valid, since we
    // need to stay balanced with the endEncodings.
    s_encoding_stack.push_back(encoding);

    // this won't change if it doesn't succeed
    s_stringconvertor.setSource(encoding);
}

/**
 * Pop the encoding from the stack, unless we are at our stacksize limit.
 * Restore the previous (valid) encoding.
 */
void endEncoding() {
    size_t min_size = s_stacksize_stack.back();
    if (s_encoding_stack.size() <= min_size) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Menu, ErrorEndEncoding, "Warning: unbalanced [encoding] tags", "User menu file had unbalanced [encoding] tags")<<endl;
        return;
    }

    s_encoding_stack.pop_back();
    s_stringconvertor.reset();

    list<string>::reverse_iterator it = s_encoding_stack.rbegin();
    list<string>::reverse_iterator it_end = s_encoding_stack.rend();
    while (it != it_end && !s_stringconvertor.setSource(*it))
        ++it;

    if (it == it_end)
        s_stringconvertor.setSource("");
}


/* push our encoding-stacksize onto the stack */
void startFile() {
    if (s_encoding_stack.empty())
        s_stringconvertor.setSource("");
    s_stacksize_stack.push_back(s_encoding_stack.size());
}

/**
 * Pop necessary encodings from the stack
 * (and endEncoding the final one) to our matching encoding-stacksize.
 */
void endFile() {
    size_t target_size = s_stacksize_stack.back();
    size_t curr_size = s_encoding_stack.size();

    if (target_size != curr_size) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Menu, ErrorEndEncoding, "Warning: unbalanced [encoding] tags", "User menu file had unbalanced [encoding] tags")<<endl;
    }

    for (; curr_size > (target_size+1); --curr_size)
        s_encoding_stack.pop_back();

    if (curr_size == (target_size+1))
        endEncoding();

    s_stacksize_stack.pop_back();
}


void createStyleMenu(FbTk::Menu &parent, const string &label,
                     AutoReloadHelper *reloader, const string &directory) {
    // perform shell style ~ home directory expansion
    string stylesdir(FbTk::StringUtil::expandFilename(directory));

    if (!FbTk::FileUtil::isDirectory(stylesdir.c_str()))
        return;

    if (reloader)
        reloader->addFile(stylesdir);

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
            parent.insertItem(new StyleMenuItem(filelist[file_index], style));
    }
    // update menu graphics
    parent.updateMenu();

}

void createRootCmdMenu(FbTk::Menu &parent, const string &label,
                       const string &directory, AutoReloadHelper *reloader,
                       const string &cmd) {

    // perform shell style ~ home directory expansion
    string rootcmddir(FbTk::StringUtil::expandFilename(directory));

    if (!FbTk::FileUtil::isDirectory(rootcmddir.c_str()))
        return;

    if (reloader)
        reloader->addFile(rootcmddir);

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
            parent.insertItem(new RootCmdMenuItem(filelist[file_index], rootcmd, cmd));
    }
    // update menu graphics
    parent.updateMenu();

}


class ParseItem {
public:
    explicit ParseItem(FbTk::Menu *menu):m_menu(menu) {}

    void load(FbTk::Parser &p, FbTk::StringConvertor &m_labelconvertor) {
        p>>m_key>>m_label>>m_cmd>>m_icon;
        m_label.second = m_labelconvertor.recode(m_label.second);
    }
    const string &icon() const { return m_icon.second; }
    const string &command() const { return m_cmd.second; }
    const string &label() const { return m_label.second; }
    const string &key() const { return m_key.second; }
    FbTk::Menu *menu() { return m_menu; }
private:
    FbTk::Parser::Item m_key, m_label, m_cmd, m_icon;
    FbTk::Menu *m_menu;
};

class MenuContext: public LayerObject {
public:
    void moveToLayer(int layer_number) {
        if (FbMenu::window() == 0)
            return;
        FbMenu::window()->moveToLayer(layer_number);
    }
    int layerNumber() const {
        if (FbMenu::window() == 0)
            return -1;
        return FbMenu::window()->layerItem().getLayerNum();
    }

};

void translateMenuItem(FbTk::Parser &parse, ParseItem &item,
                       FbTk::StringConvertor &labelconvertor,
                       AutoReloadHelper *reloader);


void parseMenu(FbTk::Parser &pars, FbTk::Menu &menu,
               FbTk::StringConvertor &label_convertor,
               AutoReloadHelper *reloader) {
    ParseItem pitem(&menu);
    while (!pars.eof()) {
        pitem.load(pars, label_convertor);
        if (pitem.key() == "end")
            return;
        translateMenuItem(pars, pitem, label_convertor, reloader);
    }
}

void translateMenuItem(FbTk::Parser &parse, ParseItem &pitem,
                       FbTk::StringConvertor &labelconvertor,
                       AutoReloadHelper *reloader) {
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
        menu.insertSubmenu(_l(str_label, L_MENU_ICONS), submenu);
    } else if (str_key == "exit") { // exit
        FbTk::RefCount<FbTk::Command<void> > exit_cmd(FbTk::CommandParser<void>::instance().parse("exit"));
        menu.insertCommand(_l(str_label, L_MENU_EXIT), exit_cmd);
    } else if (str_key == "exec") {
        FbTk::RefCount<FbTk::Command<void> > exec_cmd(FbTk::CommandParser<void>::instance().parse("exec " + str_cmd));
        menu.insertCommand(str_label, exec_cmd);
    } else if (str_key == "macrocmd") {
        FbTk::RefCount<FbTk::Command<void> > macro_cmd(FbTk::CommandParser<void>::instance().parse("macrocmd " + str_cmd));
        menu.insertCommand(str_label, macro_cmd);
    } else if (str_key == "style") {
        menu.insertItem(new StyleMenuItem(str_label, str_cmd));
    } else if (str_key == "config") {
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0)
            menu.insertSubmenu(str_label, &screen->configMenu());
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
                    MenuCreator::createFromFile(thisfile, menu, reloader, false);
                }
            }

        } else {
            // inject this file into the current menu
            MenuCreator::createFromFile(newfile, menu, reloader, false);
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

        parseMenu(parse, *submenu, labelconvertor, reloader);
        submenu->updateMenu();
        menu.insertSubmenu(str_label, submenu);

    } // end of submenu
    else if (str_key == "stylesdir" || str_key == "stylesmenu") {
        createStyleMenu(menu, str_label, reloader,
                        str_key == "stylesmenu" ? str_cmd : str_label);
    } // end of stylesdir
    else if (str_key == "themesdir" || str_key == "themesmenu") {
        createStyleMenu(menu, str_label, reloader,
                        str_key == "themesmenu" ? str_cmd : str_label);
    } // end of themesdir
    else if (str_key == "wallpapers" || str_key == "wallpapermenu" ||
             str_key == "rootcommands") {
         createRootCmdMenu(menu, str_label, str_label, reloader,
                          str_cmd == "" ? realProgramName("fbsetbg") : str_cmd);
    } // end of wallpapers
    else if (str_key == "workspaces") {
        BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
        if (screen != 0) {
            screen->workspaceMenu().setInternalMenu();
            menu.insertSubmenu(str_label, &screen->workspaceMenu());
        }
    } else if (str_key == "separator") {
        menu.insertItem(new FbTk::MenuSeparator());
    } else if (str_key == "encoding") {
        startEncoding(str_cmd);
    } else if (str_key == "endencoding") {
        endEncoding();
    } else if (!MenuCreator::createWindowMenuItem(str_key, str_label, menu)) {
        // if we didn't find any special menu item we try with command parser
        // we need to attach command to arguments so command parser can parse it
        string line = str_key + " " + str_cmd;
        FbTk::RefCount<FbTk::Command<void> > command(FbTk::CommandParser<void>::instance().parse(line));
        if (command != 0) {
            // special NLS default labels
            if (str_label.empty()) {
                if (str_key == "reconfig" || str_key == "reconfigure") {
                    menu.insertCommand(_FB_XTEXT(Menu, Reconfigure, "Reload Config", "Reload all the configs"), command);
                    return;
                } else if (str_key == "restart") {
                    menu.insertCommand(_FB_XTEXT(Menu, Restart, "Restart", "Restart Command"), command);
                    return;
                }
            }
            menu.insertCommand(str_label, command);
        }
    }
    if (menu.numberOfItems() != 0) {
        FbTk::MenuItem *item = menu.find(menu.numberOfItems() - 1);
        if (item != 0 && !pitem.icon().empty())
            item->setIcon(pitem.icon(), menu.screenNumber());
    }
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

} // end of anonymous namespace



FbMenu* MenuCreator::createMenu(const std::string& label, BScreen& screen) {
    FbTk::Layer* layer = screen.layerManager().getLayer(ResourceLayer::MENU);
    FbMenu *menu = new FbMenu(screen.menuTheme(), screen.imageControl(), *layer);
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}

FbMenu *MenuCreator::createMenu(const string &label, int screen_number) {
    BScreen *screen = Fluxbox::instance()->findScreen(screen_number);
    if (screen == 0)
        return 0;
    return MenuCreator::createMenu(label, *screen);
}

bool MenuCreator::createFromFile(const string &filename,
                                 FbTk::Menu &inject_into,
                                 AutoReloadHelper *reloader, bool begin) {
    string real_filename = FbTk::StringUtil::expandFilename(filename);

    FbMenuParser parser(real_filename);
    if (!parser.isLoaded())
        return false;

    startFile();
    if (begin) {
        string label;
        if (!getStart(parser, label, s_stringconvertor)) {
            endFile();
            return false;
        }
        inject_into.setLabel(label);
    }

    // save menu filename, so we can check if it changes
    if (reloader)
        reloader->addFile(real_filename);

    parseMenu(parser, inject_into, s_stringconvertor, reloader);
    endFile();

    return true;
}

FbMenu *MenuCreator::createMenuType(const string &type, int screen_num) {
    BScreen *screen = Fluxbox::instance()->findScreen(screen_num);
    if (screen == 0)
        return 0;
    if (type == "iconmenu")
        return new ClientMenu(*screen, screen->iconList(),
                              true); // listen to icon list changes
    else if (type == "workspacemenu")
        return new WorkspaceMenu(*screen);

    return 0;
}

bool MenuCreator::createWindowMenuItem(const string &type,
                                       const string &label,
                                       FbTk::Menu &menu) {

    typedef FbTk::RefCount<FbTk::Command<void> > RefCmd;

    static MenuContext context;
    int screen = menu.screenNumber();

    if (type == "shade") {
        static WindowMenuAccessor<bool> res(&FluxboxWindow::isShaded, &FluxboxWindow::setShaded, false);
        menu.insertItem(new FbTk::BoolMenuItem(_l(label, L_SHADE), res));
    } else if (type == "maximize") {
        RefCmd maximize_cmd(new WindowCmd<void>(&FluxboxWindow::maximizeFull));
        RefCmd maximize_vert_cmd(new WindowCmd<void>(&FluxboxWindow::maximizeVertical));
        RefCmd maximize_horiz_cmd(new WindowCmd<void>(&FluxboxWindow::maximizeHorizontal));
        FbTk::MultiButtonMenuItem *maximize_item =
            new FbTk::MultiButtonMenuItem(3, _l(label, L_MAXIMIZE));
        maximize_item->setCommand(1, maximize_cmd);
        maximize_item->setCommand(2, maximize_vert_cmd);
        maximize_item->setCommand(3, maximize_horiz_cmd);
        menu.insertItem(maximize_item);
    } else if (type == "iconify") {
        static WindowMenuAccessor<bool> res(&FluxboxWindow::isIconic, &FluxboxWindow::setIconic, false);
        menu.insertItem(new FbTk::BoolMenuItem(_l(label, L_ICONIFY), res));
    } else if (type == "close") {
        RefCmd close_cmd(new WindowCmd<void>(&FluxboxWindow::close));
        menu.insertCommand(_l(label, L_CLOSE), close_cmd);
    } else if (type == "kill" || type == "killwindow") {
        RefCmd kill_cmd(new WindowCmd<void>(&FluxboxWindow::kill));
        menu.insertCommand(_l(label, L_KILL), kill_cmd);
    } else if (type == "lower") {
        RefCmd lower_cmd(new WindowCmd<void>(&FluxboxWindow::lower));
        menu.insertCommand(_l(label, L_LOWER), lower_cmd);
    } else if (type == "raise") {
        RefCmd raise_cmd(new WindowCmd<void>(&FluxboxWindow::raise));
        menu.insertCommand(_l(label, L_RAISE), raise_cmd);
    } else if (type == "stick") {
        static WindowMenuAccessor<bool> res(&FluxboxWindow::isStuck, &FluxboxWindow::setStuck, false);
        menu.insertItem(new FbTk::BoolMenuItem(_l(label, L_STICK), res));
    } else if (type == "settitledialog") {
        RefCmd setname_cmd(new SetTitleDialogCmd());
        menu.insertCommand(_l(label, L_TITLE), setname_cmd);
#ifdef HAVE_XRENDER
    } else if (type == "alpha") {
        if (FbTk::Transparent::haveComposite() || 
            FbTk::Transparent::haveRender()) {
            BScreen* s = Fluxbox::instance()->findScreen(screen);
            if (s == 0)
                return false;

            FbTk::Menu *submenu =
                new AlphaMenu(s->menuTheme(),
                              s->imageControl(),
                              *(s->layerManager()).getLayer(ResourceLayer::MENU));
            submenu->disableTitle();
            menu.insertSubmenu(_l(label, L_ALPHA), submenu);
        }
#endif // HAVE_XRENDER
    } else if (type == "extramenus") {
#ifdef REMEMBER
        BScreen* s = Fluxbox::instance()->findScreen(screen);
        if (s == 0) {
            return false;
        }
        menu.insertSubmenu(_l("", L_REMEMBER), Remember::createMenu(*s));
#endif
    } else if (type == "sendto") {
        menu.insertSubmenu(_l(label, L_SENDTO), 
            new SendToMenu(*Fluxbox::instance()->findScreen(screen)));
    } else if (type == "layer") {
        BScreen* s = Fluxbox::instance()->findScreen(screen);
        if (s == 0)
            return false;

        FbTk::Menu *submenu = new LayerMenu(s->menuTheme(),
                                            s->imageControl(),
                                            *(s->layerManager()).getLayer(ResourceLayer::MENU),
                                            &context,
                                            false);
        submenu->disableTitle();
        menu.insertSubmenu(_l(label, L_LAYER), submenu);

    } else if (type == "separator") {
        menu.insertItem(new FbTk::MenuSeparator());
    } else
        return false;

    return true;
}

