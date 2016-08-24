// Xutil.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Xutil.hh"
#include "Debug.hh"

#include "FbTk/I18n.hh"
#include "FbTk/App.hh"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <iostream>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;
using std::strlen;
using std::endl;


namespace Xutil {

FbTk::FbString getWMName(Window window) {

    if (window == None)
        return FbTk::FbString("");

    Display *display = FbTk::App::instance()->display();

    XTextProperty text_prop;
    text_prop.value = 0;
    char **list = 0;
    int num = 0;
    _FB_USES_NLS;
    FbTk::FbString name;

    if (XGetWMName(display, window, &text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {

                text_prop.nitems = strlen((char *) text_prop.value);
                XmbTextPropertyToTextList(display, &text_prop, &list, &num);

                if (num > 0 && list != 0)
                    name = FbTk::FbStringUtil::LocaleStrToFb(static_cast<char *>(*list));
                else
                    name = text_prop.value ? FbTk::FbStringUtil::XStrToFb((char *)text_prop.value) : "";

                if (list)
                    XFreeStringList(list);

            } else
                name = text_prop.value ? FbTk::FbStringUtil::XStrToFb((char *)text_prop.value) : "";

            XFree(text_prop.value);

        } else { // default name
            name = _FB_XTEXT(Window, Unnamed, "Unnamed", "Default name for a window without a WM_NAME");
        }
    } else {
        // default name
        name = _FB_XTEXT(Window, Unnamed, "Unnamed", "Default name for a window without a WM_NAME");
    }

    return name;
}


// The name of this particular instance
FbTk::FbString getWMClassName(Window win) {

    XClassHint ch;
    FbTk::FbString instance_name;

    if (XGetClassHint(FbTk::App::instance()->display(), win, &ch) == 0) {
        fbdbg<<"Xutil: Failed to read class hint!"<<endl;
    } else {

        XFree(ch.res_class);

        if (ch.res_name != 0) {
            instance_name = const_cast<char *>(ch.res_name);
            XFree(ch.res_name);
            ch.res_name = 0;
        }
    }

    return instance_name;
}

// the name of the general class of the app
FbTk::FbString getWMClassClass(Window win) {

    XClassHint ch;
    FbTk::FbString class_name;

    if (XGetClassHint(FbTk::App::instance()->display(), win, &ch) == 0) {
        fbdbg<<"Xutil: Failed to read class hint!"<<endl;
    } else {

        XFree(ch.res_name);

        if (ch.res_class != 0) {
            class_name = const_cast<char *>(ch.res_class);
            XFree(ch.res_class);
            ch.res_class = 0;
        }
    }

    return class_name;
}

} // end namespace Xutil

