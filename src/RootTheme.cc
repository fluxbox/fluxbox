// RootTheme.cc
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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


#include <X11/Xatom.h>

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
private:
    std::string m_filename, m_options;
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
    // then generate an image with a text that 
    // notifies the user about it
        
    if (!m_background_loaded) {
        FbTk::FbPixmap root(FbTk::FbPixmap::getRootPixmap(screenNum()));
        // if there is no root background pixmap
        // then we need to create one
        if (root.drawable() == None) {
            root.create(rootwin.window(),
                        rootwin.width(), rootwin.height(),
                        rootwin.depth());

            FbTk::FbPixmap::setRootPixmap(screenNum(), root.drawable());
        }

        // setup root window property
        Atom atom_root = XInternAtom(rootwin.display(), "_XROOTPMAP_ID", false);
        Pixmap pm = root.drawable();
        rootwin.changeProperty(atom_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pm, 1);
        rootwin.setBackgroundPixmap(root.drawable());


        FbTk::GContext gc(root);

        // fill background color
        gc.setForeground(FbTk::Color("black", screenNum()));        
        root.fillRectangle(gc.gc(),
                           0, 0,
                           root.width(), root.height());
        // text color
        gc.setForeground(FbTk::Color("white", screenNum()));
        // render text
        const char errormsg[] = 
            "There is no background option specified in this style. Please consult the manual or read the FAQ.";
        FbTk::Font font;
        font.drawText(root, screenNum(), gc.gc(),
                      errormsg, strlen(errormsg), 
                      2, font.height() + 2); // added some extra pixels for better visibility

       
        // reset background mark
        m_background_loaded = true;
        root.release(); // we dont want to destroy this pixmap
    } else {
        // handle background option in style
        std::string filename = m_background->filename();
        FbTk::StringUtil::removeTrailingWhitespace(filename);
        FbTk::StringUtil::removeFirstWhitespace(filename);
        // if background argument is a file then
        // parse image options and call image setting 
        // command specified in the resources
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
            // render normal texture
            
            // we override the image control renderImage since 
            // since we do not want to cache this pixmap
            XColor *colors;
            int num_colors;
            m_image_ctrl.getXColorTable(&colors, &num_colors);
            FbTk::TextureRender image(m_image_ctrl, rootwin.width(), rootwin.height(), 
                                      colors, num_colors);
            Pixmap pixmap = image.render(*(*m_background));
            // setup root window property
            Atom atom_root = XInternAtom(rootwin.display(), "_XROOTPMAP_ID", false);
            rootwin.changeProperty(atom_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pixmap, 1);
            rootwin.setBackgroundPixmap(pixmap);

        }

    }

    // clear root window
    rootwin.clear();


        
}
