// Keys.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef KEYS_HH
#define KEYS_HH

#include "FbTk/NotCopyable.hh"
#include "FbTk/RefCount.hh"

#include <X11/Xlib.h>
#include <string>
#include <map>

class WinClient;

namespace FbTk {
    class EventHandler;
    class AutoReloadHelper;
}

class Keys:private FbTk::NotCopyable  {
public:

    // contexts for events
    // it's ok if there is overlap; it will be worked out in t_key::find()
    // eventHandlers should submit bitwise-or of contexts the event happened in
    enum {
        GLOBAL =            1 << 0,
        ON_DESKTOP =        1 << 1,
        ON_TOOLBAR =        1 << 2,
        ON_ICONBUTTON =     1 << 3,
        ON_TITLEBAR =       1 << 4,
        ON_WINDOW =         1 << 5,
        ON_WINDOWBORDER =   1 << 6,
        ON_LEFTGRIP =       1 << 7,
        ON_RIGHTGRIP =      1 << 8,
        ON_TAB =            1 << 9,
        ON_SLIT =           1 << 10,
        ON_WINBUTTON =      1 << 11,
        ON_MINBUTTON =      1 << 12,
        ON_MAXBUTTON =      1 << 13
        // and so on...
    };

    /// constructor
    explicit Keys();
    /// destructor
    ~Keys();

    /// bind a key action from a string
    /// @return false on failure
    bool addBinding(const std::string &binding);

    /**
       do action from XKeyEvent; return false if not bound to anything
    */
    bool doAction(int type, unsigned int mods, unsigned int key, int context,
                  WinClient *current = 0, Time time = 0);

    /// register a window so that proper keys/buttons get grabbed on it
    void registerWindow(Window win, FbTk::EventHandler &handler, int context);
    /// unregister window
    void unregisterWindow(Window win);

    /// grab keys again when keymap changes
    void regrab();

    const std::string& filename() const { return m_filename; }
    /**
       Load configuration from file
    */
    void reload();
    /**
       Reload configuration if keys file has changed
    */
    void reconfigure();
    void keyMode(const std::string& keyMode);

    bool inKeychain() const { return saved_keymode != 0; }

private:
    class t_key; // helper class to build a 'keytree'
    typedef FbTk::RefCount<t_key> RefKey;
    typedef std::map<std::string, RefKey> keyspace_t;
    typedef std::map<Window, int> WindowMap;
    typedef std::map<Window, FbTk::EventHandler*> HandlerMap;

    void deleteTree();

    void grabKey(unsigned int key, unsigned int mod);
    void ungrabKeys();
    void grabButton(unsigned int button, unsigned int mod, int context);
    void ungrabButtons();
    void grabWindow(Window win);

    // Load default keybindings for when there are errors loading the keys file
    void loadDefaults();
    void setKeyMode(const FbTk::RefCount<t_key> &keyMode);


    // member variables
    std::string m_filename;
    FbTk::AutoReloadHelper* m_reloader;
    RefKey m_keylist;
    keyspace_t m_map;

    RefKey next_key;
    RefKey saved_keymode;

    WindowMap m_window_map;
    HandlerMap m_handler_map;
};

#endif // KEYS_HH
