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

// $Id$

#include "RootTheme.hh"

#include "FbRootWindow.hh"
#include "FbCommands.hh"

#include "FbTk/App.hh"
#include "FbTk/Font.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/Resource.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/TextureRender.hh"
#include "FbTk/I18n.hh"

#include <X11/Xatom.h>
#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>

using std::cerr;
using std::endl;
using std::string;

class BackgroundItem: public FbTk::ThemeItem<FbTk::Texture> {
public:
    BackgroundItem(FbTk::Theme &tm, const std::string &name, const std::string &altname):
        FbTk::ThemeItem<FbTk::Texture>(tm, name, altname) {
        
    }

    void load(const std::string *o_name = 0, const std::string *o_altname = 0) {
        const string &m_name = (o_name == 0) ? name() : *o_name;
        const string &m_altname = (o_altname == 0) ? altName() : *o_altname;

        // create subnames
        string color_name(FbTk::ThemeManager::instance().
                          resourceValue(m_name + ".color", m_altname + ".Color"));
        string colorto_name(FbTk::ThemeManager::instance().
                            resourceValue(m_name + ".colorTo", m_altname + ".ColorTo"));
        string pixmap_name(FbTk::ThemeManager::instance().
                           resourceValue(m_name + ".pixmap", m_altname + ".Pixmap"));

        m_color = color_name;
        m_color_to = colorto_name;
        // set default value if we failed to load colors
        if (!(*this)->color().setFromString(color_name.c_str(),
                                            theme().screenNum()))
            (*this)->color().setFromString("darkgray", theme().screenNum());

        if (!(*this)->colorTo().setFromString(colorto_name.c_str(),
                                              theme().screenNum()))
            (*this)->colorTo().setFromString("white", theme().screenNum());


        if (((*this)->type() & FbTk::Texture::SOLID) != 0 && ((*this)->type() & FbTk::Texture::FLAT) == 0)
            (*this)->calcHiLoColors(theme().screenNum());

        // remove whitespace and set filename
        FbTk::StringUtil::removeFirstWhitespace(pixmap_name);
        FbTk::StringUtil::removeTrailingWhitespace(pixmap_name);
        m_filename = pixmap_name;

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
private:
    std::string m_filename, m_options;
    std::string m_color, m_color_to;
};


RootTheme::RootTheme(const std::string &root_command,
                     FbTk::ImageControl &image_control):
    FbTk::Theme(image_control.screenNumber()),
    m_background(new BackgroundItem(*this, "background", "Background")),
    m_opgc(RootWindow(FbTk::App::instance()->display(), image_control.screenNumber())),
    m_root_command(root_command),
    m_image_ctrl(image_control),
    m_lock(false),
    m_background_loaded(true) {

    Display *disp = FbTk::App::instance()->display();
    m_opgc.setForeground(WhitePixel(disp, screenNum())^BlackPixel(disp, screenNum()));
    m_opgc.setFunction(GXxor);
    m_opgc.setSubwindowMode(IncludeInferiors);
    m_opgc.setLineAttributes(1, LineSolid, CapNotLast, JoinMiter);
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
        m_background_loaded = false;
        return true;
    }
    return false;
}

void RootTheme::reconfigTheme() {
    _FB_USES_NLS;

    if (m_lock)
        return;

    // if user specified background in the config then use it
    // instead of style background
    if (!m_root_command.empty()) {
        FbCommands::ExecuteCmd cmd(m_root_command, screenNum());
        cmd.execute();
        return;
    }

    //
    // Else parse background from style 
    //

    // root window helper
    FbRootWindow rootwin(screenNum());

    // if the background theme item was not loaded
        
    if (!m_background_loaded) {
        const char *warning_msg = 
            _FBTEXT(Common, BackgroundWarning,
                    "There is no background option specified in this style."
                    " Please consult the manual or read the FAQ.",
                    "Background missing warning");

        cerr<<"Fluxbox: "<<warning_msg<<endl;
        
    } else {
        // handle background option in style
        std::string filename = m_background->filename();
        FbTk::StringUtil::removeTrailingWhitespace(filename);
        FbTk::StringUtil::removeFirstWhitespace(filename);
        // if background argument is a file then
        // parse image options and call image setting 
        // command specified in the resources
        filename = FbTk::StringUtil::expandFilename(filename);
        if (FbTk::FileUtil::isRegularFile(filename.c_str())) {
            // parse options
            std::string options;
            if (strstr(m_background->options().c_str(), "tiled") != 0)
                options += "-t ";
            if (strstr(m_background->options().c_str(), "centered") != 0)
                options += "-c ";
            if (strstr(m_background->options().c_str(), "random") != 0)
                options += "-r ";
            if (strstr(m_background->options().c_str(), "aspect") != 0)
                options += "-a ";
            
            // compose wallpaper application "fbsetbg" with argumetns
            std::string commandargs = "fbsetbg " + options + " " + filename;

            // call command with options
            FbCommands::ExecuteCmd exec(commandargs, screenNum());
            exec.execute();

        } else {
            // render normal texture with fbsetroot


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
                options += "-foreground '" + m_background->colorString() + "' ";
            if (color_to_valid)
                options += "-background '" + m_background->colorToString() + "' ";

            if ((*m_background)->type() & FbTk::Texture::SOLID && color_valid)
                options += "-solid '" + m_background->colorString() + "' ";

            if ((*m_background)->type() & FbTk::Texture::GRADIENT) {

                if (color_valid)
                    options += "-from '" + m_background->colorString() + "' ";
                if (color_to_valid)
                    options += "-to '" + m_background->colorToString() + "' ";

                options += "-gradient '" + m_background->options() + "'";
            }

            std::string commandargs = "fbsetroot " + options;

            FbCommands::ExecuteCmd exec(commandargs, screenNum());
            exec.execute();
        }

        rootwin.clear();
    }

}
