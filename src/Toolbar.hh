// Toolbar.hh for Fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Toolbar.hh,v 1.26 2003/03/03 21:51:08 rathnor Exp $

#ifndef	 TOOLBAR_HH
#define	 TOOLBAR_HH

#include "Timer.hh"
#include "IconBar.hh"
#include "ToolbarTheme.hh"
#include "EventHandler.hh"
#include "FbWindow.hh"
#include "ArrowButton.hh"
#include "Observer.hh"
#include "XLayer.hh"
#include "XLayerItem.hh"
#include "LayerMenu.hh"

#include <memory>


namespace FbTk {
class ImageControl;
};

///	The toolbar.
/**
   Handles iconbar, workspace name view and clock view
 */
class Toolbar : public FbTk::TimeoutHandler, public FbTk::EventHandler {
public:
       
    ///Toolbar placement on the screen
    enum Placement{ 
        // top and bottom placement
        TOPLEFT = 1, BOTTOMLEFT, TOPCENTER,
        BOTTOMCENTER, TOPRIGHT, BOTTOMRIGHT,
        // left and right placement
        LEFTCENTER, LEFTBOTTOM, LEFTTOP,
        RIGHTCENTER, RIGHTBOTTOM, RIGHTTOP        
    };

    /// create a toolbar on the screen with specific width
    explicit Toolbar(BScreen &screen, FbTk::XLayer &layer, FbTk::Menu &menu, size_t width = 200);
    /// destructor
    virtual ~Toolbar();

    /// add icon to iconbar
    void addIcon(FluxboxWindow *w);
    /// remove icon from iconbar
    void delIcon(FluxboxWindow *w);
    /// remove all icons
    void delAllIcons();
    bool containsIcon(FluxboxWindow &win);

    void enableIconBar();
    void disableIconBar();

    inline const FbTk::Menu &menu() const { return m_toolbarmenu; }
    inline FbTk::Menu &menu() { return m_toolbarmenu; }
    inline FbTk::Menu &placementMenu() { return m_placementmenu; }
    inline const FbTk::Menu &placementMenu() const { return m_placementmenu; }

    inline FbTk::Menu &layermenu() { return m_layermenu; }
    inline const FbTk::Menu &layermenu() const { return m_layermenu; }

    void moveToLayer(int layernum) { m_layeritem.moveToLayer(layernum); }

    FbTk::XLayerItem &getLayerItem() { return m_layeritem; }

    /// are we in workspacename editing?
    inline bool isEditing() const { return editing; }
    /// are we hidden?
    inline bool isHidden() const { return hidden; }
    /// do we auto hide the toolbar?
    inline bool doAutoHide() const { return do_auto_hide; }
    ///	@return X window of the toolbar
    inline Window getWindowID() const { return frame.window.window(); }
    inline BScreen &screen() { return m_screen; }
    inline const BScreen &screen() const { return m_screen; }
    inline unsigned int width() const { return frame.width; }
    inline unsigned int height() const { return frame.height; }
    inline unsigned int exposedHeight() const { return ((do_auto_hide) ? frame.bevel_w : frame.height); }
    inline int x() const { return ((hidden) ? frame.x_hidden : frame.x); }
    inline int y() const { return ((hidden) ? frame.y_hidden : frame.y); }
    /// @return pointer to iconbar if it got one, else 0
    inline const IconBar *iconBar()  const { return m_iconbar.get(); }
    inline const ToolbarTheme &theme() const { return m_theme; }
    inline ToolbarTheme &theme() { return m_theme; }
    inline bool isVertical() const;
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

    virtual void timeout();

		
private:

    bool editing;      ///< edit workspace label mode
    bool hidden;       ///< hidden state
    bool do_auto_hide; ///< do we auto hide	
    Display *display;  ///< display connection

    /// Toolbar frame
    struct Frame {
        Frame(FbTk::EventHandler &evh, int screen_num);
        ~Frame();

        Pixmap base, label, wlabel, clk, button, pbutton;
        FbTk::FbWindow window, workspace_label, window_label, clock;
        ArrowButton psbutton, nsbutton, pwbutton, nwbutton;

        int x, y, x_hidden, y_hidden, hour, minute, grab_x, grab_y;
        unsigned int width, height, window_label_w, workspace_label_w, clock_w,
            button_w, bevel_w, label_h;
    } frame;

    class HideHandler : public FbTk::TimeoutHandler {
    public:
        Toolbar *toolbar;

        virtual void timeout();
    } hide_handler;
    friend class HideHandler;

    BScreen &m_screen;
    FbTk::ImageControl &image_ctrl; 
    FbTk::Timer clock_timer; ///< timer to update clock
    FbTk::Timer hide_timer; ///< timer to for auto hide toolbar
    FbTk::Menu &m_toolbarmenu;
    FbTk::Menu m_placementmenu;
    LayerMenu<Toolbar> m_layermenu;
    std::auto_ptr<IconBar> m_iconbar;
	
    std::string new_workspace_name; ///< temp variable in edit workspace name mode

    ToolbarTheme m_theme;
    Placement m_place;
    //!! TODO this is just temporary
    class ThemeListener: public FbTk::Observer {
    public:
        ThemeListener(Toolbar &tb):m_tb(tb) { }
        void update(FbTk::Subject *subj) {
            m_tb.reconfigure();
        }
    private:
        Toolbar &m_tb;
    };
    
    ThemeListener m_themelistener;

    FbTk::XLayerItem m_layeritem;

};


#endif // TOOLBAR_HH
