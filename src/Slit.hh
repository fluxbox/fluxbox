// Slit.hh for Fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Slit.hh for Blackbox - an X11 Window manager
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
	
#ifndef	 SLIT_HH
#define	 SLIT_HH

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// forward declaration
class Slit;
class Slitmenu;

#include "Basemenu.hh"

#include <list>

class Slitmenu : public Basemenu {
public:
	explicit Slitmenu(Slit &theslist);
	virtual ~Slitmenu(void);

	inline Basemenu *getDirectionmenu(void) { return directionmenu; }
	inline Basemenu *getPlacementmenu(void) { return placementmenu; }
#ifdef XINERAMA
	inline Basemenu *getHeadmenu(void) { return headmenu; }
#endif // XINERAMA

	void reconfigure(void);

private: 
	class Directionmenu : public Basemenu {
	private:
		Slitmenu *slitmenu;

	protected:
		virtual void itemSelected(int button, unsigned int index);

	public:
		Directionmenu(Slitmenu *);
	};

	class Placementmenu : public Basemenu {
	private:
		Slitmenu *slitmenu;

	protected: 
		virtual void itemSelected(int button, unsigned int index);

	public:
		Placementmenu(Slitmenu *);
	};

#ifdef XINERAMA
	class Headmenu : public Basemenu {
	public:
		Headmenu(Slitmenu *);
	private:
		Slitmenu *slitmenu;

	protected: 
		virtual void itemSelected(int button, unsigned int index);

	};
#endif // XINERAMA

	Directionmenu *directionmenu;
	Placementmenu *placementmenu;
#ifdef XINERAMA
	Headmenu *headmenu;
#endif // XINERAMA
	Slit &slit;

	friend class Directionmenu;
	friend class Placementmenu;
#ifdef XINERAMA
	friend class Headmenu;
#endif // XINERAMA
	friend class Slit;


protected:
	virtual void itemSelected(int button, unsigned int index);
	virtual void internal_hide(void);
};


class Slit : public TimeoutHandler {
public:
	explicit Slit(BScreen *);
	virtual ~Slit();

	inline bool isOnTop(void) const { return on_top; }
	inline bool isHidden(void) const { return hidden; }
	inline bool doAutoHide(void) const { return do_auto_hide; }

	Slitmenu &getMenu() { return slitmenu; }

	inline const Window &getWindowID() const { return frame.window; }

	inline int getX() const { return ((hidden) ? frame.x_hidden : frame.x); }
	inline int getY() const { return ((hidden) ? frame.y_hidden : frame.y); }

	inline unsigned int getWidth(void) const { return frame.width; }
	inline unsigned int getHeight(void) const { return frame.height; }

	void addClient(Window);
	void removeClient(Window, bool = true);
	void reconfigure(void);
	void reposition(void);
	void shutdown(void);

	void buttonPressEvent(XButtonEvent *);
	void enterNotifyEvent(XCrossingEvent *);
	void leaveNotifyEvent(XCrossingEvent *);
	void configureRequestEvent(XConfigureRequestEvent *);

	virtual void timeout(void);

	enum { VERTICAL = 1, HORIZONTAL };
	enum { TOPLEFT = 1, CENTERLEFT, BOTTOMLEFT, TOPCENTER, BOTTOMCENTER,
				 TOPRIGHT, CENTERRIGHT, BOTTOMRIGHT };

private:
	class SlitClient {
	public:
		Window window, client_window, icon_window;

		int x, y;
		unsigned int width, height;
	};
	
	void removeClient(SlitClient *, bool = true);
	
	Bool on_top, hidden, do_auto_hide;
	Display *display;

	Fluxbox *fluxbox;
	BScreen *screen;
	BTimer timer;

	typedef std::list<SlitClient *> SlitClients;

	SlitClients clientList;
	Slitmenu slitmenu;

	struct frame {
		Pixmap pixmap;
		Window window;

		int x, y, x_hidden, y_hidden;
		unsigned int width, height;
	} frame;

	friend class Slitmenu;
	friend class Slitmenu::Directionmenu;
	friend class Slitmenu::Placementmenu;
	#ifdef XINERAMA
	friend class Slitmenu::Headmenu;
	#endif // XINERAMA
};


#endif // __Slit_hh
