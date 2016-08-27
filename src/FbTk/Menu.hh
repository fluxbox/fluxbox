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

#ifndef FBTK_MENU_HH
#define FBTK_MENU_HH

#include <vector>
#include <memory>

#include "FbString.hh"
#include "FbWindow.hh"
#include "EventHandler.hh"
#include "MenuTheme.hh"
#include "Timer.hh"

namespace FbTk {

template <typename T> class Command;
template <typename T> class RefCount;
class MenuItem;
class MenuSearch;
class ImageControl;

///   Base class for menus
class Menu: public FbTk::EventHandler, FbTk::FbWindowRenderer {
public:

    static Menu* shownMenu();
    static Menu* focused();
    static void hideShownMenu();


    enum Alignment{ ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
    enum { RIGHT = 1, LEFT };
    enum { UP = 1, DOWN = 0 };

    /**
       Bullet type
    */
    enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };

    Menu(FbTk::ThemeProxy<MenuTheme> &tm, ImageControl &imgctrl);
    virtual ~Menu();

    /**
       @name manipulators
    */
    //@{
    int insertCommand(const FbString &label, RefCount<Command<void> > &cmd, int pos=-1);
    int insert(const FbString &label, int pos=-1);
    int insertSubmenu(const FbString &label, Menu *submenu, int pos= -1);
    int insertItem(MenuItem *item, int pos=-1);
    int remove(unsigned int item);
    int removeItem(MenuItem* item);
    void removeAll();
    void setInternalMenu(bool val = true) { m_internal_menu = val; }
    void setAlignment(Alignment a) { m_alignment = a; }

    virtual void raise();
    virtual void lower();
    void cycleItems(bool reverse);
    void setActiveIndex(int new_index);
    void enterSubmenu();

    void disableTitle();
    void enableTitle();
    bool isTitleVisible() const { return m_title.visible; }

    void setScreen(int x, int y, unsigned int w, unsigned int h);

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
    void leaveNotifyEvent(XCrossingEvent &ce);
    //@}
    void grabInputFocus();
    virtual void reconfigure();
    void setLabel(const FbTk::BiDiString &labelstr);
    virtual void move(int x, int y);
    virtual void updateMenu();
    void setItemSelected(unsigned int index, bool val);
    void setItemEnabled(unsigned int index, bool val);
    void setMinimumColumns(int columns) { m_min_columns = columns; }
    virtual void drawSubmenu(unsigned int index);
    virtual void show();
    virtual void hide(bool force = false);
    virtual void clearWindow();
    /*@}*/

    /**
       @name accessors
    */
    //@{
    bool isTorn() const              { return m_state.torn; }
    bool isVisible() const           { return m_state.visible; }
    bool isMoving() const            { return m_state.moving; }
    int screenNumber() const         { return m_window.screenNumber(); }
    Window window() const            { return m_window.window(); }
    FbWindow &fbwindow()             { return m_window; }
    const FbWindow &fbwindow() const { return m_window; }
    FbWindow &titleWindow()          { return m_title.win; }
    FbWindow &frameWindow()          { return m_frame.win; }

    const FbTk::BiDiString &label() const { return m_title.label; }
    int x() const                    { return m_window.x(); }
    int y() const                    { return m_window.y(); }
    unsigned int width() const       { return m_window.width(); }
    unsigned int height() const      { return m_window.height(); }
    size_t numberOfItems() const     { return m_items.size(); }
    int currentSubmenu() const       { return m_which_sub; }

    bool isItemSelected(unsigned int index) const;
    bool isItemEnabled(unsigned int index) const;
    bool isItemSelectable(unsigned int index) const;
    FbTk::ThemeProxy<MenuTheme> &theme() { return m_theme; }
    const FbTk::ThemeProxy<MenuTheme> &theme() const { return m_theme; }
    unsigned char alpha() const { return theme()->alpha(); }
    const MenuItem *find(size_t i) const { return m_items[i]; }
    MenuItem *find(size_t i) { return m_items[i]; }

    // returns index of 'submenu', it it is in the top most list of
    // menu items. -1 if no match is found
    int findSubmenuIndex(const Menu* submenu) const;

    //@}
    /// @return true if index is valid
    bool validIndex(int index) const { return (index < static_cast<int>(numberOfItems()) && index >= 0); }

    Menu *parent() { return m_parent; }
    const Menu *parent() const { return m_parent; }

    void renderForeground(FbWindow &win, FbDrawable &drawable);

protected:

    void themeReconfigured();
    void setTitleVisibility(bool b);

    // renders item onto pm
    int drawItem(FbDrawable &pm, unsigned int index,
                 bool highlight = false,
                 bool exclusive_drawable = false);
    void clearItem(int index, bool clear = true, int search_index = -1);
    void highlightItem(int index);
    virtual void redrawTitle(FbDrawable &pm);
    virtual void redrawFrame(FbDrawable &pm);

    virtual void internal_hide(bool first = true);

private:

    void openSubmenu();
    void closeMenu();
    void startHide();
    void stopHide();

    void resetTypeAhead();
    void drawTypeAheadItems();


    Menu *m_parent;

    std::vector<MenuItem*>    m_items;
    std::unique_ptr<MenuSearch> m_search;

    struct State {
        bool moving;
        bool closing; // right click title
        bool visible;
        bool torn; // torn from parent
    } m_state;

    bool m_need_update;
    bool m_internal_menu; ///< whether we should destroy this menu or if it's managed somewhere else
    int m_active_index; ///< current highlighted index
    int m_which_sub;
    int m_x_move;
    int m_y_move;

    struct Rect {
        int x, y;
        unsigned int width, height;
    } m_screen;

    // the menu window
    FbTk::FbWindow m_window;

    // the title
    struct Title {
        FbTk::FbWindow   win;
        Pixmap           pixmap;
        FbTk::BiDiString label;
        bool             visible;
    } m_title;

    // area for the menuitems
    struct Frame {
        FbTk::FbWindow win;
        Pixmap pixmap;
        unsigned int height;
    } m_frame;


    // the menuitems are rendered in a grid with
    // 'm_columns' (a minimum of 'm_min_columns') and
    // a max of 'm_rows_per_column'
    int m_columns;
    int m_rows_per_column;
    int m_min_columns;
    unsigned int m_item_w;

    FbTk::ThemeProxy<MenuTheme>& m_theme;
    ImageControl& m_image_ctrl;
    std::unique_ptr<FbTk::Shape> m_shape; // the corners
    Pixmap      m_hilite_pixmap;
    Alignment   m_alignment;

    Timer m_submenu_timer;
    Timer m_hide_timer;

    SignalTracker m_tracker;
};

} // end namespace FbTk

#endif // FBTK_MENU_HH
