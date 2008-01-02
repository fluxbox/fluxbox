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

// $Id$

#ifndef KEYS_HH
#define KEYS_HH

#include "FbTk/NotCopyable.hh"

#include <X11/Xlib.h>
#include <string>
#include <map>


namespace FbTk {
    class EventHandler;
}

class Keys:private FbTk::NotCopyable  {
public:

    // contexts for events
    // it's ok if there is overlap; it will be worked out in t_key::find()
    // eventHandlers should submit bitwise-or of contexts the event happened in
    enum {
        GLOBAL = 0x01,
        ON_DESKTOP = 0x02,
        ON_TOOLBAR = 0x04,
        ON_ICONBUTTON = 0x08,
        ON_TITLEBAR = 0x10,
        ON_WINDOW = 0x20,
        ON_TAB = 0x40,
        ON_SLIT = 0x80
        // and so on...
    };

    /// constructor
    explicit Keys();
    /// destructor
    ~Keys();

    /**
       Load configuration from file
       @return true on success, else false
    */
    bool load(const char *filename = 0);
    /**
       Save keybindings to a file
       Note: the file will be overwritten
       @return true on success, else false
     */
    bool save(const char *filename = 0) const;
    /// bind a key action from a string
    /// @return false on failure
    bool addBinding(const std::string &binding);

    /**
       do action from XKeyEvent; return false if not bound to anything
    */
    bool doAction(int type, unsigned int mods, unsigned int key, int context,
                  Time time = 0);

    /// register a window so that proper keys/buttons get grabbed on it
    void registerWindow(Window win, FbTk::EventHandler &handler, int context);
    /// unregister window
    void unregisterWindow(Window win);

    /**
       Reload configuration from filename
       @return true on success, else false
    */
    bool reconfigure(const char *filename);
    const std::string& filename() const { return m_filename; }
    void keyMode(const std::string& keyMode);
private:
    class t_key; // helper class to build a 'keytree'
    typedef std::map<std::string, t_key *> keyspace_t;
    typedef std::map<Window, int> WindowMap;
    typedef std::map<Window, FbTk::EventHandler*> HandlerMap;

    void deleteTree();

    void grabKey(unsigned int key, unsigned int mod);
    void ungrabKeys();
    void grabButton(unsigned int button, unsigned int mod, int context);
    void ungrabButtons();
    void grabWindow(Window win);

    // Load default keybindings for when there are errors loading the initial one
    void loadDefaults();
    void setKeyMode(t_key *keyMode);


    // member variables
    std::string m_filename;
    t_key *m_keylist;
    keyspace_t m_map;

    WindowMap m_window_map;
    HandlerMap m_handler_map;
};

#endif // KEYS_HH
