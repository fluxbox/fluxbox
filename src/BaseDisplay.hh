// BaseDisplay.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// BaseDisplay.hh for Blackbox - an X11 Window manager
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

// $Id: BaseDisplay.hh,v 1.14 2002/03/18 15:28:25 fluxgen Exp $

#ifndef	 BASEDISPLAY_HH
#define	 BASEDISPLAY_HH

#include "Timer.hh"
#include "NotCopyable.hh"
#include "FbAtoms.hh"
#include <X11/Xlib.h>

#include <list>
#include <vector>

// forward declaration
class ScreenInfo;

#define PropBlackboxHintsElements		(5)
#define PropBlackboxAttributesElements	(8)

void bexec(const char *command, char* displaystring);

class BaseDisplay:private NotCopyable, public FbAtoms
{

public:
	BaseDisplay(char *, char * = 0);
	virtual ~BaseDisplay(void);
	
	enum Attrib {
		ATTRIB_SHADED = 0x01,
		ATTRIB_MAXHORIZ = 0x02,
		ATTRIB_MAXVERT = 0x04,
		ATTRIB_OMNIPRESENT = 0x08,
		ATTRIB_WORKSPACE = 0x10,
		ATTRIB_STACK = 0x20,		
		ATTRIB_DECORATION = 0x40
	};
	
	enum Decor {DECOR_NONE=0, DECOR_NORMAL, DECOR_TINY, DECOR_TOOL};
	
	
	typedef struct _blackbox_hints {
		unsigned long flags, attrib, workspace, stack;
		Decor decoration;
	} BlackboxHints;

	typedef struct _blackbox_attributes {
		unsigned long flags, attrib, workspace, stack;
		int premax_x, premax_y;
		unsigned int premax_w, premax_h;
	} BlackboxAttributes;


	inline ScreenInfo *getScreenInfo(int s)	{ return screenInfoList[s]; }

	inline const Bool &hasShapeExtensions(void) const { return shape.extensions; }
	inline const bool &doShutdown(void) const { return m_shutdown; }
	inline const bool &isStartup(void) const { return m_startup; }

	inline const Cursor &getSessionCursor(void) const { return cursor.session; }
	inline const Cursor &getMoveCursor(void) const { return cursor.move; }
	inline const Cursor &getLowerLeftAngleCursor(void) const { return cursor.ll_angle; }
	inline const Cursor &getLowerRightAngleCursor(void) const { return cursor.lr_angle; }

	inline Display *getXDisplay(void) { return m_display; }

	inline const char *getXDisplayName(void) const	{ return const_cast<const char *>(m_display_name); }
	inline const char *getApplicationName(void) const { return const_cast<const char *>(m_app_name); }

	inline const int &getNumberOfScreens(void) const { return number_of_screens; }
	inline const int &getShapeEventBase(void) const	{ return shape.event_basep; }

	inline void shutdown(void) { m_shutdown = true; }
	inline void run(void) { m_startup = m_shutdown = false; }

	const bool validateWindow(Window);

	void grab(void);
	void ungrab(void);
	void eventLoop(void);
	void addTimer(BTimer *);
	void removeTimer(BTimer *);

	// another pure virtual... this is used to handle signals that BaseDisplay
	// doesn't understand itself
	virtual Bool handleSignal(int) = 0;

	class GrabGuard:private NotCopyable
	{
		public:
		GrabGuard(BaseDisplay &bd):m_bd(bd) { }
		~GrabGuard() { m_bd.ungrab(); }
		inline void grab() { m_bd.grab(); }		
		inline void ungrab() { m_bd.ungrab(); }		
		private:
		BaseDisplay &m_bd;
	};

private:
	struct cursor {
		Cursor session, move, ll_angle, lr_angle;
	} cursor;

	struct shape {
		Bool extensions;
		int event_basep, error_basep;
	} shape;

	

	bool m_startup, m_shutdown;
	Display *m_display;

    typedef std::vector<ScreenInfo *> ScreenInfoList;
    ScreenInfoList screenInfoList;

    typedef std::list<BTimer *> TimerList;
    TimerList timerList;

	char *m_display_name, *m_app_name;
	int number_of_screens, m_server_grabs, colors_per_channel;

protected:
	
	virtual void process_event(XEvent *) = 0;

};


class ScreenInfo {
public:
	ScreenInfo(BaseDisplay *, int);

	inline BaseDisplay *getBaseDisplay(void) { return basedisplay; }

	inline Visual *getVisual(void) { return visual; }
	inline const Window &getRootWindow(void) const { return root_window; }
	inline const Colormap &getColormap(void) const { return colormap; }

	inline const int &getDepth(void) const { return depth; }
	inline const int &getScreenNumber(void) const { return screen_number; }

	inline const unsigned int &getWidth(void) const { return width; }
	inline const unsigned int &getHeight(void) const { return height; }

private:
	BaseDisplay *basedisplay;
	Visual *visual;
	Window root_window;
	Colormap colormap;

	int depth, screen_number;
	unsigned int width, height;


};


#endif // __BaseDisplay_hh
