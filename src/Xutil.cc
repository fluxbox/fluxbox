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

// $Id$

#include "Xutil.hh"

#include "FbTk/I18n.hh"
#include "FbTk/App.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <iostream>
using namespace std;

namespace Xutil {

std::string getWMName(Window window) {

    if (window == None)
        return "";

    Display *display = FbTk::App::instance()->display();

    XTextProperty text_prop;
    text_prop.value = 0;
    char **list;
    int num;
    _FB_USES_NLS;
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
            name = _FBTEXT(Window, Unnamed, "Unnamed", "Default name for a window without a WM_NAME");
        }
    } else {
        // default name
        name = _FBTEXT(Window, Unnamed, "Unnamed", "Default name for a window without a WM_NAME");
    }

    return name;
}


// The name of this particular instance
std::string getWMClassName(Window win) {
    XClassHint ch;
    std::string instance_name;

    if (XGetClassHint(FbTk::App::instance()->display(), win, &ch) == 0) {
#ifdef DEBUG
        cerr<<"Xutil: Failed to read class hint!"<<endl;
#endif //DEBUG
        instance_name = "";
    } else {        

        XFree(ch.res_class);
        
        if (ch.res_class != 0) {
            instance_name = const_cast<char *>(ch.res_name);
            XFree(ch.res_name);
            ch.res_name = 0;
        } else
            instance_name = "";
    }

    return instance_name;

}

// the name of the general class of the app
std::string getWMClassClass(Window win) {
    XClassHint ch;
    std::string class_name;

    if (XGetClassHint(FbTk::App::instance()->display(), win, &ch) == 0) {
#ifdef DEBUG
        cerr<<"Xutil: Failed to read class hint!"<<endl;
#endif //DEBUG
        class_name = "";
    } else {        

        XFree(ch.res_name);
        
        if (ch.res_class != 0) {
            class_name = const_cast<char *>(ch.res_class);
            XFree(ch.res_class);
            ch.res_class = 0;
        } else
            class_name = "";
    }

    return class_name;
}

}; // end namespace Xutil

