// Keys.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

//$Id$


#include "Keys.hh"

#include "fluxbox.hh"
#include "Screen.hh"

#include "FbTk/EventManager.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/App.hh"
#include "FbTk/Command.hh"

#include "CommandParser.hh"
#include "FbTk/I18n.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


#ifdef HAVE_CCTYPE
  #include <cctype>
#else
  #include <ctype.h>
#endif	// HAVE_CCTYPE

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CERRNO
  #include <cerrno>
#else
  #include <errno.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif	// HAVE_SYS_TYPES_H

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif	// HAVE_SYS_WAIT_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif	// HAVE_UNISTD_H

#ifdef	HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif	// HAVE_SYS_STAT_H

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::ifstream;
using std::pair;

Keys::Keys(): m_display(FbTk::App::instance()->display()) { }

Keys::~Keys() {
    ungrabKeys();
    ungrabButtons();
    deleteTree();
}

/// Destroys the keytree
void Keys::deleteTree() {
    for (keyspace_t::iterator map_it = m_map.begin(); map_it != m_map.end(); ++map_it)
        delete map_it->second;
    m_map.clear();
}

// keys are only grabbed in global context
void Keys::grabKey(unsigned int key, unsigned int mod) {
    WindowMap::iterator it = m_window_map.begin();
    WindowMap::iterator it_end = m_window_map.end();

    for (; it != it_end; ++it) {
        if ((it->second & Keys::GLOBAL) > 0)
            FbTk::KeyUtil::grabKey(key, mod, it->first);
    }
}

// keys are only grabbed in global context
void Keys::ungrabKeys() {
    WindowMap::iterator it = m_window_map.begin();
    WindowMap::iterator it_end = m_window_map.end();

    for (; it != it_end; ++it) {
        if ((it->second & Keys::GLOBAL) > 0)
            FbTk::KeyUtil::ungrabKeys(it->first);
    }
}

// ON_DESKTOP context doesn't need to be grabbed
void Keys::grabButton(unsigned int button, unsigned int mod, int context) {
    WindowMap::iterator it = m_window_map.begin();
    WindowMap::iterator it_end = m_window_map.end();

    for (; it != it_end; ++it) {
        if ((context & it->second & ~Keys::ON_DESKTOP) > 0)
            FbTk::KeyUtil::grabButton(button, mod, it->first,
                                      ButtonPressMask|ButtonReleaseMask);
    }
}

void Keys::ungrabButtons() {
    WindowMap::iterator it = m_window_map.begin();
    WindowMap::iterator it_end = m_window_map.end();

    for (; it != it_end; ++it)
        FbTk::KeyUtil::ungrabButtons(it->first);
}

void Keys::grabWindow(Window win) {
    if (!m_keylist)
        return;

    // make sure the window is in our list
    WindowMap::iterator win_it = m_window_map.find(win);
    if (win_it == m_window_map.end())
        return;

    m_handler_map[win]->grabButtons();
    keylist_t::iterator it = m_keylist->keylist.begin();
    keylist_t::iterator it_end = m_keylist->keylist.end();
    for (; it != it_end; ++it) {
        // keys are only grabbed in global context
        if ((win_it->second & Keys::GLOBAL) > 0 && (*it)->type == KeyPress)
            FbTk::KeyUtil::grabKey((*it)->key, (*it)->mod, win);
        // ON_DESKTOP buttons don't need to be grabbed
        else if ((win_it->second & (*it)->context & ~Keys::ON_DESKTOP) > 0 &&
                 (*it)->type == ButtonPress)
            FbTk::KeyUtil::grabButton((*it)->key, (*it)->mod, win,
                                      ButtonPressMask|ButtonReleaseMask);
    }
}

/**
    Load and grab keys
    TODO: error checking
    @return true on success else false
*/
bool Keys::load(const char *filename) {
    // an intentionally empty file will still have one root mapping
    bool firstload = m_map.empty();

    if (!filename) {
        if (firstload)
            loadDefaults();
        return false;
    }

    FbTk::App::instance()->sync(false);

    // open the file
    ifstream infile(filename);
    if (!infile) {
        if (firstload)
            loadDefaults();

        return false; // failed to open file
    }

    // free memory of previous grabs
    deleteTree();

    m_map["default:"] = new t_key(0,0,0,0);

    unsigned int current_line = 0; //so we can tell the user where the fault is

    while (!infile.eof()) {
        string linebuffer;

        getline(infile, linebuffer);

        current_line++;

        if (!addBinding(linebuffer)) {
            _FB_USES_NLS;
            cerr<<_FB_CONSOLETEXT(Keys, InvalidKeyMod,
                          "Keys: Invalid key/modifier on line",
                          "A bad key/modifier string was found on line (number following)")<<" "<<
                current_line<<"): "<<linebuffer<<endl;
        }
    } // end while eof

    m_filename = filename;
    keyMode("default");
    return true;
}

/**
 * Load critical key/mouse bindings for when there are fatal errors reading the keyFile.
 */
void Keys::loadDefaults() {
#ifdef DEBUG
    cerr<<"Loading default key bindings"<<endl;
#endif
    deleteTree();
    m_map["default:"] = new t_key(0,0,0,0);
    addBinding("OnDesktop Mouse1 :HideMenus");
    addBinding("OnDesktop Mouse2 :WorkspaceMenu");
    addBinding("OnDesktop Mouse3 :RootMenu");
    keyMode("default");
}

bool Keys::save(const char *filename) const {
    //!!
    //!! TODO: fix keybinding saving
    //!! (we probably need to save key actions
    //!! as strings instead of creating new Commands)

    // open file for writing
    //    ofstream outfile(filename);
    //    if (!outfile)
    return false;
    //    return true;
}

bool Keys::addBinding(const string &linebuffer) {

    vector<string> val;
    // Parse arguments
    FbTk::StringUtil::stringtok(val, linebuffer.c_str());

    // must have at least 1 argument
    if (val.size() <= 0)
        return true; // empty lines are valid.

    if (val[0][0] == '#' || val[0][0] == '!' ) //the line is commented
        return true; // still a valid line.

    unsigned int key = 0, mod = 0;
    int type = 0, context = 0;
    size_t argc = 0;
    t_key *current_key=m_map["default:"];
    t_key *first_new_keylist = current_key, *first_new_key=0;

    if (val[0][val[0].length()-1] == ':') {
        argc++;
        keyspace_t::iterator it = m_map.find(val[0]);
        if (it == m_map.end())
            m_map[val[0]] = new t_key(0,0,0,0);
        current_key = m_map[val[0]];
    }
    // for each argument
    for (; argc < val.size(); argc++) {

        if (val[argc][0] != ':') { // parse key(s)

            int tmpmod = FbTk::KeyUtil::getModifier(val[argc].c_str());
            if(tmpmod)
                mod |= tmpmod; //If it's a modifier
            else if (strcasecmp("ondesktop", val[argc].c_str()) == 0)
                context |= ON_DESKTOP;
            else if (strcasecmp("ontoolbar", val[argc].c_str()) == 0)
                context |= ON_TOOLBAR;
            else if (strcasecmp("onwindow", val[argc].c_str()) == 0)
                context |= ON_WINDOW;
            else if (strcasecmp("NONE",val[argc].c_str())) {
                // check if it's a mouse button
                if (strcasecmp("focusin", val[argc].c_str()) == 0) {
                    context = ON_WINDOW;
                    mod = key = 0;
                    type = FocusIn;
                } else if (strcasecmp("focusout", val[argc].c_str()) == 0) {
                    context = ON_WINDOW;
                    mod = key = 0;
                    type = FocusOut;
                } else if (strcasecmp("changeworkspace",
                                      val[argc].c_str()) == 0) {
                    context = ON_DESKTOP;
                    mod = key = 0;
                    type = FocusIn;
                } else if (strcasecmp("mouseover", val[argc].c_str()) == 0) {
                    type = EnterNotify;
                    if (!(context & (ON_WINDOW|ON_TOOLBAR)))
                        context |= ON_WINDOW;
                    key = 0;
                } else if (strcasecmp("mouseout", val[argc].c_str()) == 0) {
                    type = LeaveNotify;
                    if (!(context & (ON_WINDOW|ON_TOOLBAR)))
                        context |= ON_WINDOW;
                    key = 0;
                } else if (strcasecmp(val[argc].substr(0,5).c_str(),
                                      "mouse") == 0 &&
                           val[argc].length() > 5) {
                    type = ButtonPress;
                    key = atoi(val[argc].substr(5,
                                                val[argc].length()-5).c_str());
                // keycode covers the following three two-byte cases:
                // 0x       - hex
                // +[1-9]   - number between +1 and +9
                // numbers 10 and above
                //
                } else if (val[argc].size() > 1 && (isdigit(val[argc][0]) &&
                           (isdigit(val[argc][1]) || val[argc][1] == 'x') ||
                           val[argc][0] == '+' && isdigit(val[argc][1])) ) {

                    key = strtoul(val[argc].c_str(), NULL, 0);
                    type = KeyPress;

                    if (errno == EINVAL || errno == ERANGE)
                        key = 0;

                } else { // convert from string symbol
                    key = FbTk::KeyUtil::getKey(val[argc].c_str());
                    type = KeyPress;
                }

                if (key == 0 && (type == KeyPress || type == ButtonPress))
                    return false;
                if (!first_new_key) {
                    first_new_keylist = current_key;
                    current_key = current_key->find(type, mod, key, context);
                    if (!current_key) {
                        first_new_key = new t_key(type, mod, key, context);
                        current_key = first_new_key;
                    } else if (*current_key->m_command) // already being used
                        return false;
                } else {
                    t_key *temp_key = new t_key(type, mod, key, context);
                    current_key->keylist.push_back(temp_key);
                    current_key = temp_key;
                }
                mod = 0;
                key = 0;
                type = 0;
                context = 0;
            }

        } else { // parse command line
            if (!first_new_key)
                return false;

            const char *str = FbTk::StringUtil::strcasestr(linebuffer.c_str(),
                    val[argc].c_str() + 1); // +1 to skip ':'
            if (str)
                current_key->m_command = CommandParser::instance().parseLine(str);

            if (!str || *current_key->m_command == 0 || mod) {
                delete first_new_key;
                return false;
            }

            // success
            first_new_keylist->keylist.push_back(first_new_key);
            return true;
        }  // end if
    } // end for

    return false;
}

// return true if bound to a command, else false
bool Keys::doAction(int type, unsigned int mods, unsigned int key,
                    int context) {

    static t_key* next_key = m_keylist;
    if (!next_key)
        next_key = m_keylist;

    mods = FbTk::KeyUtil::instance().cleanMods(mods);
    t_key *temp_key = next_key->find(type, mods, key, context);

    // need to save this for emacs-style keybindings
    static t_key *saved_keymode = 0;

    // grab "None Escape" to exit keychain in the middle
    unsigned int esc = FbTk::KeyUtil::getKey("Escape");

    // if focus changes, windows will get NotifyWhileGrabbed,
    // which they tend to ignore
    if (temp_key && type == KeyPress &&
        !FbTk::EventManager::instance()->grabbingKeyboard())
        XUngrabKeyboard(Fluxbox::instance()->display(), CurrentTime);

    if (temp_key && !temp_key->keylist.empty()) { // emacs-style
        if (!saved_keymode)
            saved_keymode = m_keylist;
        next_key = temp_key;
        setKeyMode(next_key);
        grabKey(esc,0);
        return true;
    }
    if (!temp_key || *temp_key->m_command == 0) {
        if (type == KeyPress && key == esc && mods == 0) {
            // if we're in the middle of an emacs-style keychain, exit it
            next_key = 0;
            if (saved_keymode) {
                setKeyMode(saved_keymode);
                saved_keymode = 0;
            }
        }
        return false;
    }

    temp_key->m_command->execute();
    if (saved_keymode) {
        if (next_key == m_keylist) // don't reset keymode if command changed it
            setKeyMode(saved_keymode);
        saved_keymode = 0;
    }
    next_key = 0;
    return true;
}

/// adds the window to m_window_map, so we know to grab buttons on it
void Keys::registerWindow(Window win, FbTk::EventHandler &h, int context) {
    m_window_map[win] = context;
    m_handler_map[win] = &h;
    grabWindow(win);
}

/// remove the window from the window map, probably being deleted
void Keys::unregisterWindow(Window win) {
    FbTk::KeyUtil::ungrabKeys(win);
    FbTk::KeyUtil::ungrabButtons(win);
    m_handler_map.erase(win);
    m_window_map.erase(win);
}

/**
 deletes the tree and load configuration
 returns true on success else false
*/
bool Keys::reconfigure(const char *filename) {
    return load(filename);
}

void Keys::keyMode(string keyMode) {
    keyspace_t::iterator it = m_map.find(keyMode + ":");
    if (it == m_map.end())
        setKeyMode(m_map["default:"]);
    else
        setKeyMode(it->second);
}

void Keys::setKeyMode(t_key *keyMode) {
    ungrabKeys();
    ungrabButtons();

    // notify handlers that their buttons have been ungrabbed
    HandlerMap::iterator h_it = m_handler_map.begin(),
                         h_it_end  = m_handler_map.end();
    for (; h_it != h_it_end; ++h_it)
        h_it->second->grabButtons();

    keylist_t::iterator it = keyMode->keylist.begin();
    keylist_t::iterator it_end = keyMode->keylist.end();
    for (; it != it_end; ++it) {
        if ((*it)->type == KeyPress)
            grabKey((*it)->key, (*it)->mod);
        else
            grabButton((*it)->key, (*it)->mod, (*it)->context);
    }
    m_keylist = keyMode;
}
            
Keys::t_key::t_key(int type_, unsigned int mod_, unsigned int key_,
                   int context_, FbTk::RefCount<FbTk::Command> command) {
    key = key_;
    mod = mod_;
    type = type_;
    context = context_ ? context_ : GLOBAL;
    m_command = command;
}

Keys::t_key::t_key(t_key *k) {
    key = k->key;
    mod = k->mod;
    type = k->type;
    context = k->context;
    m_command = k->m_command;
}

Keys::t_key::~t_key() {
    for (keylist_t::iterator list_it = keylist.begin(); list_it != keylist.end(); ++list_it)
        delete *list_it;
    keylist.clear();
}
