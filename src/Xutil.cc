// Xutil.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003-2004 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: Xutil.cc,v 1.3 2004/01/11 16:04:39 fluxgen Exp $

#include "Xutil.hh"

#include "I18n.hh"
#include "App.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

namespace Xutil {

std::string getWMName(Window window) {

    if (window == None)
        return "";

    Display *display = FbTk::App::instance()->display();

    XTextProperty text_prop;
    text_prop.value = 0;
    char **list;
    int num;
    I18n *i18n = I18n::instance();
    std::string name;

    if (XGetWMName(display, window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
				
                text_prop.nitems = strlen((char *) text_prop.value);
				
                if ((XmbTextPropertyToTextList(display, &text_prop,
                                               &list, &num) == Success) &&
                    (num > 0) && *list) {
                    name = static_cast<char *>(*list);
                    XFreeStringList(list);
                } else
                    name = text_prop.value ? (char *)text_prop.value : "";
					
            } else				
                name = text_prop.value ? (char *)text_prop.value : "";

            XFree(text_prop.value);

        } else { // default name
            name = i18n->getMessage(FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                    "Unnamed");
        }
    } else {
        // default name
        name = i18n->getMessage(FBNLS::WindowSet, FBNLS::WindowUnnamed,
                                "Unnamed");
    }

    return name;
}

}; // end namespace Xutil

