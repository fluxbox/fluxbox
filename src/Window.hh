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

// $Id: Window.hh,v 1.29 2002/08/30 14:06:40 fluxgen Exp $

#ifndef	 WINDOW_HH
#define	 WINDOW_HH

#include "BaseDisplay.hh"
#include "Timer.hh"
#include "Windowmenu.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <vector>
#include <string>

#define PropMwmHintsElements	3

class Tab;

/**
	Creates the window frame and handles any window event for it
	TODO: this is to huge!
*/
class FluxboxWindow : public TimeoutHandler {
public:
	/// obsolete
	enum Error{NOERROR=0, XGETWINDOWATTRIB, CANTFINDSCREEN};
	
#ifdef GNOME
	enum GnomeLayer { 
		WIN_LAYER_DESKTOP = 0,
		WIN_LAYER_BELOW = 2,
		WIN_LAYER_NORMAL = 4,
		WIN_LAYER_ONTOP = 6,
		WIN_LAYER_DOCK = 8,
		WIN_LAYER_ABOVE_DOCK = 10,
		WIN_LAYER_MENU = 12
	};

	enum GnomeState {
		WIN_STATE_STICKY          = (1<<0), // everyone knows sticky
		WIN_STATE_MINIMIZED       = (1<<1), // Reserved - definition is unclear
		WIN_STATE_MAXIMIZED_VERT  = (1<<2), // window in maximized V state
		WIN_STATE_MAXIMIZED_HORIZ = (1<<3), // window in maximized H state
		WIN_STATE_HIDDEN          = (1<<4), // not on taskbar but window visible
		WIN_STATE_SHADED          = (1<<5), // shaded (MacOS / Afterstep style)
		WIN_STATE_HID_WORKSPACE   = (1<<6), // not on current desktop
		WIN_STATE_HID_TRANSIENT   = (1<<7), // owner of transient is hidden
		WIN_STATE_FIXED_POSITION  = (1<<8), // window is fixed in position even
		WIN_STATE_ARRANGE_IGNORE  = (1<<9)  // ignore for auto arranging
	};

	enum GnomeHints {
		WIN_HINTS_SKIP_FOCUS      = (1<<0), // "alt-tab" skips this win
		WIN_HINTS_SKIP_WINLIST    = (1<<1), // do not show in window list
		WIN_HINTS_SKIP_TASKBAR    = (1<<2), // do not show on taskbar
		WIN_HINTS_GROUP_TRANSIENT = (1<<3), // Reserved - definition is unclear
		WIN_HINTS_FOCUS_ON_CLICK  = (1<<4)  // app only accepts focus if clicked
	};
#endif // GNOME
	
	enum WinLayer {
		LAYER_BOTTOM = 0x01, 
		LAYER_BELOW  = 0x02,
		LAYER_NORMAL = 0x04, 
		LAYER_TOP    = 0x08
	};

	enum Decoration {DECOR_NONE=0, DECOR_NORMAL, DECOR_TINY, DECOR_TOOL};

	//Motif wm Hints
	enum {
		MwmHintsFunctions   = (1l << 0),
		MwmHintsDecorations	= (1l << 1)
	};
	
	//Motif wm functions
	enum MwmFunc{
		MwmFuncAll          = (1l << 0),
		MwmFuncResize       = (1l << 1),
		MwmFuncMove         = (1l << 2),
		MwmFuncIconify      = (1l << 3),
		MwmFuncMaximize     = (1l << 4),
		MwmFuncClose        = (1l << 5)
	};
	//Motif wm decorations
	enum MwmDecor {
		MwmDecorAll         = (1l << 0),
		MwmDecorBorder      = (1l << 1),
		MwmDecorHandle      = (1l << 2),
		MwmDecorTitle       = (1l << 3),
		MwmDecorMenu        = (1l << 4),
		MwmDecorIconify     = (1l << 5),
		MwmDecorMaximize    = (1l << 6)
	};

	explicit FluxboxWindow(Window win, BScreen *scr = 0);
	virtual ~FluxboxWindow();
	/**
		@name accessors		
	*/
	//@{
	inline bool isTransient() const { return ((transient) ? true : false); }
	inline bool hasTransient() const { return ((client.transient) ? true : false); }
	inline bool isManaged() const { return managed; }
	inline bool isFocused() const { return focused; }
	inline bool isVisible() const { return visible; }
	inline bool isIconic() const { return iconic; }
	inline bool isShaded() const { return shaded; }
	inline bool isMaximized() const { return maximized; }
	inline bool isIconifiable() const { return functions.iconify; }
	inline bool isMaximizable() const { return functions.maximize; }
	inline bool isResizable() const { return functions.resize; }
	inline bool isClosable() const { return functions.close; }
	inline bool isStuck() const { return stuck; }
	inline bool hasTitlebar() const { return decorations.titlebar; }
	inline bool hasTab() const { return (tab!=0 ? true : false); }
	inline bool isMoving() const { return moving; }
	inline bool isResizing() const { return resizing; }
	inline const BScreen *getScreen() const { return screen; }
	inline BScreen *getScreen() { return screen; }
	inline const Tab *getTab() const { return tab; }
	inline Tab *getTab() { return tab; }
	inline const FluxboxWindow *getTransient() const { return client.transient; }
	inline FluxboxWindow *getTransient() { return client.transient; }	
	inline const FluxboxWindow *getTransientFor() const { return client.transient_for; }
	inline FluxboxWindow *getTransientFor() { return client.transient_for; }
	
	inline const Window &getFrameWindow() const { return frame.window; }
	inline const Window &getClientWindow() const { return client.window; }

	inline Windowmenu *getWindowmenu() { return windowmenu; }

	inline const std::string &getTitle() const { return client.title; }
	inline const std::string &getIconTitle() const { return client.icon_title; }
	inline int getXFrame() const { return frame.x; }
	inline int getYFrame() const { return frame.y; }
	inline int getXClient() const { return client.x; }
	inline int getYClient() const { return client.y; }
	inline unsigned int getWorkspaceNumber() const { return workspace_number; }
	inline int getWindowNumber() const { return window_number; }
	inline WinLayer getLayer() const { return m_layer; }
	inline unsigned int getWidth() const { return frame.width; }
	inline unsigned int getHeight() const { return frame.height; }
	inline unsigned int getClientHeight() const { return client.height; }
	inline unsigned int getClientWidth() const { return client.width; }
	inline unsigned int getTitleHeight() const { return frame.title_h; }
	const std::string className() const { return m_class_name; }
	const std::string instanceName() const { return m_instance_name; }
	bool isLowerTab() const;
	//@}

	inline void setWindowNumber(int n) { window_number = n; }
	
	inline const timeval &getLastFocusTime() const {return lastFocusTime;}

	bool validateClient();
	bool setInputFocus();
	void setTab(bool flag);
	void setFocusFlag(bool flag);
	void iconify();
	void deiconify(bool = true, bool = true);
	void close();
	void withdraw();
	void maximize(unsigned int);
	void shade();
	void stick();
	void unstick();
	void reconfigure();
	void installColormap(bool);
	void restore(bool remap);
	void configure(int dx, int dy, unsigned int dw, unsigned int dh);
	void setWorkspace(int n);
	void changeBlackboxHints(BaseDisplay::BlackboxHints *bh);
	void restoreAttributes();
	void showMenu(int mx, int my);	
	void pauseMoving();
	void resumeMoving();

	void buttonPressEvent(XButtonEvent *be);
	void buttonReleaseEvent(XButtonEvent *be);
	void motionNotifyEvent(XMotionEvent *me);
	bool destroyNotifyEvent(XDestroyWindowEvent *dwe);
	void mapRequestEvent(XMapRequestEvent *mre);
	void mapNotifyEvent(XMapEvent *mapev);
	bool unmapNotifyEvent(XUnmapEvent *unmapev);
	void propertyNotifyEvent(Atom a);
	void exposeEvent(XExposeEvent *ee);
	void configureRequestEvent(XConfigureRequestEvent *ce);
	
	void setDecoration(Decoration decoration);
	void toggleDecoration();

	static void showError(FluxboxWindow::Error error);
	
#ifdef SHAPE
	void shapeEvent(XShapeEvent *);
#endif // SHAPE

	virtual void timeout();
	
	// this structure only contains 3 elements... the Motif 2.0 structure contains
	// 5... we only need the first 3... so that is all we will define
	typedef struct MwmHints {
		unsigned long flags;       // Motif wm flags
		unsigned long functions;   // Motif wm functions
		unsigned long decorations; // Motif wm decorations
	} MwmHints;

#ifdef GNOME
	void setGnomeState(int state);
	inline int getGnomeHints() const { return gnome_hints; }
#endif //GNOME
	
private:
	
	BImageControl *image_ctrl; //image control for rendering
	
	// got from WM_CLASS
	std::string m_instance_name;
	std::string m_class_name;
	
	//Window state
	bool moving, resizing, shaded, maximized, visible, iconic, transient,
		focused, stuck, modal, send_focus_message, managed;

	BScreen *screen;
	BTimer timer;
	Display *display;
	BaseDisplay::BlackboxAttributes blackbox_attrib;

	Time lastButtonPressTime;
	Windowmenu *windowmenu;

	timeval lastFocusTime;

	int focus_mode, window_number;
	unsigned int workspace_number;
	unsigned long current_state;
	WinLayer m_layer;
	Decoration old_decoration;

	struct _client {
		FluxboxWindow *transient_for, // which window are we a transient for?
			*transient;  // which window is our transient?
		Window window, window_group;

		std::string title, icon_title;
		int x, y, old_bw;
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
	

	Tab *tab;
	friend class Tab; //TODO: Don't like long distant friendship
	
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
			y_border, y_handle, save_x, save_y;
		unsigned int width, height, title_h, label_w, label_h, handle_h,
			button_w, button_h, grip_w, grip_h, mwm_border_w, border_h,
			bevel_w, resize_w, resize_h, snap_w, snap_h, move_ws;
	} frame;

	enum { F_NOINPUT = 0, F_PASSIVE, F_LOCALLYACTIVE, F_GLOBALLYACTIVE };

	void grabButtons();
	
	void createButton(int type, ButtonEventProc, ButtonEventProc, ButtonDrawProc);
	void startMoving(Window win);
	void stopMoving();
	void startResizing(XMotionEvent *me, bool left); 
	void stopResizing(Window win=0);
	void updateIcon();
	
	// Decoration functions
	void createTitlebar();
	void destroyTitlebar();
	void createHandle();
	void destroyHandle();
	void checkTransient();

#ifdef GNOME
	
	void updateGnomeAtoms() const;
	void updateGnomeStateAtom() const;
	void updateGnomeHintsAtom() const;
	void updateGnomeLayerAtom() const;
	void updateGnomeWorkspaceAtom() const;
	
	void setGnomeLayer(int layer);

	int getGnomeWindowState() const;	
	bool handleGnomePropertyNotify(Atom atom);
	int getGnomeLayer() const;
	void loadGnomeAtoms();
	void loadGnomeStateAtom();
	void loadGnomeHintsAtom();
	void loadGnomeLayerAtom();

	int gnome_hints;
#endif //GNOME
	
#ifdef NEWWMSPEC
	
	void updateNETWMAtoms();
	void handleNETWMProperyNotify(Atom atom);
	int getNETWMWindowState();

#endif //NEWWMSPEC

	Window findTitleButton(int type);	
private:
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
	
	bool getState();
	Window createToplevelWindow(int, int, unsigned int, unsigned int,
						unsigned int);
	Window createChildWindow(Window, Cursor = None);

	void getWMName();
	void getWMIconName();
	void getWMNormalHints();
	void getWMProtocols();
	void getWMHints();
	void getMWMHints();
	void getBlackboxHints();
	void setNetWMAttributes();
	void associateClientWindow();
	void decorate();
	void decorateLabel();
	void positionButtons(bool redecorate_label = false);
	void positionWindows();
	
	void redrawLabel();
	void redrawAllButtons();
 
	void restoreGravity();
	void setGravityOffsets();
	void setState(unsigned long);
	void upsize();
	void downsize();
	void right_fixsize(int * = 0, int * = 0);
	void left_fixsize(int * = 0, int * = 0);
};


#endif // WINDOW_HH
