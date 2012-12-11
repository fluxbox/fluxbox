// Theme.cc for FbTk - Fluxbox ToolKit
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "Theme.hh"

#include "XrmDatabaseHelper.hh"
#include "App.hh"
#include "StringUtil.hh"
#include "FileUtil.hh"
#include "I18n.hh"
#include "Image.hh"
#include "STLUtil.hh"

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#include <memory>
#include <iostream>
#include <algorithm>

using std::cerr;
using std::endl;
using std::string;

namespace FbTk {

struct LoadThemeHelper {
    LoadThemeHelper():m_tm(ThemeManager::instance()) {}
    void operator ()(Theme *tm) {
        m_tm.loadTheme(*tm);
    }
    void operator ()(ThemeManager::ThemeList &tmlist) {

        STLUtil::forAll(tmlist, *this);
        // send reconfiguration signal to theme and listeners
        ThemeManager::ThemeList::iterator it = tmlist.begin();
        ThemeManager::ThemeList::iterator it_end = tmlist.end();
        for (; it != it_end; ++it) {
            (*it)->reconfigSig().emit();
        }
    }

    ThemeManager &m_tm;
};

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
    if (m_max_screens < 0) {
        m_max_screens = ScreenCount(FbTk::App::instance()->display());
        m_themes.resize(m_max_screens);
    }

    // valid screen num?
    if (m_max_screens < tm.screenNum() || tm.screenNum() < 0)
        return false;

    ThemeList::const_iterator it = m_themes[tm.screenNum()].begin(),
                              it_end = m_themes[tm.screenNum()].end();
    if (std::find(it, it_end, &tm) == it_end) {
        m_themes[tm.screenNum()].push_back(&tm);
        return true;
    }
    return false;
}

bool ThemeManager::unregisterTheme(Theme &tm) {
    if (m_max_screens < tm.screenNum() || tm.screenNum() < 0)
        return false;

    m_themes[tm.screenNum()].remove(&tm);

    return true;
}

bool ThemeManager::load(const string &filename,
                        const string &overlay_filename, int screen_num) {
    
    string location = FbTk::StringUtil::expandFilename(filename);
    StringUtil::removeTrailingWhitespace(location);
    StringUtil::removeFirstWhitespace(location);
    string prefix = "";

    if (FileUtil::isDirectory(location.c_str())) {
        prefix = location;

        location.append("/theme.cfg");
        if (!FileUtil::isRegularFile(location.c_str())) {
            location = prefix;
            location.append("/style.cfg");
            if (!FileUtil::isRegularFile(location.c_str())) {
                cerr<<"Error loading theme file "<<location<<": not a regular file"<<endl;
                return false;
            }
        }
    } else {
        // dirname
        prefix = location.substr(0, location.find_last_of('/'));
    }

    if (!m_database.load(location.c_str()))
        return false;


    if (!overlay_filename.empty()) {
        string overlay_location = FbTk::StringUtil::expandFilename(overlay_filename);
        if (FileUtil::isRegularFile(overlay_location.c_str())) {
            XrmDatabaseHelper overlay_db;
            if (overlay_db.load(overlay_location.c_str())) {
                // after a merge the src_db is destroyed
                // so, make sure XrmDatabaseHelper::m_database == 0
                XrmMergeDatabases(*overlay_db, &(*m_database));
                *overlay_db = 0;
            }
        }
    }

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

    LoadThemeHelper load_theme_helper;

    // get list and go throu all the resources and load them
    // and then reconfigure them
    if (screen_num < 0 || screen_num > m_max_screens) {
        STLUtil::forAll(m_themes, load_theme_helper);
    } else {
        load_theme_helper(m_themes[screen_num]);
    }

    return true;
}

void ThemeManager::loadTheme(Theme &tm) {
    Theme::ItemList::iterator i = tm.itemList().begin();
    Theme::ItemList::iterator i_end = tm.itemList().end();
    for (; i != i_end; ++i) {
        ThemeItem_base *resource = *i;
        if (!loadItem(*resource)) {
            // try fallback resource in theme
            if (!tm.fallback(*resource)) {
                if (verbose()) {
                    _FB_USES_NLS;
                    cerr<<_FBTK_CONSOLETEXT(Error, ThemeItem, "Failed to read theme item", "When reading a style, couldn't read a specific item (following)")<<": "<<resource->name()<<endl;
                }
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
bool ThemeManager::loadItem(ThemeItem_base &resource, const string &name, const string &alt_name) {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(*m_database, name.c_str(),
                       alt_name.c_str(), &value_type, &value)) {
        resource.setFromString(value.addr);
        resource.load(&name, &alt_name); // load additional stuff by the ThemeItem
    } else
        return false;

    return true;
}

string ThemeManager::resourceValue(const string &name, const string &altname) {
    XrmValue value;
    char *value_type;
    if (*m_database != 0 && XrmGetResource(*m_database, name.c_str(),
                                           altname.c_str(), &value_type, &value) && value.addr != 0)
        return string(value.addr);

    return "";
}

/*
void ThemeManager::listItems() {
    ThemeList::iterator it = m_themelist.begin();
    ThemeList::iterator it_end = m_themelist.end();
    for (; it != it_end; ++it) {
        list<ThemeItem_base *>::iterator item = (*it)->itemList().begin();
        list<ThemeItem_base *>::iterator item_end = (*it)->itemList().end();
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
            }  else if (typeid(**item) == typeid(ThemeItem<string>)) {
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
} // end namespace FbTk
