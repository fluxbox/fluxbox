// Toolbar.hh for Fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: Toolbar.hh,v 1.13 2002/11/15 12:04:27 fluxgen Exp $

#ifndef	 TOOLBAR_HH
#define	 TOOLBAR_HH

#include "Basemenu.hh"
#include "Timer.hh"
#include "IconBar.hh"


class Toolbar;

/**
	Menu for toolbar.
	@see Toolbar
*/
class Toolbarmenu:public Basemenu {
public:
	explicit Toolbarmenu(Toolbar *tb);
	~Toolbarmenu();
	#ifdef XINERAMA
	inline Basemenu *getHeadmenu() { return headmenu; }
	#endif // XINERAMA

	inline Basemenu *getPlacementmenu() { return placementmenu; }
	inline const Basemenu *getPlacementmenu() const { return placementmenu; }

	void reconfigure();

protected:
	virtual void itemSelected(int button, unsigned int index);
	virtual void internal_hide();

private:
	class Placementmenu : public Basemenu {
	private:
		Toolbarmenu *toolbarmenu;

	protected:
		virtual void itemSelected(int button, unsigned int index);

	public:
		Placementmenu(Toolbarmenu *);
	};

#ifdef XINERAMA
	class Headmenu : public Basemenu {
	public:
		Headmenu(Toolbarmenu *);
	private:
		Toolbarmenu *toolbarmenu;

	protected:
		virtual void itemSelected(int button, unsigned int index); 
	};
	Headmenu *headmenu;
	friend class Headmenu;
#endif // XINERAMA
 

	Toolbar *toolbar;
	Placementmenu *placementmenu;

	friend class Placementmenu;
	friend class Toolbar;

};

/**
	the toolbar.
*/
class Toolbar : public TimeoutHandler {
public:
	/**
		Toolbar placement on the screen
	*/
	enum Placement{ TOPLEFT = 1, BOTTOMLEFT, TOPCENTER,
		BOTTOMCENTER, TOPRIGHT, BOTTOMRIGHT };

	explicit Toolbar(BScreen *screen);
	virtual ~Toolbar();

	/// add icon to iconbar
	void addIcon(FluxboxWindow *w);
	/// remove icon from iconbar
	void delIcon(FluxboxWindow *w);
	
	inline Toolbarmenu *getMenu() { return toolbarmenu; }
	inline const Toolbarmenu *getMenu() const { return toolbarmenu; }

	//inline Window getWindowLabel(void) { return frame.window_label; }
	
	/// are we in workspacename editing?
	inline bool isEditing() const { return editing; }
	/// always on top?
	inline bool isOnTop() const { return on_top; }
	/// are we hidden?
	inline bool isHidden() const { return hidden; }
	/// do we auto hide the toolbar?
	inline bool doAutoHide() const { return do_auto_hide; }
	/**
		@return X window of the toolbar
	*/
	inline Window getWindowID() const { return frame.window; }

	inline unsigned int width() const { return frame.width; }
	inline unsigned int height() const { return frame.height; }
	inline unsigned int getExposedHeight() const { return ((do_auto_hide) ? frame.bevel_w : frame.height); }
	inline int x() const { return ((hidden) ? frame.x_hidden : frame.x); }
	inline int y() const { return ((hidden) ? frame.y_hidden : frame.y); }
	inline const IconBar *iconBar()  const { return iconbar; }
	/**
		@name eventhandlers
	*/
	//@{
	void buttonPressEvent(XButtonEvent *be);
	void buttonReleaseEvent(XButtonEvent *be);
	void enterNotifyEvent(XCrossingEvent *ce);
	void leaveNotifyEvent(XCrossingEvent *ce);
	void exposeEvent(XExposeEvent *ee);
	void keyPressEvent(XKeyEvent *ke);
	//@}
	
	void redrawWindowLabel(bool redraw= false);
	void redrawWorkspaceLabel(bool redraw= false);
	void redrawPrevWorkspaceButton(bool pressed = false, bool redraw = false);
	void redrawNextWorkspaceButton(bool pressed = false, bool redraw = false);
	void redrawPrevWindowButton(bool pressed = false, bool redraw = false);
	void redrawNextWindowButton(bool pressed = false, bool redraw = false);
	/// enter edit mode on workspace label
	void edit();
	void reconfigure();

#ifdef HAVE_STRFTIME
	void checkClock(bool redraw = false);
#else // HAVE_STRFTIME
	void checkClock(bool redraw = false, bool date = false);
#endif // HAVE_STRFTIME

	virtual void timeout();

		
private:
	bool on_top;       ///< always on top
	bool editing;      ///< edit workspace label mode
	bool hidden;       ///< hidden state
	bool do_auto_hide; ///< do we auto hide	
	Display *display;  ///< display connection

	struct frame {
		unsigned long button_pixel, pbutton_pixel;
		Pixmap base, label, wlabel, clk, button, pbutton;
		Window window, workspace_label, window_label, clock, psbutton, nsbutton,
			pwbutton, nwbutton;

		int x, y, x_hidden, y_hidden, hour, minute, grab_x, grab_y;
		unsigned int width, height, window_label_w, workspace_label_w, clock_w,
			button_w, bevel_w, label_h;
	} frame;

	class HideHandler : public TimeoutHandler {
	public:
		Toolbar *toolbar;

		virtual void timeout();
	} hide_handler;

	BScreen *screen;
	BImageControl *image_ctrl; 
	BTimer clock_timer, hide_timer;
	Toolbarmenu *toolbarmenu;
	IconBar *iconbar;
	
	std::string new_workspace_name; ///< temp variable in edit workspace name mode

	friend class HideHandler;
	friend class Toolbarmenu;
	friend class Toolbarmenu::Placementmenu;
	#ifdef XINERAMA
	friend class Toolbarmenu::Headmenu;
	#endif // XINERAMA
};


#endif // TOOLBAR_HH
