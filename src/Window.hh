// Window.hh for Fluxbox Window Manager
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org) 
//
// Window.hh for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Window.hh,v 1.9 2002/02/16 11:26:22 fluxgen Exp $

#ifndef	 _WINDOW_HH_
#define	 _WINDOW_HH_

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef		SHAPE
#	include <X11/extensions/shape.h>
#endif // SHAPE

// forward declaration
class FluxboxWindow;
class Tab;

#include "fluxbox.hh"

#include <vector>
#include <string>
#ifndef _BASEDISPLAY_HH_
#include "BaseDisplay.hh"
#endif 
#ifndef _TIMER_HH_
#include "Timer.hh"
#endif
#ifndef _WINDOWMENU_HH_
#include "Windowmenu.hh"
#endif

#define MwmHintsFunctions	(1l << 0)
#define MwmHintsDecorations	(1l << 1)

#define MwmFuncAll			(1l << 0)
#define MwmFuncResize		(1l << 1)
#define MwmFuncMove			(1l << 2)
#define MwmFuncIconify		(1l << 3)
#define MwmFuncMaximize		(1l << 4)
#define MwmFuncClose		(1l << 5)

#define MwmDecorAll			(1l << 0)
#define MwmDecorBorder		(1l << 1)
#define MwmDecorHandle		(1l << 2)
#define MwmDecorTitle		(1l << 3)
#define MwmDecorMenu		(1l << 4)
#define MwmDecorIconify		(1l << 5)
#define MwmDecorMaximize	(1l << 6)
//names for buttons
#define NAME_STICKY		"sticky"
#define NAME_MAXIMIZE	"maximize"
#define NAME_MINIMIZE	"minimize"
#define NAME_SHADE		"shade"
#define NAME_CLOSE		"close"
#define NAME_ICONIFY	"iconify"
#define NAME_MENU		"menu"
#define NAME_NONE		"none"


#define PropMwmHintsElements	3

class FluxboxWindow : public TimeoutHandler {
public:
	enum Error{NOERROR=0, XGETWINDOWATTRIB, CANTFINDSCREEN};
	FluxboxWindow(Window, BScreen * = (BScreen *) 0);
	virtual ~FluxboxWindow(void);

	inline const bool isTransient(void) const
		{ return ((transient) ? true : false); }
	inline const bool hasTransient(void) const
		{ return ((client.transient) ? true : false); }
	inline const bool isManaged() const { return managed; }
	inline const bool &isFocused(void) const { return focused; }
	inline const bool &isVisible(void) const { return visible; }
	inline const bool &isIconic(void) const { return iconic; }
	inline const bool &isShaded(void) const { return shaded; }
	inline const bool &isMaximized(void) const { return maximized; }
	inline const bool &isIconifiable(void) const { return functions.iconify; }
	inline const bool &isMaximizable(void) const { return functions.maximize; }
	inline const bool &isResizable(void) const { return functions.resize; }
	inline const bool &isClosable(void) const { return functions.close; }
	inline const bool &isStuck(void) const { return stuck; }
	inline const bool &hasTitlebar(void) const { return decorations.titlebar; }
	inline const bool hasTab(void) const { return (tab!=0 ? true : false); }
	static void showError(FluxboxWindow::Error error);
	inline BScreen *getScreen(void) { return screen; }
	inline Tab *getTab(void) { return tab; }
	inline FluxboxWindow *getTransient(void) { return client.transient; }
	inline FluxboxWindow *getTransientFor(void) { return client.transient_for; }

	inline const Window &getFrameWindow(void) const { return frame.window; }
	inline const Window &getClientWindow(void) const { return client.window; }

	inline Windowmenu *getWindowmenu(void) { return windowmenu; }

	inline char **getTitle(void) { return &client.title; }
	inline char **getIconTitle(void) { return &client.icon_title; }
	inline const int &getXFrame(void) const { return frame.x; }
	inline const int &getYFrame(void) const { return frame.y; }
	inline const int &getXClient(void) const { return client.x; }
	inline const int &getYClient(void) const { return client.y; }
	inline const int &getWorkspaceNumber(void) const { return workspace_number; }
	inline const int &getWindowNumber(void) const { return window_number; }

	inline const unsigned int &getWidth(void) const { return frame.width; }
	inline const unsigned int &getHeight(void) const { return frame.height; }
	inline const unsigned int &getClientHeight(void) const
	{ return client.height; }
	inline const unsigned int &getClientWidth(void) const
	{ return client.width; }
	inline const unsigned int &getTitleHeight(void) const
	{ return frame.title_h; }

	inline void setWindowNumber(int n) { window_number = n; }
	
	bool validateClient(void);
	bool setInputFocus(void);
	void setTab(bool flag);
	void setFocusFlag(bool);
	void iconify(void);
	void deiconify(bool = true, bool = true);
	void close(void);
	void withdraw(void);
	void maximize(unsigned int);
	void shade(void);
	void stick(void);
	void unstick(void);
	void reconfigure(void);
	void installColormap(bool);
	void restore(void);
	void configure(int dx, int dy, unsigned int dw, unsigned int dh);
	void setWorkspace(int n);
	void changeBlackboxHints(BaseDisplay::BlackboxHints *);
	void restoreAttributes(void);

	void buttonPressEvent(XButtonEvent *);
	void buttonReleaseEvent(XButtonEvent *);
	void motionNotifyEvent(XMotionEvent *);
	bool destroyNotifyEvent(XDestroyWindowEvent *);
	void mapRequestEvent(XMapRequestEvent *);
	void mapNotifyEvent(XMapEvent *);
	bool unmapNotifyEvent(XUnmapEvent *);
	void propertyNotifyEvent(Atom);
	void exposeEvent(XExposeEvent *);
	void configureRequestEvent(XConfigureRequestEvent *);

	#ifdef SHAPE
	void shapeEvent(XShapeEvent *);
	#endif // SHAPE

	virtual void timeout(void);
	
	// this structure only contains 3 elements... the Motif 2.0 structure contains
	// 5... we only need the first 3... so that is all we will define
	typedef struct MwmHints {
		unsigned long flags, functions, decorations;
	} MwmHints;

private:
	BImageControl *image_ctrl;
	
	bool moving, resizing, shaded, maximized, visible, iconic, transient,
		focused, stuck, modal, send_focus_message, managed;
	BScreen *screen;
	BTimer *timer;
	Display *display;
	BaseDisplay::BlackboxAttributes blackbox_attrib;

	Time lastButtonPressTime;
	Windowmenu *windowmenu;

	int focus_mode, window_number, workspace_number;
	unsigned long current_state;

	struct _client {
		FluxboxWindow *transient_for, // which window are we a transient for?
			*transient;  // which window is our transient?
		Window window, window_group;

		char *title, *icon_title;
		int x, y, old_bw, title_len;
		unsigned int width, height, title_text_w,
			min_width, min_height, max_width, max_height, width_inc, height_inc,
			min_aspect_x, min_aspect_y, max_aspect_x, max_aspect_y,
			base_width, base_height, win_gravity;
		unsigned long initial_state, normal_hint_flags, wm_hint_flags;

		MwmHints *mwm_hint;
		BaseDisplay::BlackboxHints *blackbox_hint;
	} client;

	struct _decorations {
		bool titlebar, handle, border, iconify,
				maximize, close, menu, sticky, shade, tab;
	} decorations;

	struct _functions {
		bool resize, move, iconify, maximize, close;
	} functions;
	
	bool usetab;
	Tab *tab;
	friend class Tab;
	
	typedef void (*ButtonDrawProc)(FluxboxWindow *, Window, bool);
	typedef void (*ButtonEventProc)(FluxboxWindow *, XButtonEvent *);

	struct Button {
		int type;
		Window win;
		bool used;
		ButtonEventProc pressed;
		ButtonEventProc released;
		ButtonDrawProc draw;		
	};
	
	std::vector<Button> buttonlist;
	
	struct _frame {
		//different bool because of XShapeQueryExtension
		Bool shaped;
		unsigned long ulabel_pixel, flabel_pixel, utitle_pixel,
			ftitle_pixel, uhandle_pixel, fhandle_pixel, ubutton_pixel,
			fbutton_pixel, pbutton_pixel, uborder_pixel, fborder_pixel,
			ugrip_pixel, fgrip_pixel;
		Pixmap ulabel, flabel, utitle, ftitle, uhandle, fhandle,
			ubutton, fbutton, pbutton, ugrip, fgrip;

		Window window, plate, title, label, handle, 
		right_grip, left_grip;

		int x, y, resize_x, resize_y, move_x, move_y, grab_x, grab_y,
			y_border, y_handle;
		unsigned int width, height, title_h, label_w, label_h, handle_h,
			button_w, button_h, grip_w, grip_h, mwm_border_w, border_h,
			bevel_w, resize_w, resize_h, snap_w, snap_h;
	} frame;

	enum { F_NOINPUT = 0, F_PASSIVE, F_LOCALLYACTIVE, F_GLOBALLYACTIVE };

	void grabButtons();
	
	void createButton(int type, ButtonEventProc, ButtonEventProc, ButtonDrawProc);
	#ifdef GNOME
	void updateGnomeAtoms();
	long getGnomeWindowState();
	#endif

	Window findTitleButton(int type);	
protected:
	//event callbacks
	static void stickyButton_cb(FluxboxWindow *, XButtonEvent *);
	static void stickyPressed_cb(FluxboxWindow *, XButtonEvent *);
	static void iconifyButton_cb(FluxboxWindow *, XButtonEvent *);
	static void iconifyPressed_cb(FluxboxWindow *, XButtonEvent *);
	static void maximizeButton_cb(FluxboxWindow *, XButtonEvent *);
	static void maximizePressed_cb(FluxboxWindow *, XButtonEvent *);
	static void closeButton_cb(FluxboxWindow *, XButtonEvent *);
	static void closePressed_cb(FluxboxWindow *, XButtonEvent *);
	static void shadeButton_cb(FluxboxWindow *, XButtonEvent *);
	//draw callbacks
	static void stickyDraw_cb(FluxboxWindow *, Window, bool);
	static void iconifyDraw_cb(FluxboxWindow *, Window, bool);
	static void maximizeDraw_cb(FluxboxWindow *, Window, bool);
	static void closeDraw_cb(FluxboxWindow *, Window, bool);
	static void shadeDraw_cb(FluxboxWindow *, Window, bool);
	
	static void grabButton(Display *display, unsigned int button, Window window, Cursor cursor);
	//button base draw... background 
	void drawButtonBase(Window, bool);
	
	bool getState(void);
	Window createToplevelWindow(int, int, unsigned int, unsigned int,
						unsigned int);
	Window createChildWindow(Window, Cursor = None);

	void getWMName(void);
	void getWMIconName(void);
	void getWMNormalHints(void);
	void getWMProtocols(void);
	void getWMHints(void);
	void getMWMHints(void);
	void getBlackboxHints(void);
	void setNetWMAttributes(void);
	void associateClientWindow(void);
	void decorate(void);
	void decorateLabel(void);
	void positionButtons(bool redecorate_label = false);
	void positionWindows(void);
	
	void redrawLabel(void);
	void redrawAllButtons(void);
 
	void restoreGravity(void);
	void setGravityOffsets(void);
	void setState(unsigned long);
	void upsize(void);
	void downsize(void);
	void right_fixsize(int * = 0, int * = 0);
	void left_fixsize(int * = 0, int * = 0);


};


#endif // __Window_hh
