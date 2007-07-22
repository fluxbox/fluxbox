// FbAtom.hh
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$
#ifndef FBATOMS_HH
#define FBATOMS_HH

#include <X11/Xlib.h>
#include <X11/Xatom.h>

/// atom handler for basic X atoms
class FbAtoms {
public:
    FbAtoms();
    ~FbAtoms();

    static FbAtoms *instance();

    inline Atom getWMChangeStateAtom() const { return xa_wm_change_state; }
    inline Atom getWMStateAtom() const { return xa_wm_state; }
    inline Atom getWMDeleteAtom() const { return xa_wm_delete_window; }
    inline Atom getWMProtocolsAtom() const { return xa_wm_protocols; }
    inline Atom getWMTakeFocusAtom() const { return xa_wm_take_focus; }

    inline Atom getMWMHintsAtom() const { return motif_wm_hints; }

    // these atoms are for normal app->WM interaction beyond the scope of the
    // ICCCM...
    inline Atom getFluxboxAttributesAtom() const { return blackbox_attributes; }

private:
    void initAtoms();
// NETAttributes
    Atom blackbox_attributes;
    Atom motif_wm_hints;

    Atom xa_wm_protocols, xa_wm_state,
        xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state;
       
    bool m_init;
    static FbAtoms *s_singleton;
};

#endif //FBATOMS_HH
