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

#ifndef FBATOMS_HH
#define FBATOMS_HH

#include <X11/Xlib.h>

/// atom handler for basic X atoms
class FbAtoms {
public:
    ~FbAtoms();

    static FbAtoms *instance();

    Atom getWMChangeStateAtom() const { return xa_wm_change_state; }
    Atom getWMStateAtom() const { return xa_wm_state; }
    Atom getWMDeleteAtom() const { return xa_wm_delete_window; }
    Atom getWMProtocolsAtom() const { return xa_wm_protocols; }
    Atom getWMTakeFocusAtom() const { return xa_wm_take_focus; }

    Atom getMWMHintsAtom() const { return motif_wm_hints; }

    // these atoms are for normal app->WM interaction beyond the scope of the
    // ICCCM...
    Atom getFluxboxAttributesAtom() const { return blackbox_attributes; }

private:
    FbAtoms();

    Atom blackbox_attributes;
    Atom motif_wm_info;
    Atom motif_wm_hints;
    Atom xa_wm_protocols;
    Atom xa_wm_state;
    Atom xa_wm_delete_window;
    Atom xa_wm_take_focus;
    Atom xa_wm_change_state;
};

#endif //FBATOMS_HH

