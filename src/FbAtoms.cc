// FbAtom.cc
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

#include "FbAtoms.hh"
#include "App.hh"

#include <string>

using std::string;

FbAtoms *FbAtoms::s_singleton = 0;

FbAtoms::FbAtoms():m_init(false) {
    if (s_singleton != 0)
        throw string("You can only create one instance of FbAtoms");

    s_singleton = this;
    initAtoms();
}

FbAtoms::~FbAtoms() {

}

FbAtoms *FbAtoms::instance() {
    if (s_singleton == 0)
        throw string("Create one instance of FbAtoms first!");
    return s_singleton;
}

void FbAtoms::initAtoms() {
    Display *display = FbTk::App::instance()->display();
    if (display == 0)
        return;

    xa_wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    xa_wm_state = XInternAtom(display, "WM_STATE", False);
    xa_wm_change_state = XInternAtom(display, "WM_CHANGE_STATE", False);
    xa_wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    xa_wm_take_focus = XInternAtom(display, "WM_TAKE_FOCUS", False);
    motif_wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", False);

    blackbox_attributes = XInternAtom(display, "_BLACKBOX_ATTRIBUTES", False);

}
