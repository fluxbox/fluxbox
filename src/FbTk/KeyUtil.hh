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

// $Id: KeyUtil.hh,v 1.1 2003/09/06 15:46:00 fluxgen Exp $

#ifndef FBTK_KEYUTIL_HH
#define FBTK_KEYUTIL_HH

namespace FbTk {

class KeyUtil {
public:
    /** 
        Strip out modifiers we want to ignore
        @return the cleaned state number
    */
    static unsigned int cleanMods(unsigned int mods) {
        if (!s_init)
            init();
        //remove numlock, capslock and scrolllock
         return mods & (~s_capslock_mod & ~s_numlock_mod & ~s_scrolllock_mod);
    }

    static int capslockMod() { if (!s_init) init(); return s_capslock_mod; }
    static int numlockMod() { if (!s_init) init(); return s_numlock_mod; }
    static int scrolllockMod() { if (!s_init) init(); return s_scrolllock_mod; }
    /// so one can force a reinit of modifiers
    static void init();    
private:
    static int s_capslock_mod, s_numlock_mod, s_scrolllock_mod; ///< modifiers
    static bool s_init;
};

} // end namespace FbTk


#endif // FBTK_KEYUTIL_HH
