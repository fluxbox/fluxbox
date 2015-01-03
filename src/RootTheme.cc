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
    std::string cmd = realProgramName("fbsetbg") + (m_first ? " -z " : " -Z ");

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
            cmd += "-t ";
        else if (strstr(m_background->options().c_str(), "centered") != 0)
            cmd += "-c ";
        else if (strstr(m_background->options().c_str(), "aspect") != 0)
            cmd += "-a ";
        else
            cmd += "-f ";

        cmd += img_path;
    } else if (FbTk::FileUtil::isDirectory(filename.c_str()) &&
               strstr(m_background->options().c_str(), "random") != 0) {
        cmd += "-r " + filename;
    } else {
        // render normal texture with fbsetroot
        cmd += "-b ";

        // Make sure the color strings are valid,
        // so we dont pass any `commands` that can be executed
        bool color_valid =
            FbTk::Color::validColorString(m_background->colorString().c_str(),
                                          screenNum());
        bool color_to_valid =
            FbTk::Color::validColorString(m_background->colorToString().c_str(),
                                          screenNum());

        std::string options;
        if (color_valid)
            cmd += "-foreground '" + m_background->colorString() + "' ";
        if (color_to_valid)
            cmd += "-background '" + m_background->colorToString() + "' ";

        if (strstr(m_background->options().c_str(), "mod") != 0)
            cmd += "-mod " + m_background->modX() + " " + m_background->modY();
        else if ((*m_background)->type() & FbTk::Texture::SOLID && color_valid)
            cmd += "-solid '" + m_background->colorString() + "' ";
        else if ((*m_background)->type() & FbTk::Texture::GRADIENT) {
            // remove whitespace from the options, since fbsetroot doesn't care
            // and dealing with sh and fbsetbg is impossible if we don't
            std::string options = m_background->options();
            options = FbTk::StringUtil::replaceString(options, " ", "");
            options = FbTk::StringUtil::replaceString(options, "\t", "");
            cmd += "-gradient " + options;
        }
    }

    // call command with options
    FbCommands::ExecuteCmd exec(cmd, screenNum());
    m_first = false;
    exec.execute();
}
