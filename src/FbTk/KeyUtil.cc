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

// $Id: KeyUtil.cc,v 1.1 2003/09/06 15:46:00 fluxgen Exp $

#include "KeyUtil.hh"
#include "App.hh"

#include <X11/keysym.h>

#include <iostream>
using namespace std;

namespace FbTk {

int KeyUtil::s_capslock_mod = 0;
int KeyUtil::s_numlock_mod = 0;
int KeyUtil::s_scrolllock_mod = 0;
bool KeyUtil::s_init = false;

void KeyUtil::init() {
    cerr<<"init"<<endl;
    Display *disp = FbTk::App::instance()->display();

    XModifierKeymap *modmap = XGetModifierMapping(disp);

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
    for (int i = 0, realkey = 0; i < 8; ++i) {
        for (int key = 0; key < modmap->max_keypermod; ++key, ++realkey) {

            if (modmap->modifiermap[realkey] == 0)
                continue;

            KeySym ks = XKeycodeToKeysym(disp, modmap->modifiermap[realkey], 0);

            switch (ks) {
            case XK_Caps_Lock:
                s_capslock_mod = mods[i];
                break;
            case XK_Scroll_Lock:
                s_scrolllock_mod = mods[i];
                break;
            case XK_Num_Lock:
                s_numlock_mod = mods[i];
                break;
            }
        }
    }

    s_init = true;
    XFreeModifiermap(modmap);
}

} // end namespace FbTk
