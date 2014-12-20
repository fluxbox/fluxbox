// menutest.cc a test app for Menus
// Copyright (c) 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "App.hh"
#include "FbWindow.hh"
#include "Font.hh"
#include "EventHandler.hh"
#include "EventManager.hh"
#include "GContext.hh"
#include "Color.hh"
#include "Menu.hh"
#include "MenuItem.hh"
#include "MenuSeparator.hh"
#include "StringUtil.hh"
#include "ImageControl.hh"

#include "../FbMenuParser.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <string>
#include <iostream>
using namespace std;

void doSubmenu(Parser &parser, FbTk::Menu &menu, 
               FbTk::MenuTheme &theme,
               FbTk::ImageControl &image_ctrl,
               const std::string &labelstr) {

    Parser::Item key, label, cmd, icon;

    FbTk::Menu *submenu = new FbTk::Menu(theme, image_ctrl);
    submenu->setLabel(labelstr.c_str());
    menu.insert(labelstr.c_str(), submenu);
    // skip submenu items
    if (key.second == "begin") {
        while (key.second != "end") {
            parser>>key>>label>>cmd>>icon;
            if (key.second == "begin")
                doSubmenu(parser, *submenu,
                          theme, image_ctrl,
                          label.second);
        }
    }
}

class App:public FbTk::App, public FbTk::EventHandler {
public:
    App(const char *displayname, const std::string &stylefile, const std::string &menufile):
        FbTk::App(displayname),
        m_image_ctrl(DefaultScreen(display())),
        m_menu_theme(DefaultScreen(display())),
        m_menu(m_menu_theme, m_image_ctrl) { 

        //m_menu_theme.frameFont().setAntialias(true);
        //m_menu_theme.titleFont().setAntialias(true);

        cerr<<"Loading menu: "<<menufile<<endl;
        FbMenuParser parser(menufile);
        if (parser.isLoaded()) { 
            // get start of file
            Parser::Item key, label, cmd, icon;
            while (!parser.eof()) {
                // get first begin line
                parser>>key>>label>>cmd>>icon;
                if (key.second == "begin")
                    break;
                
            }

            m_menu.setLabel(label.second.c_str());
            
            while (!parser.eof()) {
                parser>>key>>label>>cmd>>icon;
                if (key.second == "end")
                    break;

                string iconfile = icon.second;
                if (key.second == "separator")
                    m_menu.insert(new FbTk::MenuSeparator());
                else if (key.second == "begin") { // new submenu
                    doSubmenu(parser, m_menu,
                              m_menu_theme,
                              m_image_ctrl,
                              label.second);
                } else if (key.second != "styles" && 
                           key.second != "stylesdir")
                    m_menu.insert(label.second.c_str());
                    

                // set icon on items
                if (!iconfile.empty()) {
                    FbTk::MenuItem *item = m_menu.find(m_menu.numberOfItems() - 1);
                    item->setIcon(iconfile, m_menu.screenNumber());
                }
            }

        }

        cerr<<"Loading style: "<<stylefile<<endl;        
        FbTk::ThemeManager::instance().load(stylefile);

        m_menu.show();

    }

    ~App() {
    }


private:
    FbTk::ImageControl m_image_ctrl;
    FbTk::MenuTheme m_menu_theme;
    FbTk::Menu m_menu;
};

int main(int argc, char **argv) {
    string displayname("");
    string stylefile, menufile="~/.fluxbox/menu";
    for (int a=1; a<argc; ++a) {
        if (strcmp("-display", argv[a]) == 0 && a + 1 < argc) {
            displayname = argv[++a];
        } else if (strcmp("-style", argv[a]) == 0 && a + 1 < argc) {
            stylefile = argv[++a];
        } else if (strcmp("-menu", argv[a]) == 0 && a + 1 < argc) {
            menufile = argv[++a];
        }
    }
    menufile = FbTk::StringUtil::expandFilename(menufile);
    App app(displayname.c_str(), stylefile, menufile);

    app.eventLoop();
	
}
