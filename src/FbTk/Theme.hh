// Theme.hh for FbTk - Fluxbox ToolKit
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

/**
 @file Theme.hh holds ThemeItem<T>, Theme and ThemeManager which is the base for any theme
*/

#ifndef FBTK_THEME_HH
#define FBTK_THEME_HH

#include "Signal.hh"
#include "XrmDatabaseHelper.hh"

#include <string>
#include <list>
#include <vector>

namespace FbTk {

class Theme;

/// Base class for ThemeItem, holds name and altname
/**
  @see ThemeItem
*/
class ThemeItem_base {
public:
    ThemeItem_base(const std::string &name, const std::string &altname):
        m_name(name), m_altname(altname) { }
    virtual ~ThemeItem_base() { }
    virtual void setFromString(const char *str) = 0;
    virtual void setDefaultValue() = 0;
    virtual void load(const std::string *name = 0, const std::string *altname = 0) = 0; // if it needs to load additional stuff
    const std::string &name() const { return m_name; }
    const std::string &altName() const { return m_altname; }
private:
    std::string m_name, m_altname;
};


/// template ThemeItem class for basic theme items
/// to use this you need to specialize setDefaultValue, setFromString and load
template <typename T>
class ThemeItem:public ThemeItem_base {
public:
    ThemeItem(FbTk::Theme &tm, const std::string &name, const std::string &altname);
    virtual ~ThemeItem();
    /// specialized
    void setDefaultValue();
    /// specialized
    virtual void setFromString(const char *strval);
    /// specialized
    // name and altname may be different to the primary ones (e.g. from fallback)
    // if they are null, then the original name is used
    virtual void load(const std::string *name = 0, const std::string *altname = 0);
    /**
       @name access operators
    */
    /**@{*/
    T& operator*() { return m_value; }
    const T& operator*() const { return m_value; }
    T *operator->() { return &m_value; }
    const T *operator->() const { return &m_value; }
    /**@}*/

    FbTk::Theme &theme() { return m_tm; }
private:

    T m_value;
    FbTk::Theme &m_tm;
};


/// Hold ThemeItems. Use this to create a Theme set
class Theme {
public:
    typedef std::list<ThemeItem_base *> ItemList;

    explicit Theme(int screen_num); // create a theme for a specific screen
    virtual ~Theme();
    virtual void reconfigTheme() = 0;
    int screenNum() const { return m_screen_num; }
    std::list<ThemeItem_base *> &itemList() { return m_themeitems; }
    const std::list<ThemeItem_base *> &itemList() const { return m_themeitems; }
    /// add ThemeItem
    template <typename T>
    void add(ThemeItem<T> &item);
    /// remove ThemeItem
    template <typename T>
    void remove(ThemeItem<T> &item);
    virtual bool fallback(ThemeItem_base &) { return false; }
    Signal<> &reconfigSig() { return m_reconfig_sig; }

private:
    const int m_screen_num;

    ItemList m_themeitems;
    Signal<> m_reconfig_sig;
};

/// Proxy interface for themes, so they can be substituted dynamically
template <class BaseTheme>
class ThemeProxy {
public:
    virtual ~ThemeProxy() { }

    virtual Signal<> &reconfigSig() = 0;

    virtual BaseTheme &operator *() = 0;
    virtual const BaseTheme &operator *() const = 0;
    virtual BaseTheme *operator ->() { return &(**this); }
    virtual const BaseTheme *operator ->() const { return &(**this); }
};

/// Singleton theme manager
/**
 Use this to load all the registred themes
*/
class ThemeManager {
public:
    typedef std::list<FbTk::Theme *> ThemeList;
    typedef std::vector<ThemeList> ScreenThemeVector;

    static ThemeManager &instance();
    /// load style file "filename" to screen
    bool load(const std::string &filename, const std::string &overlay_filename, int screen_num = -1);
    std::string resourceValue(const std::string &name, const std::string &altname);
    void loadTheme(Theme &tm);
    bool loadItem(ThemeItem_base &resource);
    bool loadItem(ThemeItem_base &resource, const std::string &name, const std::string &altname);

    bool verbose() const { return m_verbose; }
    void setVerbose(bool value) { m_verbose = value; }

    // dump theme out to filename, stdout if no filename is given
    void dump(Theme& theme, const char* filename = 0) const;
    //    void listItems();
private:
    ThemeManager();
    ~ThemeManager() { }

    friend class FbTk::Theme; // so only theme can register itself in constructor
    /// @return false if screen_num if out of
    /// range or theme already registered, else true
    bool registerTheme(FbTk::Theme &tm);
    /// @return false if theme isn't registred in the manager
    bool unregisterTheme(FbTk::Theme &tm);
    /// map each theme manager to a screen

    ScreenThemeVector m_themes;
    int m_max_screens;
    XrmDatabaseHelper m_database;
    bool m_verbose;

    std::string m_themelocation;
};



template <typename T>
ThemeItem<T>::ThemeItem(FbTk::Theme &tm,
                        const std::string &name, const std::string &altname):
    ThemeItem_base(name, altname),
    m_tm(tm) {
    tm.add(*this);
    setDefaultValue();
}

template <typename T>
ThemeItem<T>::~ThemeItem() {
    m_tm.remove(*this);
}

template <typename T>
void Theme::add(ThemeItem<T> &item) {
    m_themeitems.push_back(&item);
    m_themeitems.unique();
}

template <typename T>
void Theme::remove(ThemeItem<T> &item)  {
    m_themeitems.remove(&item);
}

} // end namespace FbTk

#endif // FBTK_THEME_HH

