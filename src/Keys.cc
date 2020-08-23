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

#include "Keys.hh"

#include "fluxbox.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "WindowCmd.hh"
#include "Debug.hh"

#include "FbTk/EventManager.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/App.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/CommandParser.hh"
#include "FbTk/LogicCommands.hh"
#include "FbTk/I18n.hh"
#include "FbTk/AutoReloadHelper.hh"
#include "FbTk/STLUtil.hh"

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

#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <memory>

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::ifstream;
using std::pair;

using FbTk::STLUtil::destroyAndClearSecond;

namespace {

// enforces the linking of FbTk/LogicCommands
FbTk::Command<void>* link_helper = FbTk::IfCommand::parse("", "", false);



// candidate for FbTk::StringUtil ?
int extractKeyFromString(const std::string& in, const char* start_pattern, unsigned int& key) {

    int ret = 0;

    if (strstr(in.c_str(), start_pattern) != 0) {

        unsigned int tmp_key = 0;
        if (FbTk::StringUtil::extractNumber(in.substr(strlen(start_pattern)), tmp_key)) {

            key = tmp_key;
            ret = 1;
        }
    }

    return ret;
}

} // end of anonymous namespace

// helper class 'keytree'
class Keys::t_key {
public:

    // typedefs
    typedef std::list<RefKey> keylist_t;

    // constructor / destructor
    t_key(int type = 0, unsigned int mod = 0, unsigned int key = 0,
            const std::string &key_str = std::string(), int context = 0,
            bool isdouble = false, bool isPlaceHolderArg = false);

    RefKey find(int type_, unsigned int mod_, unsigned int key_,
                int context_, bool isdouble_) {
        // t_key ctor sets context_ of 0 to GLOBAL, so we must here too
        context_ = context_ ? context_ : GLOBAL;
        keylist_t::iterator itPlaceHolder = keylist.end();
        keylist_t::iterator it = keylist.begin(), it_end = keylist.end();
        for (; it != it_end; ++it) {

            if ((*it)->isPlaceHolderArg)
                itPlaceHolder = it;

            if (*it && (*it)->type == type_ && (*it)->key == key_ &&
                ((*it)->context & context_) > 0 &&
                isdouble_ == (*it)->isdouble && (*it)->mod ==
                FbTk::KeyUtil::instance().isolateModifierMask(mod_))
                return *it;
        }

        // Could not find any matching key. If a placeholder was located then user
        // is trying to pass in a value for the placeholder.
        if (itPlaceHolder == keylist.end()) {
            return RefKey();
        }
        else {
            (*itPlaceHolder)->lastPlaceHolderArgValue = key_;
            return *itPlaceHolder;
        }
    }

    // member variables

    int type; // KeyPress or ButtonPress
    unsigned int mod;
    unsigned int key; // key code or button number
    std::string key_str; // key-symbol, needed for regrab()
    int context; // ON_TITLEBAR, etc.: bitwise-or of all desired contexts
    bool isdouble;
    bool isPlaceHolderArg;
    unsigned int lastPlaceHolderArgValue;
    FbTk::RefCount<FbTk::Command<void> > m_command;

    keylist_t keylist;
};

Keys::t_key::t_key(int type_, unsigned int mod_, unsigned int key_,
                   const std::string &key_str_,
                   int context_, bool isdouble_, bool isPlaceHolderArg_) :
    type(type_),
    mod(mod_),
    key(key_),
    key_str(key_str_),
    context(context_),
    isdouble(isdouble_),
    isPlaceHolderArg(isPlaceHolderArg_),
    lastPlaceHolderArgValue(0),
    m_command(0) {

    context = context_ ? context_ : GLOBAL;
}


Keys::Keys():
    m_reloader(new FbTk::AutoReloadHelper()),
    m_keylist(0),
    next_key(0), saved_keymode(0) {
    m_reloader->setReloadCmd(FbTk::RefCount<FbTk::Command<void> >(new FbTk::SimpleCommand<Keys>(*this, &Keys::reload)));
}

Keys::~Keys() {
    ungrabKeys();
    ungrabButtons();
    deleteTree();
    delete m_reloader;
}

/// Destroys the keytree
void Keys::deleteTree() {

    m_map.clear();
    m_keylist.reset();
    next_key.reset();
    saved_keymode.reset();
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
                                      ButtonPressMask|ButtonReleaseMask|ButtonMotionMask);
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
    t_key::keylist_t::iterator it = m_keylist->keylist.begin();
    t_key::keylist_t::iterator it_end = m_keylist->keylist.end();
    for (; it != it_end; ++it) {
        // keys are only grabbed in global context
        if ((win_it->second & Keys::GLOBAL) > 0 && (*it)->type == KeyPress)
            FbTk::KeyUtil::grabKey((*it)->key, (*it)->mod, win);
        // ON_DESKTOP buttons don't need to be grabbed
        else if ((win_it->second & (*it)->context & ~Keys::ON_DESKTOP) > 0) {

            if ((*it)->type == ButtonPress || (*it)->type == ButtonRelease || (*it)->type == MotionNotify) {
                FbTk::KeyUtil::grabButton((*it)->key, (*it)->mod, win, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask);
            }
        }
    }
}

/**
    Load and grab keys
    TODO: error checking
*/
void Keys::reload() {
    // an intentionally empty file will still have one root mapping
    bool firstload = m_map.empty();

    if (m_filename.empty()) {
        if (firstload)
            loadDefaults();
        return;
    }

    FbTk::App::instance()->sync(false);

    if (! FbTk::FileUtil::isRegularFile(m_filename.c_str())) {
        return;
    }

    // open the file
    ifstream infile(m_filename.c_str());
    if (!infile) {
        if (firstload)
            loadDefaults();
        return; // failed to open file
    }

    // free memory of previous grabs
    deleteTree();

    m_map["default:"] = FbTk::makeRef<t_key>();

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

    keyMode("default");
}

/**
 * Load critical key/mouse bindings for when there are fatal errors reading the keyFile.
 */
void Keys::loadDefaults() {
    fbdbg<<"Loading default key bindings"<<endl;

    deleteTree();
    m_map["default:"] = FbTk::makeRef<t_key>();
    addBinding("OnDesktop Mouse1 :HideMenus");
    addBinding("OnDesktop Mouse2 :WorkspaceMenu");
    addBinding("OnDesktop Mouse3 :RootMenu");
    addBinding("OnTitlebar Mouse3 :WindowMenu");
    addBinding("OnWindow Mouse1 :MacroCmd {Focus} {Raise} {StartMoving}");
    addBinding("OnTitlebar Mouse1 :MacroCmd {Focus} {Raise} {ActivateTab}");
    addBinding("OnTitlebar Move1 :StartMoving");
    addBinding("OnLeftGrip Move1 :StartResizing bottomleft");
    addBinding("OnRightGrip Move1 :StartResizing bottomright");
    addBinding("OnWindowBorder Move1 :StartMoving");
    addBinding("Mod1 Tab :NextWindow (workspace=[current])");
    addBinding("Mod1 Shift Tab :PrevWindow (workspace=[current])");
    keyMode("default");
}

bool Keys::addBinding(const string &linebuffer) {

    // Parse arguments
    vector<string> val;
    FbTk::StringUtil::stringtok(val, linebuffer);

    // must have at least 1 argument
    if (val.empty())
        return true; // empty lines are valid.

    if (val[0][0] == '#' || val[0][0] == '!' ) //the line is commented
        return true; // still a valid line.

    unsigned int key = 0, mod = 0;
    int type = 0, context = 0;
    bool isdouble = false;
    size_t argc = 0;
    RefKey current_key = m_map["default:"];
    RefKey first_new_keylist = current_key, first_new_key;

    if (val[0][val[0].length()-1] == ':') {
        argc++;
        keyspace_t::iterator it = m_map.find(val[0]);
        if (it == m_map.end())
            m_map[val[0]] = FbTk::makeRef<t_key>();
        current_key = m_map[val[0]];
    }
    // for each argument
    for (; argc < val.size(); argc++) {

        std::string arg = FbTk::StringUtil::toLower(val[argc]);

        bool isPlaceHolderArg = false;

        if (arg[0] != ':') { // parse key(s)

            std::string key_str;

            int tmpmod = FbTk::KeyUtil::getModifier(arg.c_str());
            if(tmpmod)
                mod |= tmpmod; //If it's a modifier
            else if (arg == "ondesktop")
                context |= ON_DESKTOP;
            else if (arg == "ontoolbar")
                context |= ON_TOOLBAR;
            else if (arg == "onslit")
                context |= ON_SLIT;
            else if (arg == "onwindow")
                context |= ON_WINDOW;
            else if (arg == "ontitlebar")
                context |= ON_TITLEBAR;
            else if (arg == "onwinbutton")
                context |= ON_WINBUTTON;
            else if (arg == "onminbutton")
                context |= ON_MINBUTTON; // one char diff, oh great ... ... blast!
            else if (arg == "onmaxbutton")
                context |= ON_MAXBUTTON;
            else if (arg == "onwindowborder")
                context |= ON_WINDOWBORDER;
            else if (arg == "onleftgrip")
                context |= ON_LEFTGRIP;
            else if (arg == "onrightgrip")
                context |= ON_RIGHTGRIP;
            else if (arg == "ontab")
                context |= ON_TAB;
            else if (arg == "double")
                isdouble = true;
            else if (arg != "none") {
                if (arg == "focusin") {
                    context = ON_WINDOW;
                    mod = key = 0;
                    type = FocusIn;
                } else if (arg == "focusout") {
                    context = ON_WINDOW;
                    mod = key = 0;
                    type = FocusOut;
                } else if (arg == "changeworkspace") {
                    context = ON_DESKTOP;
                    mod = key = 0;
                    type = FocusIn;
                } else if (arg == "mouseover") {
                    type = EnterNotify;
                    if (!(context & (ON_WINDOW|ON_TOOLBAR|ON_SLIT)))
                        context |= ON_WINDOW;
                    key = 0;
                } else if (arg == "mouseout") {
                    type = LeaveNotify;
                    if (!(context & (ON_WINDOW|ON_TOOLBAR|ON_SLIT)))
                        context |= ON_WINDOW;
                    key = 0;

                // check if it's a mouse button
                } else if (extractKeyFromString(arg, "mouse", key)) {
                    type = ButtonPress;

                    // fluxconf mangles things like OnWindow Mouse# to Mouse#ow
                    if (strstr(arg.c_str(), "top"))
                        context = ON_DESKTOP;
                    else if (strstr(arg.c_str(), "ebar"))
                        context = ON_TITLEBAR;
                    else if (strstr(arg.c_str(), "bar"))
                        context = ON_TOOLBAR;
                    else if (strstr(arg.c_str(), "slit"))
                        context = ON_SLIT;
                    else if (strstr(arg.c_str(), "ow"))
                        context = ON_WINDOW;
                } else if (extractKeyFromString(arg, "click", key)) {
                    type = ButtonRelease;
                } else if (extractKeyFromString(arg, "move", key)) {
                    type = MotionNotify;
                } else if (arg == "arg") {
                    isPlaceHolderArg = true;
                    key = 0;
                    mod = 0;
                    type = 0;
                } else if ((key = FbTk::KeyUtil::getKey(val[argc].c_str()))) { // convert from string symbol
                    type = KeyPress;
                    key_str = val[argc];

                // keycode covers the following three two-byte cases:
                // 0x       - hex
                // +[1-9]   - number between +1 and +9
                // numbers 10 and above
                //
                } else {
                    FbTk::StringUtil::extractNumber(arg, key);
                    type = KeyPress;
                }

                if (key == 0 && (type == KeyPress || type == ButtonPress || type == ButtonRelease) && !isPlaceHolderArg)
                    return false;

                if (type != ButtonPress)
                    isdouble = false;

                // Placeholder argument cannot be the first key
                if (!first_new_key && isPlaceHolderArg) {
                    return false;
                }

                if (!first_new_key) {
                    first_new_keylist = current_key;
                    current_key = current_key->find(type, mod, key, context,
                                                    isdouble);
                    if (!current_key) {
                        first_new_key.reset( new t_key(type, mod, key, key_str, context,
                                                  isdouble, isPlaceHolderArg) );
                        current_key = first_new_key;
                    } else if (current_key->m_command) // already being used
                        return false;
                } else {

                    RefKey temp_key( new t_key(type, mod, key, key_str, context,
                                                isdouble, isPlaceHolderArg) );
                    current_key->keylist.push_back(temp_key);
                    current_key = temp_key;
                }
                mod = 0;
                key = 0;
                context = 0;
                isdouble = false;
            }

        } else { // parse command line
            if (!first_new_key)
                return false;

            const char *str = FbTk::StringUtil::strcasestr(linebuffer.c_str(),
                   val[argc].c_str());
            if (str) // +1 to skip ':'
                current_key->m_command.reset(FbTk::CommandParser<void>::instance().parse(str + 1));

            if (!str || current_key->m_command == 0 || mod)
                return false;

            // success
            first_new_keylist->keylist.push_back(first_new_key);
            return true;
        }  // end if
    } // end for

    return false;
}

// return true if bound to a command, else false
bool Keys::doAction(int type, unsigned int mods, unsigned int key,
                    int context, WinClient *current, Time time) {

    if (!m_keylist)
        return false;

    static Time first_key_time = 0;

    static Time last_button_time = 0;
    static unsigned int last_button = 0;

    // need to remember whether or not this is a double-click, e.g. when
    // double-clicking on the titlebar when there's an OnWindow Double command
    // we just don't update it if timestamp is the same
    static bool double_click = false;

    // actual value used for searching
    bool isdouble = false;

    if (type == ButtonPress) {
        if (time > last_button_time) {
            double_click = (time - last_button_time <
                Fluxbox::instance()->getDoubleClickInterval()) &&
                last_button == key;
        }
        last_button_time = time;
        last_button = key;
        isdouble = double_click;
    }

    auto resetKeyChain = [&]() {
        first_key_time = 0;
        next_key.reset();
        if (saved_keymode) {
            setKeyMode(saved_keymode);
            saved_keymode.reset();
        }
    };
    if (type == KeyPress && first_key_time && time - first_key_time > 5000)
        resetKeyChain();

    if (!next_key)
        next_key = m_keylist;

    mods = FbTk::KeyUtil::instance().cleanMods(mods);
    RefKey temp_key = next_key->find(type, mods, key, context, isdouble);

    // just because we double-clicked doesn't mean we shouldn't look for single
    // click commands
    if (!temp_key && isdouble)
        temp_key = next_key->find(type, mods, key, context, false);

    if (!temp_key && type == ButtonPress && // unassigned button press
        next_key->find(MotionNotify, mods, key, context, false))
        return true; // if there's a motion action, prevent replay to the client (but do nothing)

    if (temp_key && !temp_key->keylist.empty()) { // emacs-style
        if (!saved_keymode) {
            first_key_time = time;
            saved_keymode = m_keylist;
        }
        next_key = temp_key;
        setKeyMode(next_key);
        return true;
    }
    if (!temp_key || temp_key->m_command == 0) {
        if (type == KeyPress &&
            !FbTk::KeyUtil::instance().keycodeToModmask(key)) {
            // if we're in the middle of an emacs-style keychain, exit it
            resetKeyChain();
        }
        return false;
    }

    // if focus changes, windows will get NotifyWhileGrabbed,
    // which they tend to ignore
    if (type == KeyPress) {
        XUngrabKeyboard(Fluxbox::instance()->display(), CurrentTime);
    }

    WinClient *old = WindowCmd<void>::client();
    WindowCmd<void>::setClient(current);

    // The key is a placeholder, store the value of the entered key in the shortcut manager
    // before executing the action
    if (temp_key->isPlaceHolderArg) {
        fbdbg << "Encountered placeholder key. Assign value[" << temp_key->lastPlaceHolderArgValue
              << "] to the placeholder" << std::endl;
        Fluxbox::instance()->shortcutManager().setLastPlaceHolderKey(temp_key->lastPlaceHolderArgValue);
    }

    temp_key->m_command->execute();
    WindowCmd<void>::setClient(old);

    if (saved_keymode) {
        if (next_key == m_keylist) // don't reset keymode if command changed it
            setKeyMode(saved_keymode);
        saved_keymode.reset();
    }
    next_key.reset();
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
void Keys::reconfigure() {
    m_filename = FbTk::StringUtil::expandFilename(Fluxbox::instance()->getKeysFilename());
    m_reloader->setMainFile(m_filename);
    m_reloader->checkReload();
}

void Keys::regrab() {
    setKeyMode(m_keylist);
}

void Keys::keyMode(const string& keyMode) {
    keyspace_t::iterator it = m_map.find(keyMode + ":");
    if (it == m_map.end())
        setKeyMode(m_map["default:"]);
    else
        setKeyMode(it->second);
}

void Keys::setKeyMode(const FbTk::RefCount<t_key> &keyMode) {
    ungrabKeys();
    ungrabButtons();

    // notify handlers that their buttons have been ungrabbed
    HandlerMap::iterator h_it = m_handler_map.begin(),
                         h_it_end  = m_handler_map.end();
    for (; h_it != h_it_end; ++h_it)
        h_it->second->grabButtons();

    t_key::keylist_t::iterator it = keyMode->keylist.begin();
    t_key::keylist_t::iterator it_end = keyMode->keylist.end();
    for (; it != it_end; ++it) {
        RefKey t = *it;
        if (t->type == KeyPress) {
            if (!t->key_str.empty()) {
                int key = FbTk::KeyUtil::getKey(t->key_str.c_str());
                t->key = key;
            }
            grabKey(t->key, t->mod);
        } else {
            grabButton(t->key, t->mod, t->context);
        }
    }
    m_keylist = keyMode;
}

