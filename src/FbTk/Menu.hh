// Menu.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2001 - 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Basemenu.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

// $Id$

#ifndef	 FBTK_MENU_HH
#define	 FBTK_MENU_HH

#include <X11/Xlib.h>
#include <vector>
#include <string>
#include <memory>

#include "FbWindow.hh"
#include "EventHandler.hh"
#include "RefCount.hh"
#include "Command.hh"
#include "Observer.hh"
#include "FbPixmap.hh"
#include "MenuTheme.hh"
#include "Timer.hh"
#include "FbString.hh"
#include "TypeAhead.hh"

namespace FbTk {

class MenuItem;
class ImageControl;

///   Base class for menus
class Menu: public FbTk::EventHandler, FbTk::FbWindowRenderer,
            public FbTk::Observer {
public:
    enum Alignment{ ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
    enum { RIGHT = 1, LEFT };
	
    /**
       Bullet type
    */
    enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };
	
    Menu(MenuTheme &tm, ImageControl &imgctrl);
    virtual ~Menu();

    /**
       @name manipulators
    */
    //@{
    /// add a menu item with a label and a command
    int insert(const FbString &label, RefCount<Command> &cmd, int pos=-1);
    /// add empty menu item
    int insert(const FbString &label, int pos=-1);
    /// add submenu
    int insert(const FbString &label, Menu *submenu, int pos= -1);
    /// add menu item
    int insert(MenuItem *item, int pos=-1);
    /// remove an item
    int remove(unsigned int item);
    /// remove all items
    void removeAll();
    inline void setInternalMenu(bool val = true) { m_internal_menu = val; }
    inline void setAlignment(Alignment a) { m_alignment = a; }
#ifdef NOT_USED
    inline void setTorn() { m_torn = true; }
    inline void removeParent() { if (m_internal_menu) m_parent = 0; }
#endif
    /// raise this window
    virtual void raise();
    /// lower this window
    virtual void lower();
    /// cycle through menuitems
    void cycleItems(bool reverse);
    void enterSubmenu();

    void disableTitle();
    void enableTitle();

    void setScreen(int x, int y, int w, int h);

    /**
       @name event handlers
    */
    //@{
    void handleEvent(XEvent &event);
    void buttonPressEvent(XButtonEvent &bp);
    virtual void buttonReleaseEvent(XButtonEvent &br);
    void motionNotifyEvent(XMotionEvent &mn);
    void exposeEvent(XExposeEvent &ee);
    void keyPressEvent(XKeyEvent &ke);
    //@}
    /// get input focus
    void grabInputFocus();
    virtual void reconfigure();
    /// set label string
    void setLabel(const FbString &labelstr);
    /// move menu to x,y
    virtual void move(int x, int y);
    virtual void updateMenu(int active_index = -1);
    void setItemSelected(unsigned int index, bool val);
    void setItemEnabled(unsigned int index, bool val);
    inline void setMinimumSublevels(int m) { menu.minsub = m; }
    virtual void drawSubmenu(unsigned int index);
    /// show menu
    virtual void show();
    /// hide menu
    virtual void hide(bool force = false);
    virtual void clearWindow();
#ifdef NOT_USED
    void setActiveIndex(int index) { m_active_index = index; }
    /*@}*/
	
    /**
       @name accessors
    */
    //@{
    inline int activeIndex() const { return m_active_index; }
#endif
    inline bool isTorn() const { return m_torn; }
    inline bool isVisible() const { return m_visible; }
    inline bool isMoving() const { return m_moving; }
    inline int screenNumber() const { return menu.window.screenNumber(); }
    inline Window window() const { return menu.window.window(); }
    inline FbWindow &fbwindow() { return menu.window; }
    inline const FbWindow &fbwindow() const { return menu.window; }
    inline FbWindow &titleWindow() { return menu.title; }
    inline FbWindow &frameWindow() { return menu.frame; }
    inline const std::string &label() const { return menu.label; }  
    inline int x() const { return menu.window.x(); }
    inline int y() const { return menu.window.y(); }
    inline unsigned int width() const { return menu.window.width(); }
    inline unsigned int height() const { return menu.window.height(); }
    inline size_t numberOfItems() const { return menuitems.size(); }
    inline int currentSubmenu() const { return m_which_sub; } 

    bool isItemSelected(unsigned int index) const;
    bool isItemEnabled(unsigned int index) const;
    bool isItemSelectable(unsigned int index) const;
    inline const MenuTheme &theme() const { return m_theme; }
    inline unsigned char alpha() const { return theme().alpha(); }
    inline static Menu *shownMenu() { return shown; }
    inline static Menu *focused() { return s_focused; }
    static void hideShownMenu(bool force = true);
    /// @return menuitem at index
    inline const MenuItem *find(unsigned int index) const { return menuitems[index]; }
    inline MenuItem *find(unsigned int index) { return menuitems[index]; }
    //@}
    /// @return true if index is valid
    inline bool validIndex(int index) const { return (index < static_cast<int>(numberOfItems()) && index >= 0); }

    inline Menu *parent() { return m_parent; }
    inline const Menu *parent() const { return m_parent; }

    void renderForeground(FbWindow &win, FbDrawable &drawable);

protected:

    inline void setTitleVisibility(bool b) { 
        m_title_vis = b; m_need_update = true; 
        if (!b)
            titleWindow().lower();
        else
            titleWindow().raise();
    }

    // renders item onto pm
    int drawItem(FbDrawable &pm, unsigned int index,
                 bool highlight = false,
                 bool exclusive_drawable = false);
    void clearItem(int index, bool clear = true, int search_index = -1);
    void highlightItem(int index);
    virtual void redrawTitle(FbDrawable &pm);
    virtual void redrawFrame(FbDrawable &pm);

    virtual void internal_hide(bool first = true);

    virtual void update(FbTk::Subject *);

private: 

    void openSubmenu();
    void closeMenu();
    void startHide();
    void stopHide();


    typedef std::vector<MenuItem *> Menuitems;
    MenuTheme &m_theme;
    Menu *m_parent;
    ImageControl &m_image_ctrl;
    Menuitems menuitems;

    TypeAhead<Menuitems, MenuItem *> m_type_ahead;
    Menuitems m_matches;

    void resetTypeAhead();
    void drawTypeAheadItems();
    void drawLine(int index, int size);
    void fixMenuItemIndices();

    int m_screen_x, m_screen_y;
    unsigned int m_screen_width, m_screen_height;
    bool m_moving; ///< if we're moving/draging or not
    bool m_closing; ///< if we're right clicking on the menu title
    bool m_visible; ///< menu visibility
    bool m_torn; ///< torn from parent
    bool m_internal_menu; ///< whether we should destroy this menu or if it's managed somewhere else
    bool m_title_vis; ///< title visibility
	
    int m_which_sub;
    Alignment m_alignment;

    struct _menu {
        Pixmap frame_pixmap, title_pixmap, hilite_pixmap;
        FbTk::FbWindow window, frame, title;

        std::string label;
        int x_move, y_move, sublevels, persub, minsub, grab_x, grab_y;

        unsigned int frame_h, item_w;
    } menu;

    int m_active_index; ///< current highlighted index

    Drawable m_root_pm;
    static Menu *shown; ///< used for determining if there's a menu open at all
    static Menu *s_focused; ///< holds current input focused menu, so one can determine if a menu is focused
    bool m_need_update;
    Timer m_submenu_timer;
    Timer m_hide_timer;
};

} // end namespace FbTk

#endif // FBTK_MENU_HH
