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

// $Id: Toolbar.hh,v 1.10 2002/04/03 12:08:54 fluxgen Exp $

#ifndef	 TOOLBAR_HH
#define	 TOOLBAR_HH

#include <X11/Xlib.h>
#include "Basemenu.hh"
#include "Timer.hh"
#include "IconBar.hh"

// forward declaration
class Toolbar;

class Toolbarmenu : public Basemenu {
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
	#endif // XINERAMA
 

	Toolbar *toolbar;
	Placementmenu *placementmenu;
	#ifdef XINERAMA
	Headmenu *headmenu;
	friend class Headmenu;
	#endif // XINERAMA

	friend class Placementmenu;
	friend class Toolbar;


protected:
	virtual void itemSelected(int button, unsigned int index);
	virtual void internal_hide(void);

public:
	Toolbarmenu(Toolbar *);
	~Toolbarmenu(void);
	#ifdef XINERAMA
	inline Basemenu *getHeadmenu(void) { return headmenu; }
	#endif // XINERAMA

	inline Basemenu *getPlacementmenu(void) { return placementmenu; }

	void reconfigure(void);
};


class Toolbar : public TimeoutHandler {
private:
	Bool on_top, editing, hidden, do_auto_hide;
	Display *display;

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

		virtual void timeout(void);
	} hide_handler;

	Fluxbox *fluxbox;
	BScreen *screen;
	BImageControl *image_ctrl; 
	BTimer clock_timer, *hide_timer;
	Toolbarmenu *toolbarmenu;
	class IconBar *iconbar;
	
	std::string new_workspace_name;

	friend class HideHandler;
	friend class Toolbarmenu;
	friend class Toolbarmenu::Placementmenu;
	#ifdef XINERAMA
	friend class Toolbarmenu::Headmenu;
	#endif // XINERAMA


public:
	Toolbar(BScreen *);
	virtual ~Toolbar(void);
	void addIcon(FluxboxWindow *w);
	void delIcon(FluxboxWindow *w);
	
	inline Toolbarmenu *getMenu(void) { return toolbarmenu; }
	//inline Window getWindowLabel(void) { return frame.window_label; }
	inline const Bool &isEditing(void) const { return editing; }
	inline const Bool &isOnTop(void) const { return on_top; }
	inline const Bool &isHidden(void) const { return hidden; }
	inline const Bool &doAutoHide(void) const { return do_auto_hide; }

	inline const Window &getWindowID(void) const { return frame.window; }

	inline const unsigned int &getWidth(void) const { return frame.width; }
	inline const unsigned int &getHeight(void) const { return frame.height; }
	inline const unsigned int &getExposedHeight(void) const
	{ return ((do_auto_hide) ? frame.bevel_w : frame.height); }
	inline const int &getX(void) const
	{ return ((hidden) ? frame.x_hidden : frame.x); }
	inline const int &getY(void) const
	{ return ((hidden) ? frame.y_hidden : frame.y); }
	inline IconBar *getIconBar(void) { return iconbar; }

	void buttonPressEvent(XButtonEvent *);
	void buttonReleaseEvent(XButtonEvent *);
	void enterNotifyEvent(XCrossingEvent *);
	void leaveNotifyEvent(XCrossingEvent *);
	void exposeEvent(XExposeEvent *);
	void keyPressEvent(XKeyEvent *);

	void redrawWindowLabel(Bool = False);
	void redrawWorkspaceLabel(Bool = False);
	void redrawPrevWorkspaceButton(Bool = False, Bool = False);
	void redrawNextWorkspaceButton(Bool = False, Bool = False);
	void redrawPrevWindowButton(Bool = False, Bool = False);
	void redrawNextWindowButton(Bool = False, Bool = False);
	void edit(void);
	void reconfigure(void);

#ifdef		HAVE_STRFTIME
	void checkClock(Bool = False);
#else //	HAVE_STRFTIME
	void checkClock(Bool = False, Bool = False);
#endif // HAVE_STRFTIME

	virtual void timeout(void);

	enum Placement{ TOPLEFT = 1, BOTTOMLEFT, TOPCENTER,
		BOTTOMCENTER, TOPRIGHT, BOTTOMRIGHT };
};


#endif // __Toolbar_hh
