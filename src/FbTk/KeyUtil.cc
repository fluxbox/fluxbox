// KeyUtil.cc for FbTk
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "KeyUtil.hh"
#include "App.hh"

#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <string>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

namespace {

struct t_modlist{
    const char *str;
    unsigned int mask;
    bool operator == (const char *modstr) const {
        return  (strcasecmp(str, modstr) == 0 && mask !=0);
    }
};

const struct t_modlist modlist[] = {
    {"shift", ShiftMask},
    {"lock", LockMask},
    {"control", ControlMask},
    {"mod1", Mod1Mask},
    {"mod2", Mod2Mask},
    {"mod3", Mod3Mask},
    {"mod4", Mod4Mask},
    {"mod5", Mod5Mask},
    {"alt", Mod1Mask},
    {"ctrl", ControlMask},
    {0, 0}
};

}

namespace FbTk {

std::unique_ptr<KeyUtil> KeyUtil::s_keyutil;

KeyUtil &KeyUtil::instance() {
    if (s_keyutil.get() == 0)
        s_keyutil.reset(new KeyUtil());
    return *s_keyutil.get();
}


KeyUtil::KeyUtil()
    : m_modmap(0), m_numlock(0), m_scrolllock(0)
{
    init();
}

void KeyUtil::init() {
    loadModmap();
}

KeyUtil::~KeyUtil() {
    if (m_modmap)
        XFreeModifiermap(m_modmap);
}

void KeyUtil::loadModmap() {
    if (m_modmap)
        XFreeModifiermap(m_modmap);

    m_modmap = XGetModifierMapping(App::instance()->display());

    // find modifiers and set them
    for (int i=0, realkey=0; i<8; ++i) {
        for (int key=0; key<m_modmap->max_keypermod; ++key, ++realkey) {

            if (m_modmap->modifiermap[realkey] == 0)
                continue;

            KeySym ks = XkbKeycodeToKeysym(App::instance()->display(),
                    m_modmap->modifiermap[realkey], 0, 0);

            switch (ks) {
            // we just want to clean the Lock modifier, not specifically the
            // XK_Caps_Lock key
            // the others tend to vary from distro to distro, though
            case XK_Scroll_Lock:
                m_scrolllock = modlist[i].mask;
                break;
            case XK_Num_Lock:
                m_numlock = modlist[i].mask;
                break;
            }
        }
    }
}


/**
 Grabs a key with the modifier
 and with numlock,capslock and scrollock
*/
void KeyUtil::grabKey(unsigned int key, unsigned int mod, Window win) {
    Display *display = App::instance()->display();
    const unsigned int nummod = instance().numlock();
    const unsigned int scrollmod = instance().scrolllock();

    // Grab with numlock, capslock and scrlock
    for (int i = 0; i < 8; i++) {
        XGrabKey(display, key, mod | (i & 1 ? LockMask : 0) |
                 (i & 2 ? nummod : 0) | (i & 4 ? scrollmod : 0),
                 win, True, GrabModeAsync, GrabModeSync);
    }

}

void KeyUtil::grabButton(unsigned int button, unsigned int mod, Window win,
                         unsigned int event_mask, Cursor cursor) {
    Display *display = App::instance()->display();
    const unsigned int nummod = instance().numlock();
    const unsigned int scrollmod = instance().scrolllock();

    // Grab with numlock, capslock and scrlock
    for (int i = 0; i < 8; i++) {
        XGrabButton(display, button, mod | (i & 1 ? LockMask : 0) |
                    (i & 2 ? nummod : 0) | (i & 4 ? scrollmod : 0),
                    win, False, event_mask, GrabModeSync, GrabModeAsync,
                    None, cursor);
    }

}

/**
 @return keycode of keystr on success else 0
*/

unsigned int KeyUtil::getKey(const char *keystr) {

    KeyCode code = 0;

    if (keystr) {

        KeySym sym = XStringToKeysym(keystr);
        if (sym != NoSymbol) {
            code = XKeysymToKeycode(App::instance()->display(), sym);
        }
    }

    return code;
}


/**
 @return the modifier for the modstr else zero on failure.
*/
unsigned int KeyUtil::getModifier(const char *modstr) {
    if (!modstr)
        return 0;

    // find mod mask string
    for (unsigned int i=0; modlist[i].str !=0; i++) {
        if (modlist[i] == modstr)
            return modlist[i].mask;
    }

    return 0;
}

/// Ungrabs the keys
void KeyUtil::ungrabKeys(Window win) {
    Display * display = App::instance()->display();
    XUngrabKey(display, AnyKey, AnyModifier, win);
}

void KeyUtil::ungrabButtons(Window win) {
    Display * display = App::instance()->display();
    XUngrabButton(display, AnyButton, AnyModifier, win);
}

unsigned int KeyUtil::keycodeToModmask(unsigned int keycode) {
    XModifierKeymap *modmap = instance().m_modmap;

    if (!modmap)
        return 0;

    // search through modmap for this keycode
    for (int mod=0; mod < 8; mod++) {
        for (int key=0; key < modmap->max_keypermod; ++key) {
            // modifiermap is an array with 8 sets of keycodes
            // each max_keypermod long, but in a linear array.
            if (modmap->modifiermap[modmap->max_keypermod*mod + key] == keycode) {
                return modlist[mod].mask;
            }
        }
    }
    // no luck
    return 0;
}



} // end namespace FbTk
