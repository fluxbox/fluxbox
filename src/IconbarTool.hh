// IconbarTool.hh
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef ICONBARTOOL_HH
#define ICONBARTOOL_HH

#include "ToolbarItem.hh"
#include "FbMenu.hh"

#include "FbTk/Container.hh"
#include "FbTk/CachedPixmap.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Timer.hh"

#include <list>
#include <map>
#include <memory>
#include <vector>

class IconbarTheme;
class BScreen;
class IconButton;
class Focusable;
class FocusableList;
class FluxboxWindow;

class IconbarTool: public ToolbarItem {
public:
    typedef std::map<Focusable *, IconButton *> IconMap;

    /// the position of a button along the axis the buttons are laid out on
    struct AxisBox {
        int start;  ///< start of the button
        int length; ///< length of the button
    };

    /// where to move an icon to
    enum MoveIconTarget {
        MOVE_ICON_LEFT,  ///< one slot towards the front of the iconbar
        MOVE_ICON_RIGHT, ///< one slot towards the end of the iconbar
        MOVE_ICON_START, ///< the first slot of the iconbar
        MOVE_ICON_END    ///< the last slot of the iconbar
    };

    IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme,
                FbTk::ThemeProxy<IconbarTheme> &focused_theme,
                FbTk::ThemeProxy<IconbarTheme> &unfocused_theme,
                BScreen &screen, FbTk::Menu &menu,
                std::shared_ptr<std::list<Focusable *> > order);
    ~IconbarTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void show();
    void hide();
    void setAlignment(FbTk::Container::Alignment a);
    void setMode(std::string mode);
    void parentMoved() { m_icon_container.parentMoved(); }

    unsigned int width() const;
    unsigned int preferredWidth() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    std::string mode() const { return *m_rc_mode; }

    void setOrientation(FbTk::Orientation orient);
    FbTk::Container::Alignment alignment() const { return m_icon_container.alignment(); }
    static std::string &iconifiedPrefix() { return s_iconifiedDecoration[0]; }
    static std::string &iconifiedSuffix() { return s_iconifiedDecoration[1]; }

    /**
       Move the icon of the given window in the iconbar.
       @param win the window whose icon should be moved
       @param target where the icon should go
       @return true if the icon was moved
     */
    bool moveIcon(FluxboxWindow &win, MoveIconTarget target);
    /**
       Move the icon of the given window to the given slot.
       @param win the window whose icon should be moved
       @param index the zero based slot the icon should end up in
       @return true if the icon was moved
     */
    bool moveIconTo(FluxboxWindow &win, int index);

    /// @return the number of icons in the iconbar
    int iconCount() const;
    /**
       Set whether the order of the icons is controlled by the user.

       While the order is user controlled, the iconbar does not follow the
       focus or creation order of the windows any more. Moving an icon
       turns this on automatically.
       @param manual true to let the user control the order
     */
    void setManualOrder(bool manual);
    /// @return true if the order of the icons is controlled by the user
    bool isManualOrder() const { return *m_rc_manual_order; }

    /**
       @name drag and drop
     */
    //@{
    /// a drag gesture started on the button of the given window
    void dragStarted(Focusable &win, int root_x, int root_y);
    /// the pointer moved during a drag gesture
    void dragMoved(int root_x, int root_y);
    /// a drag gesture ended
    void dragEnded();
    //@}

    const BScreen &screen() const { return m_screen; }
private:
    enum UpdateReason { LIST_ORDER, LIST_ADD, LIST_REMOVE, LIST_RESET, ALIGN };

    void updateSizing();

    /// render single button, and probably apply changes (clear)
    /// @param button the button to render
    /// @param clear if the window should be cleared first
    void renderButton(IconButton &button, bool clear = true);
    /// render all buttons
    void renderTheme();
    void renderTheme(int alpha);
    /// destroy all icons
    void deleteIcons();
    /// add or move a single window
    void insertWindow(Focusable &win, int pos = -2);
    /// remove a single window
    void removeWindow(Focusable &win);
    /// make a button for the window
    IconButton *makeButton(Focusable &win);
    /// remove all windows and add again
    void reset();
    /// add icons to the list
    void updateList();

    /// move the icon of the given focusable to the given slot
    bool moveFocusableTo(Focusable &win, int index);
    /// apply the manual order resource to the window list
    void applyManualOrder();
    /// @return the focusable of the given window that is in the iconbar, or 0
    Focusable *focusableFor(FluxboxWindow &win);
    /// @return the index of the given focusable in the iconbar, or -1
    int indexOf(const Focusable &win);
    /// @return the slot a button should be dropped at for a pointer position
    int indexForPosition(int root_x, int root_y);
    /// remember the position of the buttons, so that the pointer keeps
    /// referring to the same place during a drag
    void snapshotGeometry();

    void updateMaxSizes(unsigned int width, unsigned int height);
    /// called when the list emits a signal
    void update(UpdateReason reason, Focusable *win);

    void updateIconifiedPattern();

    void themeReconfigured();

    FbTk::Timer m_resizeSig_timer;
    void emitResizeSig();

    BScreen &m_screen;
    FbTk::Container m_icon_container;
    IconbarTheme &m_theme;
    FbTk::ThemeProxy<IconbarTheme> &m_focused_theme, &m_unfocused_theme;
    FbTk::CachedPixmap m_empty_pm; ///< pixmap for empty container

    FbTk::SignalTracker m_tracker;

    std::unique_ptr<FocusableList> m_winlist;
    /// the order of the icons, kept here so that it survives a new mode
    std::shared_ptr<std::list<Focusable *> > m_order;
    IconMap m_icons;
    std::string m_mode;
    FbTk::Resource<std::string> m_rc_mode;
    FbTk::Resource<bool> m_rc_manual_order; ///< true if the user controls the order
    FbTk::Resource<FbTk::Container::Alignment> m_rc_alignment; ///< alignment of buttons
    FbTk::Resource<int> m_rc_client_width; ///< size of client button in LEFT/RIGHT mode
    FbTk::Resource<unsigned int> m_rc_client_padding; ///< padding of the text
    FbTk::Resource<bool> m_rc_use_pixmap; ///< if iconbar should use win pixmap or not
    FbMenu m_menu;
    int m_alpha;

    Focusable *m_drag_win; ///< the window whose icon is being dragged
    bool m_dragging; ///< true while a drag gesture is in progress
    int m_drag_index; ///< the slot the dragged icon would end up in
    int m_drag_slot; ///< the slot the dragged icon started in
    int m_drag_border; ///< border width of the dragged icon
    int m_drag_origin_x; ///< x of the icon container, in root window coordinates
    int m_drag_origin_y; ///< y of the icon container, in root window coordinates
    /// the position of the buttons when the drag started
    std::vector<AxisBox> m_drag_geometry;
    static std::string s_iconifiedDecoration[2];
};

#endif // ICONBARTOOL_HH
