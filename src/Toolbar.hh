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

// $Id: Toolbar.hh,v 1.18 2002/12/13 20:36:36 fluxgen Exp $

#ifndef	 TOOLBAR_HH
#define	 TOOLBAR_HH

#include "Basemenu.hh"
#include "Timer.hh"
#include "IconBar.hh"
#include "ToolbarTheme.hh"
#include "EventHandler.hh"
#include "FbWindow.hh"
#include "ArrowButton.hh"

#include <memory>

class Toolbar;

/**
	Menu for toolbar.
	@see Toolbar
*/
class Toolbarmenu:public Basemenu {
public:
    explicit Toolbarmenu(Toolbar &tb);
    ~Toolbarmenu();

    inline const Basemenu *headmenu() const { return m_headmenu.get(); }

    inline Basemenu *placementmenu() { return &m_placementmenu; }
    inline const Basemenu *placementmenu() const { return &m_placementmenu; }

    void reconfigure();

protected:
    virtual void itemSelected(int button, unsigned int index);
    virtual void internal_hide();

private:
    class Placementmenu : public Basemenu {
    public:
        Placementmenu(Toolbarmenu &tm);
    protected:
        virtual void itemSelected(int button, unsigned int index);
    private:
        Toolbarmenu &m_toolbarmenu;
    };
    friend class Placementmenu;

    class Headmenu : public Basemenu {
    public:
        Headmenu(Toolbarmenu &tm);
    protected:
        virtual void itemSelected(int button, unsigned int index); 
    private:
        Toolbarmenu &m_toolbarmenu;
    };
    std::auto_ptr<Headmenu> m_headmenu;
    friend class Headmenu;
 

    Toolbar &m_toolbar;
    Placementmenu m_placementmenu;

	
    friend class Toolbar;

};



///	The toolbar.
/**
   Handles iconbar, workspace name view and clock view
 */
class Toolbar : public TimeoutHandler, public FbTk::EventHandler {
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
    explicit Toolbar(BScreen *screen, size_t width = 200);
    /// destructor
    virtual ~Toolbar();

    /// add icon to iconbar
    void addIcon(FluxboxWindow *w);
    /// remove icon from iconbar
    void delIcon(FluxboxWindow *w);
	
    inline const Toolbarmenu &menu() const { return m_toolbarmenu; }
    /// are we in workspacename editing?
    inline bool isEditing() const { return editing; }
    /// always on top?
    inline bool isOnTop() const { return on_top; }
    /// are we hidden?
    inline bool isHidden() const { return hidden; }
    /// do we auto hide the toolbar?
    inline bool doAutoHide() const { return do_auto_hide; }
    ///	@return X window of the toolbar
    inline Window getWindowID() const { return frame.window.window(); }
    inline BScreen *screen() { return m_screen; }
    inline const BScreen *screen() const { return m_screen; }
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

    bool on_top;       ///< always on top
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

    class HideHandler : public TimeoutHandler {
    public:
        Toolbar *toolbar;

        virtual void timeout();
    } hide_handler;

    BScreen *m_screen;
    BImageControl &image_ctrl; 
    BTimer clock_timer; ///< timer to update clock
    BTimer hide_timer; ///< timer to for auto hide toolbar
    Toolbarmenu m_toolbarmenu;
    std::auto_ptr<IconBar> m_iconbar;
	
    std::string new_workspace_name; ///< temp variable in edit workspace name mode

    ToolbarTheme m_theme;
    Placement m_place;

    friend class HideHandler;
    friend class Toolbarmenu;
    friend class Toolbarmenu::Placementmenu;

    friend class Toolbarmenu::Headmenu;
};


#endif // TOOLBAR_HH
