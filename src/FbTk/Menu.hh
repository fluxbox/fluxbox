// Menu.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Menu.hh,v 1.23 2003/08/30 01:03:12 fluxgen Exp $

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

namespace FbTk {

class MenuItem;
class ImageControl;
class Transparent;

///   Base class for menus
class Menu: public FbTk::EventHandler {
public:
    enum Alignment{ ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
    enum { RIGHT = 1, LEFT };
	
    /**
       Bullet type
    */
    enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };
	
    Menu(MenuTheme &tm, int screen_num, ImageControl &imgctrl);
    virtual ~Menu();

    /**
       @name manipulators
    */
    //@{
    /// add a menu item with a label and a command
    int insert(const char *label, RefCount<Command> &cmd, int pos=-1);
    /// add empty menu item
    int insert(const char *label, int pos=-1);
    /// add submenu
    int insert(const char *label, Menu *submenu, int pos= -1);
    /// add menu item
    int insert(MenuItem *item, int pos=-1);
    /// remove an item
    int remove(unsigned int item);
    /// remove all items
    void removeAll();
    inline void setInternalMenu(bool val = true) { internal_menu = val; }
    inline void setAlignment(Alignment a) { m_alignment = a; }
    inline void setTorn() { torn = true; }
    inline void removeParent() { if (internal_menu) m_parent = 0; }
    /// raise this window
    virtual void raise();
    /// lower this window
    virtual void lower();
    /// select next item
    void nextItem();
    /// select previous item
    void prevItem();
    void enterSubmenu();
    void enterParent();

    void disableTitle();
    void enableTitle();

    /**
       @name event handlers
    */
    //@{
    void handleEvent(XEvent &event);
    void buttonPressEvent(XButtonEvent &bp);
    void buttonReleaseEvent(XButtonEvent &br);
    void motionNotifyEvent(XMotionEvent &mn);
    void enterNotifyEvent(XCrossingEvent &en);
    void leaveNotifyEvent(XCrossingEvent &ce);
    void exposeEvent(XExposeEvent &ee);
    void keyPressEvent(XKeyEvent &ke);
    //@}
    /// get input focus
    void grabInputFocus();
    virtual void reconfigure();
    /// set label string
    void setLabel(const char *labelstr);
    /// move menu to x,y
    void move(int x, int y);
    void update(int active_index = -1);
    void setItemSelected(unsigned int index, bool val);
    void setItemEnabled(unsigned int index, bool val);
    inline void setMinimumSublevels(int m) { menu.minsub = m; }
    virtual void drawSubmenu(unsigned int index);
    /// show menu
    virtual void show();
    /// hide menu
    virtual void hide();
    virtual void clearWindow();
    /*@}*/
	
    /**
       @name accessors
    */
    //@{
    bool isTorn() const { return torn; }
    bool isVisible() const { return visible; }
    int screenNumber() const { return m_screen_num; }
    Window window() const { return menu.window.window(); }
    FbWindow &fbwindow() { return menu.window; }
    const FbWindow &fbwindow() const { return menu.window; }
    FbWindow &titleWindow() { return menu.title; }
    FbWindow &frameWindow() { return menu.frame; }
    const std::string &label() const { return menu.label; }  
    int x() const { return menu.x; }
    int y() const { return menu.y; }
    unsigned int width() const { return menu.width; }
    unsigned int height() const { return menu.height; }
    unsigned int numberOfItems() const { return menuitems.size(); }
    int currentSubmenu() const { return which_sub; } 
    unsigned int titleHeight() const { return menu.title_h; }
    bool hasSubmenu(unsigned int index) const;
    bool isItemSelected(unsigned int index) const;
    bool isItemEnabled(unsigned int index) const;
    const MenuTheme &theme() const { return m_theme; }
    inline unsigned char alpha() const { return m_theme.alpha(); }
    static Menu *focused() { return s_focused; }
    /// @return menuitem at index
    inline const MenuItem *find(unsigned int index) const { return menuitems[index]; }
    inline MenuItem *find(unsigned int index) { return menuitems[index]; }
    //@}

protected:

    inline void setTitleVisibility(bool b) { title_vis = b; m_need_update = true; }
    inline void setMovable(bool b) { movable = b; }
    inline void setHideTree(bool h) { hide_tree = h; }

    virtual void itemSelected(int button, unsigned int index) { }
    virtual void drawItem(unsigned int index, bool highlight = false, 
                          bool clear= false, bool render_trans = true,
                          int x= -1, int y= -1, 
                          unsigned int width= 0, unsigned int height= 0);
    virtual void redrawTitle();
    virtual void internal_hide();
    inline Menu *parent() { return m_parent; }
    inline const Menu *parent() const { return m_parent; }

private: 
    void renderTransFrame();

    typedef std::vector<MenuItem *> Menuitems;
    const MenuTheme &m_theme;
    Display *m_display;
    const int m_screen_num;
    Menu *m_parent;
    ImageControl &m_image_ctrl;
    Menuitems menuitems;

    const unsigned int m_screen_width, m_screen_height;
    bool moving, visible, movable, torn, internal_menu, title_vis, shifted,
        hide_tree;
	
    int which_sub, which_press, which_sbl;
    Alignment m_alignment;
    int m_border_width;
    struct _menu {
        Pixmap frame_pixmap, title_pixmap, hilite_pixmap, sel_pixmap;
        FbTk::FbWindow window, frame, title;

        std::string label;
        int x, y, x_move, y_move, x_shift, y_shift, sublevels, persub, minsub,
            grab_x, grab_y;
        unsigned int width, height, title_h, frame_h, item_w, item_h, bevel_w,
            bevel_h;
    } menu;

    class ThemeObserver:public Observer {
    public:
        ThemeObserver(FbTk::Menu &menu):m_menu(menu) { }
        void update(FbTk::Subject *subj) {
            m_menu.reconfigure();
        }
    private:
        Menu &m_menu;
    };

    ThemeObserver m_themeobserver;
    Drawable m_root_pm;
    static Menu *s_focused; ///< holds current input focused menu, so one can determine if a menu is focused
    FbPixmap m_frame_pm;
    bool m_need_update;
};

}; // end namespace FbTk

#endif // FBTK_MENU_HH
