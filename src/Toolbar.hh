// Toolbar.hh for Fluxbox
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Toolbar.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// $Id: Toolbar.hh,v 1.40 2003/08/11 15:54:24 fluxgen Exp $

#ifndef	 TOOLBAR_HH
#define	 TOOLBAR_HH

#include "Timer.hh"
#include "ToolbarTheme.hh"
#include "ToolTheme.hh"

#include "EventHandler.hh"
#include "FbWindow.hh"
#include "ArrowButton.hh"
#include "Observer.hh"
#include "XLayer.hh"
#include "XLayerItem.hh"
#include "LayerMenu.hh"
#include "Resource.hh"

#include <memory>

class BScreen;
class Strut;
class Container;
class IconButton;
class Shape;
class ToolbarItem;

namespace FbTk {
class ImageControl;
};

typedef Container IconBar;
///	The toolbar.
/// Handles iconbar, workspace name view and clock view
class Toolbar: public FbTk::EventHandler, public FbTk::Observer {
public:
       
    /// Toolbar placement on the screen
    enum Placement{ 
        // top and bottom placement
        TOPLEFT = 1, BOTTOMLEFT, TOPCENTER,
        BOTTOMCENTER, TOPRIGHT, BOTTOMRIGHT,
        // left and right placement
        LEFTCENTER, LEFTBOTTOM, LEFTTOP,
        RIGHTCENTER, RIGHTBOTTOM, RIGHTTOP        
    };

    /// Create a toolbar on the screen with specific width
    Toolbar(BScreen &screen, FbTk::XLayer &layer, FbTk::Menu &menu, size_t width = 200);

    virtual ~Toolbar();

    /// add icon to iconbar
    void addIcon(FluxboxWindow *w);
    /// remove icon from iconbar
    void delIcon(FluxboxWindow *w);
    bool containsIcon(const FluxboxWindow &win) const;
    /// remove all icons
    void delAllIcons(bool ignore_stuck = false);
    void enableIconBar();
    void disableIconBar();
    void raise();
    void lower();
    void toggleHidden();

    void enableUpdates();
    void disableUpdates();


    void moveToLayer(int layernum);

    void saveOnHead(int head);

    /**
       @name eventhandlers
    */
    //@{
    void buttonPressEvent(XButtonEvent &be);
    void buttonReleaseEvent(XButtonEvent &be);
    void enterNotifyEvent(XCrossingEvent &ce);
    void leaveNotifyEvent(XCrossingEvent &ce);
    void exposeEvent(XExposeEvent &ee);
    void keyPressEvent(XKeyEvent &ke);
    //@}
	
    void redrawWindowLabel(bool redraw= false);
    void redrawWorkspaceLabel(bool redraw= false);
    /// enter edit mode on workspace label
    void edit();
    void reconfigure();
    void setPlacement(Placement where);
    void checkClock(bool redraw = false, bool date = false);

    void update(FbTk::Subject *subj);

    FbTk::XLayerItem &layerItem() { return m_layeritem; }

    inline const FbTk::Menu &menu() const { return m_toolbarmenu; }
    inline FbTk::Menu &menu() { return m_toolbarmenu; }
    inline FbTk::Menu &placementMenu() { return m_placementmenu; }
    inline const FbTk::Menu &placementMenu() const { return m_placementmenu; }

    inline FbTk::Menu &layermenu() { return m_layermenu; }
    inline const FbTk::Menu &layermenu() const { return m_layermenu; }

    /// are we in workspacename editing?
    inline bool isEditing() const { return m_editing; }
    /// are we hidden?
    inline bool isHidden() const { return m_hidden; }
    /// do we auto hide the toolbar?
    inline bool doAutoHide() const { return *m_rc_auto_hide; }
    ///	@return X window of the toolbar
    inline const FbTk::FbWindow &window() const { return frame.window; }
    inline BScreen &screen() { return m_screen; }
    inline const BScreen &screen() const { return m_screen; }
    inline unsigned int width() const { return frame.width; }
    inline unsigned int height() const { return frame.height; }
    inline unsigned int exposedHeight() const { return doAutoHide() ? frame.bevel_w : frame.height; }
    inline int x() const { return isHidden() ? frame.x_hidden : frame.x; }
    inline int y() const { return isHidden() ? frame.y_hidden : frame.y; }
    inline Placement placement() const { return *m_rc_placement; }
    /// @return pointer to iconbar if it got one, else 0
    inline const IconBar *iconBar()  const { return m_iconbar.get(); }
    inline const ToolbarTheme &theme() const { return m_theme; }
    inline ToolbarTheme &theme() { return m_theme; }
    bool isVertical() const;

    inline int getOnHead() const { return *m_rc_on_head; }
		
private:
    void updateIconbarGraphics();
    void setupMenus();
    void clearStrut();
    void updateStrut();

    bool m_editing;      ///< edit workspace label mode
    bool m_hidden;       ///< hidden state

    /// Toolbar frame
    struct Frame {
        Frame(FbTk::EventHandler &evh, int screen_num);
        ~Frame();

        Pixmap base, label;
        FbTk::FbWindow window, window_label;

        int x, y, x_hidden, y_hidden, grab_x, grab_y;
        unsigned int width, height, window_label_w, bevel_w, label_h;
    } frame;

    Pixmap m_icon_focused_pm, m_icon_unfocused_pm;
    FbTk::Color m_icon_focused_color, m_icon_unfocused_color;

    BScreen &m_screen; ///< screen connection

    FbTk::Timer m_hide_timer; ///< timer to for auto hide toolbar
    FbTk::Menu &m_toolbarmenu;
    FbTk::Menu m_placementmenu;
    LayerMenu<Toolbar> m_layermenu;

    // icon stuff
    std::auto_ptr<Container> m_iconbar;
    typedef std::map<FluxboxWindow *, IconButton *> Icon2WinMap;
    Icon2WinMap m_icon2winmap;
	
    std::string m_new_workspace_name; ///< temp variable in edit workspace name mode

    ToolbarTheme m_theme;

    FbTk::XLayerItem m_layeritem;
    typedef std::list<ToolbarItem *> ItemList;
    ItemList m_item_list;

    Strut *m_strut; ///< created and destroyed by BScreen
    // resources
    FbTk::Resource<bool> m_rc_auto_hide, m_rc_maximize_over;
    FbTk::Resource<int> m_rc_width_percent;
    FbTk::Resource<Fluxbox::Layer> m_rc_layernum;
    FbTk::Resource<int> m_rc_on_head;
    FbTk::Resource<Placement> m_rc_placement;
    std::auto_ptr<Shape> m_shape;

    ToolTheme m_tool_theme;

};


#endif // TOOLBAR_HH
