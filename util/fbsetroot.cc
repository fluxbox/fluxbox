// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
// Copyright (c) 1997 - 2000 Brad Hughes <bhughes at trolltech.com>
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

#include "fbsetroot.hh"

#include "../src/FbTk/I18n.hh"
#include "../src/FbTk/ImageControl.hh"
#include "../src/FbTk/Texture.hh"
#include "../src/FbTk/GContext.hh"
#include "../src/FbRootWindow.hh"

#include <X11/Xatom.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using std::string;

fbsetroot::fbsetroot(int argc, char **argv, char *dpy_name)
    : FbTk::App(dpy_name), m_app_name(argv[0]) {

    pixmap = (Pixmap *) 0;
    screen = DefaultScreen(FbTk::App::instance()->display());
    grad = fore = back = (char *) 0;

    bool mod = false, sol = false, grd = false;
    int mod_x = 0, mod_y = 0, i = 1;

    img_ctrl = new FbTk::ImageControl(screen);

    for (; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-help" || arg == "--help" || arg == "-h") {
            usage();

        } else if (arg == "-fg" || arg == "-foreground" ||
                   arg == "--foreground" || arg == "-from" || arg == "--from") {
            if ((++i) >= argc)
                usage(1);
            fore = argv[i];

        } else if (arg == "-bg" || arg == "-background" ||
                   arg == "--background" || arg == "-to" || arg == "--to") {
            if ((++i) >= argc)
                usage(1);
            back = argv[i];

        } else if (arg == "-solid" || arg == "--solid") {
            if ((++i) >= argc)
                usage(1);
            fore = argv[i];
            sol = true;

        } else if (arg == "-mod" || arg == "--mod") {
            if ((++i) >= argc)
                usage();
            mod_x = atoi(argv[i]);
            if ((++i) >= argc)
                usage();
            mod_y = atoi(argv[i]);
            if (mod_x < 1)
                mod_x = 1;
            if (mod_y < 1)
                mod_y = 1;
            mod = true;

        } else if (arg == "-gradient" || arg == "--gradient") {
            if ((++i) >= argc)
                usage();

            grad = argv[i];
            grd = true;

        } else if (arg == "-display" || arg == "--display") {
            // -display passed through tests earlier... we just skip it now
            i++;

        } else
            usage();
    }

    if ((mod + sol + grd) != true) {
        _FB_USES_NLS;
        cerr << _FB_CONSOLETEXT(fbsetroot, MustSpecify,
                    "Error: must specify one of: -solid, -mod, -gradient\n",
                    "user didn't give one of the required options") << endl;

        exit(2);
    }

    if (sol && fore)
        solid();
    else if (mod && mod_x && mod_y && fore && back)
        modula(mod_x, mod_y);
    else if (grd && grad && fore && back)
        gradient();
    else
        usage();

}


fbsetroot::~fbsetroot() {
    XKillClient(display(), AllTemporary);

    if (pixmap) { // should always be true
        XSetCloseDownMode(display(), RetainTemporary);

        delete pixmap;
    }

    delete img_ctrl;
}

/**
   set root pixmap atoms so that apps like
   Eterm and xchat will be able to use
   transparent background
*/
void fbsetroot::setRootAtoms(Pixmap pixmap, int screen) {
    Atom atom_root, atom_eroot, type;
    unsigned char *data_root, *data_eroot;
    int format;
    unsigned long length, after;

    atom_root = XInternAtom(display(), "_XROOTMAP_ID", true);
    atom_eroot = XInternAtom(display(), "ESETROOT_PMAP_ID", true);
    FbRootWindow root(screen);

    // doing this to clean up after old background
    if (atom_root != None && atom_eroot != None) {
        root.property(atom_root, 0L, 1L, false, AnyPropertyType,
                      &type, &format, &length, &after, &data_root);

        if (type == XA_PIXMAP) {
            root.property(atom_eroot, 0L, 1L, False, AnyPropertyType,
                          &type, &format, &length, &after, &data_eroot);

            if (data_root && data_eroot && type == XA_PIXMAP &&
                *((Pixmap *) data_root) == *((Pixmap *) data_eroot)) {

                XKillClient(display(), *((Pixmap *) data_root));
            }
        }
    }

    atom_root = XInternAtom(display(), "_XROOTPMAP_ID", false);
    atom_eroot = XInternAtom(display(), "ESETROOT_PMAP_ID", false);

    if (atom_root == None || atom_eroot == None) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(fbsetroot, NoPixmapAtoms, "Couldn't create pixmap atoms, giving up!", "Couldn't create atoms to point at root pixmap")<<endl;
        exit(1);
    }

    // setting new background atoms
    root.changeProperty(atom_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
    root.changeProperty(atom_eroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);

}

/**
   Draws pixmaps with a single color
*/
void fbsetroot::solid() {
    FbTk::Color c(fore, screen);
    if (! c.isAllocated())
        c.setPixel(BlackPixel(display(), screen));

    FbRootWindow root(screen);

    FbTk::GContext gc(root);
    gc.setForeground(c);

    pixmap = new Pixmap(XCreatePixmap(display(),
                                      root.window(),
                                      root.width(), root.height(),
                                      root.depth()));

    XFillRectangle(display(), *pixmap, gc.gc(), 0, 0,
                   root.width(), root.height());

    setRootAtoms(*pixmap, screen);

    root.setBackgroundPixmap(*pixmap);
    root.clear();
}

/**
 Draws pixmaps with an 16x16 pattern with
 fg and bg colors.
*/
void fbsetroot::modula(int x, int y) {

    const int s = 16;

    char data[32];
    long pattern = 0;

    int i;

    FbRootWindow root(screen);

    for (i = 0; i < s; i++) {
        pattern <<= 1;
        if ((i % x) == 0)
            pattern |= 0x0001;
    }

    for (i = 0; i < s; i++) {
        if ((i %  y) == 0) {
            data[(i * 2)] = (char) 0xff;
            data[(i * 2) + 1] = (char) 0xff;
        } else {
            data[(i * 2)] = pattern & 0xff;
            data[(i * 2) + 1] = (pattern >> 8) & 0xff;
        }
    }


    Pixmap bitmap, r_bitmap;


    bitmap = XCreateBitmapFromData(display(),
                                   root.window(), data, s, s);

    // bitmap used as tile, needs to have the same depth as background pixmap
    r_bitmap = XCreatePixmap(display(),
                             root.window(), s, s,
                             root.depth());

    FbTk::Color f(fore, screen), b(back, screen);

    if (! f.isAllocated())
        f.setPixel(WhitePixel(display(), screen));
    if (! b.isAllocated())
        b.setPixel(BlackPixel(display(), screen));

    FbTk::GContext gc(root);

    gc.setForeground(f);
    gc.setBackground(b);

    // copying bitmap to the one going to be used as tile
    XCopyPlane(display(), bitmap, r_bitmap, gc.gc(),
               0, 0, s, s, 0, 0, 1l);

    gc.setTile(r_bitmap);
    gc.setFillStyle(FillTiled);

    pixmap = new Pixmap(XCreatePixmap(display(),
                                      root.window(),
                                      root.width(), root.height(),
                                      root.depth()));

    XFillRectangle(display(), *pixmap, gc.gc(), 0, 0,
                   root.width(), root.height());

    setRootAtoms(*pixmap, screen);
    root.setBackgroundPixmap(*pixmap);
    root.clear();

    XFreePixmap(display(), bitmap);
    XFreePixmap(display(), r_bitmap);
}

/**
 draws pixmaps with a fluxbox texure
*/
void fbsetroot::gradient() {
    // using temporaray pixmap and then copying it to background pixmap, as it'll
    // get crashed somewhere on the way causing apps like XChat chrashing
    // as the pixmap has been destroyed
    Pixmap tmp;
    // we must insert gradient text
    string texture_value = grad ? grad : "solid";
    texture_value.insert(0, "gradient ");
    FbTk::Texture texture;
    texture.setFromString(texture_value.c_str());

    FbRootWindow root(screen);


    FbTk::GContext gc(root);
    texture.color().setFromString(fore, screen);
    texture.colorTo().setFromString(back, screen);


    if (! texture.color().isAllocated())
        texture.color().setPixel(WhitePixel(display(), screen));

    if (! texture.colorTo().isAllocated())
        texture.colorTo().setPixel(BlackPixel(display(), screen));

    tmp = img_ctrl->renderImage(root.width(), root.height(), texture);

    pixmap = new Pixmap(XCreatePixmap(display(),
                                      root.window(),
                                      root.width(), root.height(),
                                      root.depth()));


    XCopyArea(display(), tmp, *pixmap, gc.gc(), 0, 0,
              root.width(), root.height(),
              0, 0);

    setRootAtoms(*pixmap, screen);

    root.setBackgroundPixmap(*pixmap);
    root.clear();

    if (! (root.visual()->c_class & 1)) {
        img_ctrl->removeImage(tmp);
        img_ctrl->cleanCache();
    }

}

/**
 Shows information about usage
*/
void fbsetroot::usage(int exit_code) {
    _FB_USES_NLS;
    cout << m_app_name << " 2.3 : (c) 2003-2015 Fluxbox Development Team" << endl;
    cout << m_app_name << " 2.1 : (c) 2002 Claes Nasten" << endl;
    cout << m_app_name << " 2.0 : (c) 1997-2000 Brad Hughes\n" << endl;
    cout << _FB_CONSOLETEXT(fbsetroot, Usage,
                  "  -display <string>        display connection\n"
                  "  -mod <x> <y>             modula pattern\n"
                  "  -foreground, -fg <color> modula foreground color\n"
                  "  -background, -bg <color> modula background color\n\n"
                  "  -gradient <texture>      gradient texture\n"
                  "  -from <color>            gradient start color\n"
                  "  -to <color>              gradient end color\n\n"
                  "  -solid <color>           solid color\n\n"
                  "  -help                    print this help text and exit\n",
                  "fbsetroot usage options") << endl;
    exit(exit_code);
}


int main(int argc, char **argv) {
    char *display_name = (char *) 0;
    int i = 1;

    FbTk::I18n::init(0);

    for (; i < argc; i++) {
        if (!strcmp(argv[i], "-display") || !strcmp(argv[i], "--display")) {
            // check for -display option

            if ((++i) >= argc) {
                _FB_USES_NLS;
                cerr<<_FB_CONSOLETEXT(main, DISPLAYRequiresArg,
                              "error: '-display' requires an argument",
                              "option requires an argument")<<endl;

                ::exit(1);
            }

            display_name = argv[i];
        }
    }

    try {
        fbsetroot app(argc, argv, display_name);
    } catch (string & error_str) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Common, Error, "Error", "Error message header")<<": "<<error_str<<endl;
    }

    return (0);
}
