// Remember.cc for Fluxbox Window Manager
// Copyright (c) 2002 Xavier Brouckaert
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Remember.cc,v 1.18 2003/05/26 11:27:31 rathnor Exp $

#include "Remember.hh"
#include "StringUtil.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "FbMenu.hh"
#include "MenuItem.hh"
#include "App.hh"

// TODO get rid of these
#define RC_PATH "fluxbox"
#define RC_INIT_FILE "init"

#include <X11/Xlib.h>

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE


#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <memory>

using namespace std;

#ifndef    MAXPATHLEN
#define    MAXPATHLEN 255
#endif // MAXPATHLEN

namespace {

class RememberMenuItem : public FbTk::MenuItem {
public:
    RememberMenuItem(const char *label, Remember &remember,
                     FluxboxWindow &fbwin,
                     Remember::Attribute attrib) :
        FbTk::MenuItem(label), m_remember(remember), 
        m_win(fbwin), m_attrib(attrib) {}

    bool isSelected() const {
        return m_remember.isRemembered(m_win.winClient(), m_attrib);
    }

    bool isEnabled() const {
        if (m_attrib != Remember::REM_JUMPWORKSPACE) 
            return true;
        else 
            return (m_remember.isRemembered(m_win.winClient(), Remember::REM_WORKSPACE));
    }

    void click(int button, int time) {
        if (isSelected()) {
            m_remember.forgetAttrib(m_win.winClient(), m_attrib);
        } else {
            m_remember.rememberAttrib(m_win.winClient(), m_attrib);
        }
        m_remember.save();
        FbTk::MenuItem::click(button, time);
    }

private:
    // my remember manager
    Remember &m_remember;
    FluxboxWindow &m_win;
    Remember::Attribute m_attrib;
};

FbTk::Menu *createRememberMenu(Remember &remember, FluxboxWindow &win) {
    // each fluxboxwindow has its own windowmenu
    // so we also create a remember menu just for it...
    BScreen &screen = win.screen();
    FbTk::Menu *menu = new FbMenu(*screen.menuTheme(), 
                                  screen.screenNumber(), 
                                  screen.imageControl(), 
                                  *screen.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()));
    menu->disableTitle();
    // TODO: nls
    menu->insert(new RememberMenuItem("Workspace", remember, win,
                                      Remember::REM_WORKSPACE));
    menu->insert(new RememberMenuItem("Jump to workspace", remember, win,
                                      Remember::REM_JUMPWORKSPACE));
    menu->insert(new RememberMenuItem("Dimensions", remember, win,
                                      Remember::REM_DIMENSIONS));
    menu->insert(new RememberMenuItem("Position", remember, win,
                                      Remember::REM_POSITION));
    menu->insert(new RememberMenuItem("Sticky", remember, win,
                                      Remember::REM_STUCKSTATE));
    menu->insert(new RememberMenuItem("Decorations", remember, win,
                                      Remember::REM_DECOSTATE));
    menu->insert(new RememberMenuItem("Shaded", remember, win,
                                      Remember::REM_SHADEDSTATE));
    menu->insert(new RememberMenuItem("Layer", remember, win,
                                      Remember::REM_LAYER));
    //    menu->insert(new RememberMenuItem("Tab", remember, win,
    //                                     Remember::REM_TABSTATE));
    menu->insert(new RememberMenuItem("Save on close", remember, win,
                                      Remember::REM_SAVEONCLOSE));

    menu->update();
    return menu;
};

const char * getWMClass(Window w) {
    XClassHint ch;
    
    if (XGetClassHint(FbTk::App::instance()->display(), w, &ch) == 0) {
        cerr<<"Failed to read class hint!"<<endl;
        return 0;
    } else {
        string m_instance_name;
        if (ch.res_name != 0) {
            m_instance_name = const_cast<char *>(ch.res_name);
            XFree(ch.res_name);
        } else 
            m_instance_name = "";
        
        if (ch.res_class != 0) {
            //m_class_name = const_cast<char *>(ch.res_class);
            XFree(ch.res_class);
        } else {
            //m_class_name = "";
        }
        return m_instance_name.c_str();
    }
}

};

Application::Application() {
    workspace_remember =
	dimensions_remember =
	position_remember =
	stuckstate_remember =
	decostate_remember =
	shadedstate_remember =
	tabstate_remember =
	jumpworkspace_remember =
        layer_remember =
	save_on_close_remember = false;
}

Remember::Remember() {
    load();
}

Application* Remember::add(const char* app_name) {
    if (!app_name)
        return 0;
    Application* a = new Application();
    apps[app_name] = a;
    return a;
}

Application* Remember::find(WinClient &winclient) {
    return find(getWMClass(winclient.window()));
}

Application* Remember::add(WinClient &winclient) {
    return add(getWMClass(winclient.window()));
}


Application* Remember::find(const char* app_name) {
    if (!app_name)
        return 0;
    Apps::iterator i = apps.find(app_name);
    if (i != apps.end())
        return i->second;
    else
        return 0;
}

int Remember::parseApp(ifstream &file, Application &app) {
    string line;
    int row = 0;
    while (! file.eof()) {
        if (getline(file, line)) {
            row++;
            if (line[0] != '#') { //the line is commented
                int parse_pos = 0, err = 0;
                string str_key, str_label;
                err = FbTk::StringUtil::getStringBetween(str_key, 
                                                         line.c_str(), 
                                                         '[', ']');
                if (err > 0 ) {
                    parse_pos += err;
                    err = FbTk::StringUtil::getStringBetween(str_label, 
                                                       line.c_str() + parse_pos, 
                                                       '{', '}');
                    if (err>0) {
                        parse_pos += err;
                    }
                } else
                    continue; //read next line

                if (!str_key.size())
                    continue; //read next line
                if (str_key == "Workspace") {
                    unsigned int w;
                    istringstream iss(str_label.c_str());
                    iss >> w;
                    app.rememberWorkspace(w);
                } else if (str_key == "Layer") {
                    unsigned int l;
                    istringstream iss(str_label.c_str());
                    iss >> l;
                    app.rememberLayer(l);
                } else if (str_key == "Dimensions") {
                    unsigned int h,w;
                    istringstream iss(str_label.c_str());
                    iss >> w >> h;
                    app.rememberDimensions(w,h);
                } else if (str_key == "Position") {
                    unsigned int x,y;
                    istringstream iss(str_label);
                    iss >> x >> y;
                    app.rememberPosition(x,y);
                } else if (str_key == "Shaded") {
                    app.rememberShadedstate((str_label=="yes"));
                } else if (str_key == "Tab") {
                    app.rememberTabstate((str_label=="yes"));
                } else if (str_key == "Deco") {
                    if (str_label == "NONE") {
                        app.rememberDecostate((unsigned int) 0);
                    } else if (str_label == "NORMAL") {
                        app.rememberDecostate((unsigned int) 0xfffffff);
                    } else if (str_label == "TINY") {
                        app.rememberDecostate((unsigned int)
                                             FluxboxWindow::DECORM_TITLEBAR 
                                             | FluxboxWindow::DECORM_ICONIFY
                                             | FluxboxWindow::DECORM_MENU
                                             );
                    } else if (str_label == "TOOL") {
                        app.rememberDecostate((unsigned int)
                                             FluxboxWindow::DECORM_TITLEBAR
                                             | FluxboxWindow::DECORM_MENU
                                             );
                    } else if (str_label == "BORDER") {
                        app.rememberDecostate((unsigned int)
                                             FluxboxWindow::DECORM_BORDER
                                             | FluxboxWindow::DECORM_MENU
                                             );
                    } else {
                        unsigned int mask;
                        const char * str = str_label.c_str();
                        // it'll have at least one char and \0, so this is safe
                        istringstream iss(str);
                        // check for hex
                        if (str[0] == '0' && str[1] == 'x') {
                            iss.seekg(2);
                            iss >> hex;
                        }
                        iss >> mask ;
                        app.rememberDecostate(mask);
                    }
                } else if (str_key == "Sticky") {
                    app.rememberStuckstate((str_label=="yes"));
                } else if (str_key == "Jump") {
                    app.rememberJumpworkspace((str_label=="yes"));
                } else if (str_key == "Close") {
                    app.rememberSaveOnClose((str_label=="yes"));
                } else if (str_key == "end") {
                    return row;
                } else {
                    cerr << "Unsupported apps key = " << str_key << endl;
                }
            }
        }
    }
    return row;
}

void Remember::load() {

    string apps_string = getenv("HOME")+string("/.")+RC_PATH+string("/")+"apps";
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): Loading apps file ["<<apps_string<<"]"<<endl;
#endif // DEBUG
    ifstream apps_file(apps_string.c_str());

    if (!apps_file.fail()) {
        if (!apps_file.eof()) {
            string line;
            int row = 0;
            while (getline(apps_file, line) && ! apps_file.eof()) {
                row++;
                if (line[0] == '#')
                    continue;
                string key;
                int pos=0;
                int err = FbTk::StringUtil::getStringBetween(key, 
                                                             line.c_str(), 
                                                             '[', ']');

                if (err > 0 && key == "app") {
                    pos += err;
                    string label;
                    err = FbTk::StringUtil::getStringBetween(label, 
                                                             line.c_str()+pos,
                                                             '(', ')');
                    if (err>0) {
                        Application *app = 0;
                        Apps::iterator i = apps.find(label);
                        if (i == apps.end()) {
                            app = new Application();
                            apps[label] = app;
                        } else
                            app = i->second;
                        row += parseApp(apps_file, *app);
                    } else
                        cerr<<"Error1 in apps file. Line("<<row<<")"<<endl;
                } else
                    cerr<<"Error2 in apps file. Line("<<row<<")"<<endl;

            }
        } else {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__FUNCTION__<< ") Empty apps file" << endl;
#endif
        }
    } else {
        cerr << "apps file failure" << endl;
    }
}

void Remember::save() {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): Saving apps file..."<<endl;
#endif // DEBUG
    string apps_string = getenv("HOME")+string("/.")+RC_PATH+string("/")+"apps";
    ofstream apps_file(apps_string.c_str());
    Apps::iterator it = apps.begin();
    Apps::iterator it_end = apps.end();
    for (; it != it_end; ++it) {
        apps_file << "[app] (" <<  it->first << ")" << endl;
        Application *a = it->second;
        if (a->workspace_remember) {
            apps_file << "  [Workspace]\t{" << a->workspace << "}" << endl;
        }
        if (a->dimensions_remember) {
            apps_file << "  [Dimensions]\t{" << a->w << " " << a->h << "}" << endl;
        }
        if (a->position_remember) {
            apps_file << "  [Position]\t{" << a->x << " " << a->y << "}" << endl;
        }
        if (a->shadedstate_remember) {
            apps_file << "  [Shaded]\t{" << ((a->shadedstate)?"yes":"no") << "}" << endl;
        }
        if (a->tabstate_remember) {
            apps_file << "  [Tab]\t\t{" << ((a->tabstate)?"yes":"no") << "}" << endl;
        }
        if (a->decostate_remember) {
            switch (a->decostate) {
            case (0) :
                apps_file << "  [Deco]\t{NONE}" << endl; 
                break;
            case (0xffffffff):
            case (FluxboxWindow::DECORM_LAST - 1):
                apps_file << "  [Deco]\t{NORMAL}" << endl;
                break;
            case (FluxboxWindow::DECORM_TITLEBAR 
                  | FluxboxWindow::DECORM_ICONIFY
                  | FluxboxWindow::DECORM_MENU):
                apps_file << "  [Deco]\t{TOOL}" << endl;
                break;
            case (FluxboxWindow::DECORM_TITLEBAR 
                  | FluxboxWindow::DECORM_MENU):
                apps_file << "  [Deco]\t{TINY}" << endl;
                break;
            case (FluxboxWindow::DECORM_BORDER 
                  | FluxboxWindow::DECORM_MENU):
                apps_file << "  [Deco]\t{BORDER}" << endl;
                break;
            default:
                apps_file << "  [Deco]\t{0x"<<hex<<a->decostate<<dec<<"}"<<endl;
                break;
            }
        }
        if (a->stuckstate_remember) {
            apps_file << "  [Sticky]\t{" << ((a->stuckstate)?"yes":"no") << "}" << endl;
        }
        if (a->jumpworkspace_remember) {
            apps_file << "  [Jump]\t{" << ((a->jumpworkspace)?"yes":"no") << "}" << endl;
        }
        if (a->layer_remember) {
            apps_file << "  [Layer]\t{" << a->layer << "}" << endl;
        }
        if (a->save_on_close_remember) {
            apps_file << "  [Close]\t{" << ((a->save_on_close)?"yes":"no") << "}" << endl;
        }
        apps_file << "[end]" << endl;
    }
}

bool Remember::isRemembered(WinClient &winclient, Attribute attrib) {
    Application *app = find(winclient);
    if (!app) return false;
    switch (attrib) {
    case REM_WORKSPACE:
        return app->workspace_remember;
        break;
    case REM_DIMENSIONS:
        return app->dimensions_remember;
        break;
    case REM_POSITION:
        return app->position_remember;
        break;
    case REM_STUCKSTATE:
        return app->stuckstate_remember;
        break;
    case REM_DECOSTATE:
        return app->decostate_remember;
        break;
    case REM_SHADEDSTATE:
        return app->shadedstate_remember;
        break;
        //    case REM_TABSTATE:
        //        return app->tabstate_remember;
        //        break;
    case REM_JUMPWORKSPACE:
        return app->jumpworkspace_remember;
        break;
    case REM_LAYER:
        return app->layer_remember;
        break;
    case REM_SAVEONCLOSE:
        return app->save_on_close_remember;
        break;
    case REM_LASTATTRIB:
    default:
        return false; // should never get here
    }
}

void Remember::rememberAttrib(WinClient &winclient, Attribute attrib) {
    FluxboxWindow *win = winclient.fbwindow();
    if (!win) return;
    Application *app = find(winclient);
    if (!app) {
        app = add(winclient);
        if (!app) return;
    }
    switch (attrib) {
    case REM_WORKSPACE:
        app->rememberWorkspace(win->workspaceNumber());
        break;
    case REM_DIMENSIONS:
        app->rememberDimensions(win->width(), win->height());
        break;
    case REM_POSITION:
        app->rememberPosition(win->x(), win->y());
        break;
    case REM_STUCKSTATE:
        app->rememberShadedstate(win->isShaded());
        break;
    case REM_DECOSTATE:
        app->rememberDecostate(win->decorationMask());
        break;
    case REM_SHADEDSTATE:
        app->rememberStuckstate(win->isStuck());
        break;
        //    case REM_TABSTATE:
        //        break;
    case REM_JUMPWORKSPACE:
        app->rememberJumpworkspace(true);
        break;
    case REM_LAYER:
        app->rememberLayer(win->layerNum());
        break;
    case REM_SAVEONCLOSE:
        app->rememberSaveOnClose(true);
        break;
    case REM_LASTATTRIB:
    default:
        // nothing
        break;
    }
}

void Remember::forgetAttrib(WinClient &winclient, Attribute attrib) {
    FluxboxWindow *win = winclient.fbwindow();
    if (!win) return;
    Application *app = find(winclient);
    if (!app) {
        app = add(winclient);
        if (!app) return;
    }
    switch (attrib) {
    case REM_WORKSPACE:
        app->forgetWorkspace();
        break;
    case REM_DIMENSIONS:
        app->forgetDimensions();
        break;
    case REM_POSITION:
        app->forgetPosition();
        break;
    case REM_STUCKSTATE:
        app->forgetStuckstate();
        break;
    case REM_DECOSTATE:
        app->forgetDecostate();
        break;
    case REM_SHADEDSTATE:
        app->forgetShadedstate();
        break;
//    case REM_TABSTATE:
//        break;
    case REM_JUMPWORKSPACE:
        app->forgetJumpworkspace();
        break;
    case REM_LAYER:
        app->forgetLayer();
        break;
    case REM_SAVEONCLOSE:
        app->forgetSaveOnClose();
        break;
    case REM_LASTATTRIB:
    default:
        // nothing
        break;
    }
}

void Remember::setupWindow(FluxboxWindow &win) {
    WinClient &winclient = win.winClient();

    // we don't touch the window if it is a transient
    // of something else
    int menupos = win.menu().numberOfItems()-2;
    if (menupos < -1)
        menupos = -1;

    if (winclient.transientFor()) {
        // still put something in the menu so people don't get confused
        // so, we add a disabled item...
        FbTk::MenuItem *item = new FbTk::MenuItem("Remember...");
        item->setEnabled(false);
        win.menu().insert(item, menupos);
        win.menu().update();
        return;
    }

    // add the menu, this -2 is somewhat dodgy... :-/
    // All windows get the remember menu.
    // TODO: nls
    win.menu().insert("Remember...", 
                               createRememberMenu(*this, win), 
                               menupos);
    win.menu().reconfigure();

    Application *app = find(winclient);
    if (app == 0) 
        return; // nothing to do

    BScreen &screen = win.screen();

    if (app->workspace_remember) {
        // TODO: fix placement to initialise properly
        screen.reassociateWindow(&win, app->workspace, true); 
        if (app->jumpworkspace_remember)
            screen.changeWorkspaceID(app->workspace);
    }

    if (app->dimensions_remember)
        win.resize(app->w, app->h);
    
    if (app->position_remember)
        win.move(app->x, app->y);

    if (app->shadedstate_remember)
        // if inconsistent...
        if (win.isShaded() && !app->shadedstate ||
            !win.isShaded() && app->shadedstate)
            win.shade(); // toggles

    // external tabs aren't available atm...
    //if (app->tabstate_remember) ...

    if (app->decostate_remember)
        win.setDecorationMask(app->decostate);

    if (app->stuckstate_remember)
        // if inconsistent...
        if (win.isStuck() && !app->stuckstate ||
            !win.isStuck() && app->stuckstate)
            win.stick(); // toggles

    if (app->layer_remember)
        win.moveToLayer(app->layer);

}

void Remember::updateWindowClose(FluxboxWindow &win) {
    // This doesn't work at present since fluxbox.cc is missing the windowclose stuff.
    // I don't trust it (particularly winClient()) while this is the case
    return;

    WinClient &winclient = win.winClient();
    Application *app = find(winclient);

    if (!app || !(app->save_on_close_remember && app->save_on_close))
        return;

    for (int attrib = 0; attrib <= REM_LASTATTRIB; attrib++) {
        if (isRemembered(winclient, (Attribute) attrib)) {
            rememberAttrib(winclient, (Attribute) attrib);
        }
    }
/*
    if (app->workspace_remember) 
        app->rememberWorkspace(win.workspaceNumber());
    if (app->dimensions_remember)
        app->rememberDimensions(win.width(), win.height());
    if (app->position_remember)
        app->rememberPosition(win.x(), win.y());
    if (app->shadedstate_remember)
        app->rememberShadedstate(win.isShaded());
    // external tabs off atm
    //if (app->tabstate_remember) ...
    if (app->decostate_remember)
        app->rememberDecostate(win.decorationMask());
    if (app->stuckstate_remember)
        app->rememberStuckstate(win.isStuck());
    if (app->jumpworkspace_remember)
        app->rememberJumpworkspace(true);
*/
    save();
}
