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

// $Id: Window.hh,v 1.37 2002/11/23 16:07:19 rathnor Exp $

#ifndef	 WINDOW_HH
#define	 WINDOW_HH

#include "BaseDisplay.hh"
#include "Timer.hh"
#include "Windowmenu.hh"
#include "Subject.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <vector>
#include <string>
#include <memory>

#define PropMwmHintsElements	3

class Tab;


/**
	Creates the window frame and handles any window event for it
	TODO: this is to huge!
*/
class FluxboxWindow : public TimeoutHandler {
public:
	
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
	bool isTransient() const { return ((client.transient_for) ? true : false); }
	bool hasTransient() const { return ((client.transients.size()) ? true : false); }
	bool isManaged() const { return managed; }
	bool isFocused() const { return focused; }
	bool isVisible() const { return visible; }
	bool isIconic() const { return iconic; }
	bool isShaded() const { return shaded; }
	bool isMaximized() const { return maximized; }
	bool isIconifiable() const { return functions.iconify; }
	bool isMaximizable() const { return functions.maximize; }
	bool isResizable() const { return functions.resize; }
	bool isClosable() const { return functions.close; }
	bool isStuck() const { return stuck; }
	bool hasTitlebar() const { return decorations.titlebar; }
	bool hasTab() const { return (tab!=0 ? true : false); }
	bool isMoving() const { return moving; }
	bool isResizing() const { return resizing; }
	bool isGroupable() const;
	const BScreen *getScreen() const { return screen; }
	BScreen *getScreen() { return screen; }
	const Tab *getTab() const { return tab; }
	Tab *getTab() { return tab; }
	const std::list<FluxboxWindow *> &getTransients() const { return client.transients; } 
	std::list<FluxboxWindow *> &getTransients() { return client.transients; } 	
	const FluxboxWindow *getTransientFor() const { return client.transient_for; }
	FluxboxWindow *getTransientFor() { return client.transient_for; }
	
	const Window &getFrameWindow() const { return frame.window; }
	const Window &getClientWindow() const { return client.window; }

	Windowmenu *getWindowmenu() { return m_windowmenu.get(); }
	const Windowmenu *getWindowmenu() const { return m_windowmenu.get(); }
	
	const std::string &getTitle() const { return client.title; }
	const std::string &getIconTitle() const { return client.icon_title; }
	int getXFrame() const { return frame.x; }
	int getYFrame() const { return frame.y; }
	int getXClient() const { return client.x; }
	int getYClient() const { return client.y; }
	unsigned int getWorkspaceNumber() const { return workspace_number; }
	int getWindowNumber() const { return window_number; }
	WinLayer getLayer() const { return m_layer; }
	unsigned int getWidth() const { return frame.width; }
	unsigned int getHeight() const { return frame.height; }
	unsigned int getClientHeight() const { return client.height; }
	unsigned int getClientWidth() const { return client.width; }
	unsigned int getTitleHeight() const { return frame.title_h; }
	const std::string &className() const { return m_class_name; }
	const std::string &instanceName() const { return m_instance_name; }
	bool isLowerTab() const;
	/**
		@name signals
 	*/
	//@{
	FbTk::Subject &stateSig() { return m_statesig; }
	const FbTk::Subject &stateSig() const { return m_statesig; }
	FbTk::Subject &hintSig() { return m_hintsig; }
	const FbTk::Subject &hintSig() const { return m_hintsig; }
	FbTk::Subject &workspaceSig() { return m_workspacesig; }
	const FbTk::Subject &workspaceSig() const { return m_workspacesig; }
	//@}

	//@}

	void setWindowNumber(int n) { window_number = n; }
	
	const timeval &getLastFocusTime() const {return lastFocusTime;}

	bool validateClient();
	bool setInputFocus();
	void setTab(bool flag);
	void setFocusFlag(bool flag);
	void iconify();
	void deiconify(bool = true, bool = true);
	void close();
	void withdraw();
	void maximize(unsigned int);
	/// toggles shade
	void shade();
	/// toggles sticky
	void stick(); 
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
	/**
		@name event handlers
	*/
	//@{
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
	//@}

	void setDecoration(Decoration decoration);
	void toggleDecoration();
	
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

	class WinSubject: public FbTk::Subject {
	public:
		WinSubject(FluxboxWindow &w):m_win(w) { }
		FluxboxWindow &win() { return m_win; }
		const FluxboxWindow &win() const { return m_win; }
	private:
		FluxboxWindow &m_win;
	};

private:
	// state and hint signals
	WinSubject m_hintsig, m_statesig, m_workspacesig;

	BImageControl *image_ctrl; /// image control for rendering
	
	std::string m_instance_name; /// instance name from WM_CLASS
	std::string m_class_name; /// class name from WM_CLASS
	
	//Window state
	bool moving, resizing, shaded, maximized, visible, iconic, transient,
		focused, stuck, modal, send_focus_message, managed;

	BScreen *screen; /// screen on which this window exist
	BTimer timer;
	Display *display; /// display connection (obsolete by BaseDisplay singleton)
	BaseDisplay::BlackboxAttributes blackbox_attrib;

	Time lastButtonPressTime;
	std::auto_ptr<Windowmenu> m_windowmenu;

	timeval lastFocusTime;

	int focus_mode, window_number;
	unsigned int workspace_number;
	unsigned long current_state;
	WinLayer m_layer;
	Decoration old_decoration;

	struct _client {
		FluxboxWindow *transient_for; // which window are we a transient for?
		std::list<FluxboxWindow *>	transients;  // which windows are our transients?
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
			maximize, close, menu, sticky, shade, tab, enabled;
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

	Window findTitleButton(int type);	

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
	void setState(unsigned long stateval);
	void upsize();
	void downsize();
	void right_fixsize(int * = 0, int * = 0);
	void left_fixsize(int * = 0, int * = 0);
};


#endif // WINDOW_HH
