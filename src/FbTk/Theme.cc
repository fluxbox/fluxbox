// Theme.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Theme.cc,v 1.25 2004/01/12 20:24:06 fluxgen Exp $

#include "Theme.hh"

#include "XrmDatabaseHelper.hh"
#include "App.hh"
#include "StringUtil.hh"
#include "ThemeItems.hh"
#include "Directory.hh"

#include <cstdio>
#include <memory>
#include <iostream>

using namespace std;

namespace FbTk {

Theme::Theme(int screen_num):m_screen_num(screen_num) {
    ThemeManager::instance().registerTheme(*this);
}

Theme::~Theme() {
    ThemeManager::instance().unregisterTheme(*this);
}

ThemeManager &ThemeManager::instance() {
    static ThemeManager tm;
    return tm;
}

ThemeManager::ThemeManager():
    // max_screens: we initialize this later so we can set m_verbose 
    // without having a display connection
    m_max_screens(-1), 
    m_verbose(false),
    m_themelocation("") {

}

bool ThemeManager::registerTheme(Theme &tm) {
    if (m_max_screens < 0)
        m_max_screens = ScreenCount(FbTk::App::instance()->display());

    // valid screen num?
    if (m_max_screens < tm.screenNum() || tm.screenNum() < 0)
        return false;
    // TODO: use find and return false if it's already there
    // instead of unique 
    m_themelist.push_back(&tm);
    m_themelist.unique(); 
    return true;
}

bool ThemeManager::unregisterTheme(Theme &tm) {
    m_themelist.remove(&tm);
    return true;
}

bool ThemeManager::load(const std::string &filename, int screen_num) {
    std::string location = FbTk::StringUtil::expandFilename(filename);
    std::string prefix = "";

    if (Directory::isDirectory(filename)) {
        prefix = location;

        location.append("/theme.cfg");
        if (!Directory::isRegularFile(location)) {
            cerr<<"Error loading theme file "<<location<<": not a regular file"<<endl;
            return false;
        }
    } else {
        // dirname
        prefix = location.substr(0, location.find_last_of('/'));
    }

    if (!m_database.load(location.c_str()))
        return false;

    // relies on the fact that load_rc clears search paths each time
    if (m_themelocation != "") {
        Image::removeSearchPath(m_themelocation);
        m_themelocation.append("/pixmaps");
        Image::removeSearchPath(m_themelocation);
    }

    m_themelocation = prefix;

    location = prefix;
    Image::addSearchPath(location);
    location.append("/pixmaps");
    Image::addSearchPath(location);

    // get list and go throu all the resources and load them
    ThemeList::iterator theme_it = m_themelist.begin();
    const ThemeList::iterator theme_it_end = m_themelist.end();
    for (; theme_it != theme_it_end; ++theme_it) {
        if (screen_num < 0)
            loadTheme(**theme_it);
        else if (screen_num == (*theme_it)->screenNum()) // specified screen
            loadTheme(**theme_it);
            
    }
    // notify all themes that we reconfigured
    theme_it = m_themelist.begin();
    for (; theme_it != theme_it_end; ++theme_it) {
        // send reconfiguration signal to theme and listeners
        (*theme_it)->reconfigTheme();
        (*theme_it)->reconfigSig().notify();
    }
    return true;
}

void ThemeManager::loadTheme(Theme &tm) {
    std::list<ThemeItem_base *>::iterator i = tm.itemList().begin();
    std::list<ThemeItem_base *>::iterator i_end = tm.itemList().end();
    for (; i != i_end; ++i) {
        ThemeItem_base *resource = *i;
        if (!loadItem(*resource)) {
            // try fallback resource in theme
            if (!tm.fallback(*resource)) {
                if (verbose())
                    cerr<<"Failed to read theme item: "<<resource->name()<<endl;
                resource->setDefaultValue();                
            }
        }
    }
    // send reconfiguration signal to theme and listeners
}

bool ThemeManager::loadItem(ThemeItem_base &resource) {
    return loadItem(resource, resource.name(), resource.altName());
}

/// handles resource item loading with specific name/altname
bool ThemeManager::loadItem(ThemeItem_base &resource, const std::string &name, const std::string &alt_name) {
    XrmValue value;
    char *value_type;

    if (XrmGetResource(*m_database, name.c_str(),
                       alt_name.c_str(), &value_type, &value)) {
        resource.setFromString(value.addr);
        resource.load(); // load additional stuff by the ThemeItem
    } else
        return false;

    return true;
}

std::string ThemeManager::resourceValue(const std::string &name, const std::string &altname) {
    XrmValue value;
    char *value_type;
	
    if (*m_database != 0 && XrmGetResource(*m_database, name.c_str(),
                                           altname.c_str(), &value_type, &value) && value.addr != 0) {
        return string(value.addr);
    }
    return "";
}

/*
void ThemeManager::listItems() {
    ThemeList::iterator it = m_themelist.begin();
    ThemeList::iterator it_end = m_themelist.end();
    for (; it != it_end; ++it) {
        std::list<ThemeItem_base *>::iterator item = (*it)->itemList().begin();
        std::list<ThemeItem_base *>::iterator item_end = (*it)->itemList().end();
        for (; item != item_end; ++item) {
            
            if (typeid(**item) == typeid(ThemeItem<Texture>)) {
                cerr<<(*item)->name()<<": <texture type>"<<endl;
                cerr<<(*item)->name()<<".pixmap:  <filename>"<<endl;
                cerr<<(*item)->name()<<".color:  <color>"<<endl;
                cerr<<(*item)->name()<<".colorTo: <color>"<<endl;
            } else if (typeid(**item) == typeid(ThemeItem<Color>)) {
                cerr<<(*item)->name()<<": <color>"<<endl;
            } else if (typeid(**item) == typeid(ThemeItem<int>)) {
                cerr<<(*item)->name()<<": <integer>"<<endl;
            } else if (typeid(**item) == typeid(ThemeItem<bool>)) {
                cerr<<(*item)->name()<<": <boolean>"<<endl;
            } else if (typeid(**item) == typeid(ThemeItem<PixmapWithMask>)) {
                cerr<<(*item)->name()<<": <filename>"<<endl;
            }  else if (typeid(**item) == typeid(ThemeItem<std::string>)) {
                cerr<<(*item)->name()<<": <string>"<<endl;
            } else if (typeid(**item) == typeid(ThemeItem<Font>)) {
                cerr<<(*item)->name()<<": <font>"<<endl;
            } else {
                cerr<<(*item)->name()<<":"<<endl;
            }
        }
    }
             
}
*/
}; // end namespace FbTk
