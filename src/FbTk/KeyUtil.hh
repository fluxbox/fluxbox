// KeyUtil.hh for FbTk
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

// $Id: KeyUtil.hh,v 1.4 2003/12/16 17:06:51 fluxgen Exp $

#ifndef FBTK_KEYUTIL_HH
#define FBTK_KEYUTIL_HH

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <memory>

namespace FbTk {

class KeyUtil {
public:

    KeyUtil();
    ~KeyUtil();

    void init();
    static KeyUtil &instance();

    /**
       Grab the specified key
    */
    static void grabKey(unsigned int key, unsigned int mod);

    /**
       convert the string to the keysym
       @return the keysym corresponding to the string, or zero
    */
    static unsigned int getKey(const char *keystr);

    /**
       @return the modifier for the modstr else zero on failure.
    */
    static unsigned int getModifier(const char *modstr);

    /**
       ungrabs all keys
     */
    static void ungrabKeys();

    /** 
        Strip out modifiers we want to ignore
        @return the cleaned state number
    */
    static unsigned int cleanMods(unsigned int mods) {
        //remove numlock(Mod2), capslock and scrolllock(Mod5)
         return mods & ~(LockMask | Mod2Mask | Mod5Mask);
    }

    /**
       Convert the specified key into appropriate modifier mask
       @return corresponding modifier mask
    */
    static unsigned int keycodeToModmask(unsigned int keycode);

private:
    void loadModmap();

    XModifierKeymap *m_modmap;
    static std::auto_ptr<KeyUtil> s_keyutil;
};

} // end namespace FbTk


#endif // FBTK_KEYUTIL_HH
