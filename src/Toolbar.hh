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

// $Id: Toolbar.hh,v 1.46 2003/08/29 00:44:41 fluxgen Exp $

#ifndef	 TOOLBAR_HH
#define	 TOOLBAR_HH

#include "Timer.hh"
#include "ToolbarTheme.hh"
#include "ToolTheme.hh"
#include "IconbarTheme.hh"

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

    void raise();
    void lower();
    void toggleHidden();


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
    void handleEvent(XEvent &event);
    //@}
	
    /// enter edit mode on workspace label
    void edit();
    void reconfigure();
    void setPlacement(Placement where);

    void update(FbTk::Subject *subj);

    FbTk::XLayerItem &layerItem() { return m_layeritem; }

    inline const FbTk::Menu &menu() const { return m_toolbarmenu; }
    inline FbTk::Menu &menu() { return m_toolbarmenu; }
    inline FbTk::Menu &placementMenu() { return m_placementmenu; }
    inline const FbTk::Menu &placementMenu() const { return m_placementmenu; }

    inline FbTk::Menu &layermenu() { return m_layermenu; }
    inline const FbTk::Menu &layermenu() const { return m_layermenu; }

    /// are we hidden?
    inline bool isHidden() const { return m_hidden; }
    /// do we auto hide the toolbar?
    inline bool doAutoHide() const { return *m_rc_auto_hide; }
    ///	@return X window of the toolbar
    inline const FbTk::FbWindow &window() const { return frame.window; }
    inline BScreen &screen() { return m_screen; }
    inline const BScreen &screen() const { return m_screen; }
    inline unsigned int width() const { return frame.window.width(); }
    inline unsigned int height() const { return frame.window.height(); }
    inline unsigned int exposedHeight() const { return doAutoHide() ? frame.bevel_w : frame.height; }
    inline int x() const { return isHidden() ? frame.x_hidden : frame.x; }
    inline int y() const { return isHidden() ? frame.y_hidden : frame.y; }
    inline Placement placement() const { return *m_rc_placement; }
    /// @return pointer to iconbar if it got one, else 0
    inline const ToolbarTheme &theme() const { return m_theme; }
    inline ToolbarTheme &theme() { return m_theme; }
    bool isVertical() const;

    inline int getOnHead() const { return *m_rc_on_head; }
		
private:
    void rearrangeItems();
    void deleteItems();

    void updateIconbarGraphics();
    void setupMenus();
    void clearStrut();
    void updateStrut();

    bool m_hidden;       ///< hidden state

    /// Toolbar frame
    struct Frame {
        Frame(FbTk::EventHandler &evh, int screen_num);
        ~Frame();

        FbTk::FbWindow window;

        int x, y, x_hidden, y_hidden, grab_x, grab_y;
        unsigned int width, height, bevel_w;
    } frame;
    // background pixmap
    Pixmap m_window_pm;

    BScreen &m_screen; ///< screen connection

    FbTk::Timer m_hide_timer; ///< timer to for auto hide toolbar
    FbTk::Menu &m_toolbarmenu;
    FbTk::Menu m_placementmenu;
    LayerMenu<Toolbar> m_layermenu;

    // themes
    ToolbarTheme m_theme;
    ToolTheme m_clock_theme;
    std::auto_ptr<ToolTheme> m_workspace_theme;
    IconbarTheme m_iconbar_theme;

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
    FbTk::Resource<int> m_rc_height;
    FbTk::Resource<std::string> m_rc_tools;
    std::auto_ptr<Shape> m_shape;
    typedef std::list<std::string> StringList;
    StringList m_tools;

    bool m_resize_lock; ///< to lock rearrangeItems or not
};


#endif // TOOLBAR_HH
