// Theme.hh for FbTk - Fluxbox ToolKit
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

// $Id: Theme.hh,v 1.15 2004/01/02 22:55:15 fluxgen Exp $

/**
 @file holds ThemeItem<T>, Theme and ThemeManager which is the base for any theme
*/

#ifndef FBTK_THEME_HH
#define FBTK_THEME_HH

#include <string>
#include <list>
#include <string>

#include "XrmDatabaseHelper.hh"
#include "Subject.hh"

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
    virtual void load() = 0; // if it needs to load additional stuff
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
    void setFromString(const char *strval);
    /// specialized
    void load();
    /**
       @name access operators
    */
    /**@{*/
    inline T& operator*() { return m_value; }
    inline const T& operator*() const { return m_value; }
    inline T *operator->() { return &m_value; }
    inline const T *operator->() const { return &m_value; }
    /**@}*/

    FbTk::Theme &theme() { return m_tm; }
private:

    T m_value;
    FbTk::Theme &m_tm;
};


/// Hold ThemeItems. Use this to create a Theme set
class Theme {
public:
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
    FbTk::Subject &reconfigSig() { return m_reconfig_sig; }
    const FbTk::Subject &reconfigSig() const { return m_reconfig_sig; }

    
private:
    const int m_screen_num;
    typedef std::list<ThemeItem_base *> ItemList;
    ItemList m_themeitems;
    FbTk::Subject m_reconfig_sig;
};


/// Singleton theme manager
/**
 Use this to load all the registred themes
*/
class ThemeManager {
public:
    static ThemeManager &instance();
    /// load style file "filename" to screen 
    bool load(const std::string &filename, int screen_num = -1);
    std::string resourceValue(const std::string &name, const std::string &altname);
    void loadTheme(Theme &tm);
    bool loadItem(ThemeItem_base &resource);
    bool loadItem(ThemeItem_base &resource, const std::string &name, const std::string &altname);

    bool verbose() const { return m_verbose; }
    void setVerbose(bool value) { m_verbose = value; }
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
    typedef std::list<FbTk::Theme *> ThemeList;
    ThemeList m_themelist;
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

