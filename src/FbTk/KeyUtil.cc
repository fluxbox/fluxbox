// KeyUtil.cc for FbTk
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: KeyUtil.cc,v 1.6 2003/12/31 11:57:47 fluxgen Exp $

#include "KeyUtil.hh"
#include "App.hh"

#include <string>

namespace FbTk {

std::auto_ptr<KeyUtil> KeyUtil::s_keyutil;

KeyUtil &KeyUtil::instance() {
    if (s_keyutil.get() == 0)
        s_keyutil.reset(new KeyUtil());
    return *s_keyutil.get();
}


KeyUtil::KeyUtil()
    : m_modmap(0)
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
    // mask to use for modifier
    int mods[] = {
        ShiftMask,
        LockMask,
        ControlMask,
        Mod1Mask,
        Mod2Mask,
        Mod3Mask,
        Mod4Mask,
        Mod5Mask,
        0
    };
	
    // find modifiers and set them
    for (int i=0, realkey=0; i<8; ++i) {
        for (int key=0; key<m_modmap->max_keypermod; ++key, ++realkey) {

            if (m_modmap->modifiermap[realkey] == 0)
                continue;

            KeySym ks = XKeycodeToKeysym(App::instance()->display(), m_modmap->modifiermap[realkey], 0);

            switch (ks) {
            case XK_Caps_Lock:
                m_capslock = mods[i];
                break;
            case XK_Scroll_Lock:
                m_scrolllock = mods[i];
                break;
            case XK_Num_Lock:
                m_numlock = mods[i];
                break;
            }
        }
    }
}



/**
 Grabs a key with the modifier
 and with numlock,capslock and scrollock
*/
void KeyUtil::grabKey(unsigned int key, unsigned int mod) {
    Display *display = App::instance()->display();
    const unsigned int capsmod = instance().capslock();
    const unsigned int nummod = instance().numlock();
    const unsigned int scrollmod = instance().scrolllock();
    
    for (int screen=0; screen<ScreenCount(display); screen++) {
		
        Window root = RootWindow(display, screen);
		
        XGrabKey(display, key, mod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);
						
        // Grab with numlock, capslock and scrlock	

        //numlock	
        XGrabKey(display, key, mod|nummod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);		
        //scrolllock
        XGrabKey(display, key, mod|scrollmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);	
        //capslock
        XGrabKey(display, key, mod|capsmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);
	
        //capslock+numlock
        XGrabKey(display, key, mod|capsmod|nummod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);

        //capslock+scrolllock
        XGrabKey(display, key, mod|capsmod|scrollmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);						
	
        //capslock+numlock+scrolllock
        XGrabKey(display, key, mod|capsmod|scrollmod|nummod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);						

        //numlock+scrollLock
        XGrabKey(display, key, mod|nummod|scrollmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);
	
    }
			
}

/**
 @return keycode of keystr on success else 0
*/
unsigned int KeyUtil::getKey(const char *keystr) {
    if (!keystr)
        return 0;
    return XKeysymToKeycode(App::instance()->display(),
                            XStringToKeysym(keystr));
}

/**
 @return the modifier for the modstr else zero on failure.
*/
unsigned int KeyUtil::getModifier(const char *modstr) {
    if (!modstr)
        return 0;
    struct t_modlist{
        char *str;
        unsigned int mask;
        bool operator == (const char *modstr) {
            return  (strcasecmp(str, modstr) == 0 && mask !=0);
        }
    } modlist[] = {
        {"SHIFT", ShiftMask},
        {"CONTROL", ControlMask},
        {"MOD1", Mod1Mask},
        {"MOD2", Mod2Mask},
        {"MOD3", Mod3Mask},
        {"MOD4", Mod4Mask},
        {"MOD5", Mod5Mask},
        {0, 0}
    };

    // find mod mask string
    for (unsigned int i=0; modlist[i].str !=0; i++) {
        if (modlist[i] == modstr)		
            return modlist[i].mask;		
    }
	
    return 0;	
}

/// Ungrabs the keys
void KeyUtil::ungrabKeys() {
    Display * display = App::instance()->display();
    for (int screen=0; screen<ScreenCount(display); screen++) {
        XUngrabKey(display, AnyKey, AnyModifier,
                   RootWindow(display, screen));		
    }
}

unsigned int KeyUtil::keycodeToModmask(unsigned int keycode) {
    XModifierKeymap *modmap = instance().m_modmap;

    if (!modmap) return 0;

    // search through modmap for this keycode
    for (int mod=0; mod < 8; mod++) {
        for (int key=0; key < modmap->max_keypermod; ++key) {
            // modifiermap is an array with 8 sets of keycodes
            // each max_keypermod long, but in a linear array.
            if (modmap->modifiermap[modmap->max_keypermod*mod + key] == keycode) {
                return (1<<mod);
            }
        } 
    }
    // no luck
    return 0;
}



} // end namespace FbTk
