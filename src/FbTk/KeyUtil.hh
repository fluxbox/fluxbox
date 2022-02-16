// KeyUtil.hh for FbTk
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_KEYUTIL_HH
#define FBTK_KEYUTIL_HH

#include <X11/Xlib.h>

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
    static void grabKey(unsigned int key, unsigned int mod, Window win);
    static void grabButton(unsigned int button, unsigned int mod, Window win,
                           unsigned int event_mask, Cursor cursor = None);

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
    static void ungrabKeys(Window win);
    static void ungrabButtons(Window win);

    /** 
        Strip out modifiers we want to ignore
        @return the cleaned state number
    */
    unsigned int cleanMods(unsigned int mods) {
        // remove numlock, capslock, and scrolllock
        // and anything beyond Button5Mask
        return mods & ~(capslock() | numlock() | scrolllock()) & ((1<<13) - 1);
    }

    /** 
       strip away everything which is actually not a modifier
       eg, xkb-keyboardgroups are encoded as bit 13 and 14
    */
    unsigned int isolateModifierMask(unsigned int mods) {
        return mods & (ShiftMask|LockMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask); 
    }

    /**
       Convert the specified key into appropriate modifier mask
       @return corresponding modifier mask
    */
    static unsigned int keycodeToModmask(unsigned int keycode);
    int numlock() const { return m_numlock; }
    int capslock() const { return LockMask; }
    int scrolllock() const { return m_scrolllock; }

private:
    void loadModmap();

    XModifierKeymap *m_modmap;
    int m_numlock, m_scrolllock;
    static std::unique_ptr<KeyUtil> s_keyutil;
};

} // end namespace FbTk


#endif // FBTK_KEYUTIL_HH
