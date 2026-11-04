// RootTheme.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "RootTheme.hh"

#include "defaults.hh"
#include "FbRootWindow.hh"
#include "FbCommands.hh"
#include "Screen.hh"

#include "FbTk/App.hh"
#include "FbTk/Font.hh"
#include "FbTk/Image.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/Resource.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/TextureRender.hh"
#include "FbTk/I18n.hh"

#include <X11/Xatom.h>
#include <iostream>
#include <vector>

#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;

namespace {

// fbsetroot -gradient argument, rebuilt from the parsed texture so no style
// text reaches fbsetbg. one word: fbsetbg word-splits what it forwards.
string gradientSpec(unsigned long type) {
    string spec;
    if (type & FbTk::Texture::CROSSDIAGONAL)
        spec = "crossdiagonal";
    else if (type & FbTk::Texture::RECTANGLE)
        spec = "rectangle";
    else if (type & FbTk::Texture::PYRAMID)
        spec = "pyramid";
    else if (type & FbTk::Texture::PIPECROSS)
        spec = "pipecross";
    else if (type & FbTk::Texture::ELLIPTIC)
        spec = "elliptic";
    else if (type & FbTk::Texture::HORIZONTAL)
        spec = "horizontal";
    else if (type & FbTk::Texture::VERTICAL)
        spec = "vertical";
    else
        spec = "diagonal";

    if (type & FbTk::Texture::RAISED)
        spec += "raised";
    else if (type & FbTk::Texture::SUNKEN)
        spec += "sunken";
    else
        spec += "flat";

    if (type & FbTk::Texture::BEVEL2)
        spec += "bevel2";
    if (type & FbTk::Texture::INVERT)
        spec += "invert";
    if (type & FbTk::Texture::INTERLACED)
        spec += "interlaced";

    return spec;
}

int ignoreXErrors(Display *, XErrorEvent *) {
    return 0;
}

Pixmap rootPixmapProperty(Display *disp, Window root, Atom atom) {
    Atom type;
    int format;
    unsigned long items, after;
    unsigned char *data = 0;
    Pixmap pm = None;

    if (XGetWindowProperty(disp, root, atom, 0L, 1L, False, AnyPropertyType,
                           &type, &format, &items, &after, &data) == Success &&
        data != 0) {
        if (type == XA_PIXMAP && format == 32 && items == 1)
            pm = *reinterpret_cast<Pixmap *>(data);
        XFree(data);
    }
    return pm;
}

// Publish 'src' as the root background the way Esetroot and fbsetroot do,
// so pseudo-transparent clients (fluxbox's own alpha included) find it via
// _XROOTPMAP_ID. The published pixmap lives on a short-lived connection
// closed with RetainPermanent: the next setter XKillClient()s the owner of
// a pixmap both atoms point to, and that must never be fluxbox itself.
bool publishRootPixmap(const char *display_name, int screen, Pixmap src,
                       unsigned int width, unsigned int height,
                       unsigned int depth) {
    Display *disp = XOpenDisplay(display_name);
    if (disp == 0)
        return false;

    Window root = RootWindow(disp, screen);
    Atom atom_root = XInternAtom(disp, "_XROOTPMAP_ID", False);
    Atom atom_eroot = XInternAtom(disp, "ESETROOT_PMAP_ID", False);

    // free what the previous setter left behind. the id may be stale,
    // so do not let the resulting BadValue reach fluxbox's error log.
    Pixmap old_root = rootPixmapProperty(disp, root, atom_root);
    if (old_root != None &&
        old_root == rootPixmapProperty(disp, root, atom_eroot)) {
        XErrorHandler old_handler = XSetErrorHandler(ignoreXErrors);
        XKillClient(disp, old_root);
        XSync(disp, False);
        XSetErrorHandler(old_handler);
    }

    Pixmap pm = XCreatePixmap(disp, root, width, height, depth);
    GC gc = XCreateGC(disp, pm, 0, 0);
    XCopyArea(disp, src, pm, gc, 0, 0, width, height, 0, 0);
    XFreeGC(disp, gc);

    XSetWindowBackgroundPixmap(disp, root, pm);
    XClearWindow(disp, root);
    XChangeProperty(disp, root, atom_root, XA_PIXMAP, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(&pm), 1);
    XChangeProperty(disp, root, atom_eroot, XA_PIXMAP, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(&pm), 1);

    XSetCloseDownMode(disp, RetainPermanent);
    XCloseDisplay(disp);
    return true;
}

}

class BackgroundItem: public FbTk::ThemeItem<FbTk::Texture> {
public:
    BackgroundItem(FbTk::Theme &tm, const std::string &name, const std::string &altname):
        FbTk::ThemeItem<FbTk::Texture>(tm, name, altname),
        m_changed(false), m_loaded(false) {

    }

    void load(const std::string *o_name = 0, const std::string *o_altname = 0) {
        const string &m_name = (o_name == 0) ? name() : *o_name;
        const string &m_altname = (o_altname == 0) ? altName() : *o_altname;

        // if we got this far, then the background was loaded
        m_loaded = true;

        // create subnames
        string color_name(FbTk::ThemeManager::instance().
                          resourceValue(m_name + ".color", m_altname + ".Color"));
        string colorto_name(FbTk::ThemeManager::instance().
                            resourceValue(m_name + ".colorTo", m_altname + ".ColorTo"));
        string pixmap_name(FbTk::ThemeManager::instance().
                           resourceValue(m_name + ".pixmap", m_altname + ".Pixmap"));
        string mod_x(FbTk::ThemeManager::instance().
                     resourceValue(m_name + ".modX", m_altname + ".ModX"));
        string mod_y(FbTk::ThemeManager::instance().
                     resourceValue(m_name + ".modY", m_altname + ".ModY"));

        // validate mod_x and mod_y
        if (mod_x.length() > 2)
            mod_x.erase(2,mod_x.length()); // shouldn't be longer than 2 digits
        if (mod_y.length() > 2)
            mod_y.erase(2,mod_y.length()); // ditto
        // should be integers
        if (!mod_x.length() || mod_x[0] < '0' || mod_x[0] > '9' ||
            (mod_x.length() == 2 && (mod_x[1] < '0' || mod_x[1] > '9')))
            mod_x = "1";
        if (!mod_y.length() || mod_y[0] < '0' || mod_y[0] > '9' ||
            (mod_y.length() == 2 && (mod_y[1] < '0' || mod_y[1] > '9')))
            mod_y = "1";

        // remove whitespace from filename
        FbTk::StringUtil::removeFirstWhitespace(pixmap_name);
        FbTk::StringUtil::removeTrailingWhitespace(pixmap_name);

        // check if the background has been changed
        if (mod_x != m_mod_x || mod_y != m_mod_y || pixmap_name != m_filename ||
                color_name != m_color || colorto_name != m_color_to) {
            m_changed = true;
            m_mod_x = mod_x;
            m_mod_y = mod_y;
            m_filename = pixmap_name;

            // these aren't quite right because of defaults set below
            m_color = color_name;
            m_color_to = colorto_name;
        }

        // set default value if we failed to load colors
        if (!(*this)->color().setFromString(color_name.c_str(),
                                            theme().screenNum()))
            (*this)->color().setFromString("darkgray", theme().screenNum());

        if (!(*this)->colorTo().setFromString(colorto_name.c_str(),
                                              theme().screenNum()))
            (*this)->colorTo().setFromString("white", theme().screenNum());


        if (((*this)->type() & FbTk::Texture::SOLID) != 0 && ((*this)->type() & FbTk::Texture::FLAT) == 0)
            (*this)->calcHiLoColors(theme().screenNum());

        // we dont load any pixmap, using external command to set background pixmap
        (*this)->pixmap() = 0;
    }

    void setFromString(const char *str) {
        m_options = str; // save option string
        FbTk::ThemeItem<FbTk::Texture>::setFromString(str);
    }
    const std::string &filename() const { return m_filename; }
    const std::string &options() const { return m_options; }
    const std::string &colorString() const { return m_color; }
    const std::string &colorToString() const { return m_color_to; }
    const std::string &modX() const { return m_mod_x; }
    const std::string &modY() const { return m_mod_y; }
    bool changed() const { return m_changed; }
    bool loaded() const { return m_loaded; }
    void setApplied() { m_changed = false; }
    void unsetLoaded() { m_loaded = false; }
private:
    std::string m_filename, m_options;
    std::string m_color, m_color_to;
    std::string m_mod_x, m_mod_y;
    bool m_changed, m_loaded;
};


RootTheme::RootTheme(FbTk::ImageControl &image_control):
    FbTk::Theme(image_control.screenNumber()),
    m_image_control(image_control),
    m_background(new BackgroundItem(*this, "background", "Background")),
    m_opgc(RootWindow(FbTk::App::instance()->display(), image_control.screenNumber())),
    m_first(true) {

    Display *disp = FbTk::App::instance()->display();
    m_opgc.setForeground(WhitePixel(disp, screenNum())^BlackPixel(disp, screenNum()));
    m_opgc.setFunction(GXxor);
    m_opgc.setSubwindowMode(IncludeInferiors);
    FbTk::ThemeManager::instance().loadTheme(*this);
}

RootTheme::~RootTheme() {
    delete m_background;
}

bool RootTheme::fallback(FbTk::ThemeItem_base &item) {
    // if background theme item was not found in the
    // style then mark background as not loaded so
    // we can deal with it in reconfigureTheme()
    if (item.name() == "background") {
        // mark no background loaded
        m_background->unsetLoaded();
        return true;
    }
    return false;
}

void RootTheme::reconfigTheme() {
    if (!m_background->loaded())
        return;

    if (!m_first && !m_background->changed())
        return;

    //
    // Else parse background from style
    //

    m_background->setApplied();

    // handle background option in style
    std::string filename = m_background->filename();
    FbTk::StringUtil::removeTrailingWhitespace(filename);
    FbTk::StringUtil::removeFirstWhitespace(filename);

    // if background argument is a file then
    // parse image options and call image setting
    // command specified in the resources
    std::string img_path = FbTk::Image::locateFile(filename);
    filename = FbTk::StringUtil::expandFilename(filename);
    // style values go in as plain argv entries; no shell parses them
    std::vector<std::string> args;
    args.push_back(realProgramName("fbsetbg"));
    args.push_back(m_first ? "-z" : "-Z");

    // user explicitly requests NO background be set at all
    if (strstr(m_background->options().c_str(), "unset") != 0) {
        return;
    }
    // style doesn't wish to change the background
    if (strstr(m_background->options().c_str(), "none") != 0) {
        if (!m_first)
            return;
    } else if (!img_path.empty()) {
        // parse options
        if (strstr(m_background->options().c_str(), "tiled") != 0)
            args.push_back("-t");
        else if (strstr(m_background->options().c_str(), "centered") != 0)
            args.push_back("-c");
        else if (strstr(m_background->options().c_str(), "aspect") != 0)
            args.push_back("-a");
        else
            args.push_back("-f");

        args.push_back(img_path);
    } else if (FbTk::FileUtil::isDirectory(filename.c_str()) &&
               strstr(m_background->options().c_str(), "random") != 0) {
        args.push_back("-r");
        args.push_back(filename);
    } else if (!FbTk::FileUtil::isInPath(realProgramName("fbsetbg")) ||
               !FbTk::FileUtil::isInPath(realProgramName("fbsetroot"))) {
        m_first = false;
        renderBackground();
        return;
    } else {
        // render normal texture with fbsetroot
        args.push_back("-b");

        bool color_valid =
            FbTk::Color::validColorString(m_background->colorString().c_str(),
                                          screenNum());
        bool color_to_valid =
            FbTk::Color::validColorString(m_background->colorToString().c_str(),
                                          screenNum());

        if (color_valid) {
            args.push_back("-foreground");
            args.push_back(m_background->colorString());
        }
        if (color_to_valid) {
            args.push_back("-background");
            args.push_back(m_background->colorToString());
        }

        if (strstr(m_background->options().c_str(), "mod") != 0) {
            args.push_back("-mod");
            args.push_back(m_background->modX());
            args.push_back(m_background->modY());
        } else if ((*m_background)->type() & FbTk::Texture::SOLID && color_valid) {
            args.push_back("-solid");
            args.push_back(m_background->colorString());
        } else if ((*m_background)->type() & FbTk::Texture::GRADIENT) {
            args.push_back("-gradient");
            args.push_back(gradientSpec((*m_background)->type()));
        }
    }

    m_first = false;
    FbCommands::spawn(args, screenNum());
}

void RootTheme::renderBackground() {
    // plain Xlib: an FbRootWindow would remove the screen's event handler
    // for the root window when it goes out of scope
    Display *disp = FbTk::App::instance()->display();
    Window root = RootWindow(disp, screenNum());
    Pixmap pixmap = m_image_control.renderImage(DisplayWidth(disp, screenNum()),
                                                DisplayHeight(disp, screenNum()),
                                                **m_background,
                                                FbTk::ROT0, false);
    if (pixmap == None)
        return;

    Window pm_root;
    int x, y;
    unsigned int width, height, border, depth;
    // a round trip: 'pixmap' exists server-side before the second
    // connection copies from it
    XGetGeometry(disp, pixmap, &pm_root, &x, &y, &width, &height, &border, &depth);

    if (!publishRootPixmap(DisplayString(disp), screenNum(), pixmap,
                           width, height, depth)) {
        // no second connection: at least show the background. the root
        // window keeps it after the pixmap is freed, but _XROOTPMAP_ID
        // must not point to a pixmap owned by (or freed by) fluxbox.
        XSetWindowBackgroundPixmap(disp, root, pixmap);
        XClearWindow(disp, root);
    }
    XFreePixmap(disp, pixmap);
}
