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

#include "Basemenu.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <list>
#include <string>
#include <memory>

// forward declaration
class Slit;

class Slitmenu : public Basemenu {
public:
	explicit Slitmenu(Slit &theslist);
	virtual ~Slitmenu();

	const Basemenu &getDirectionmenu() const { return m_directionmenu; }
	const Basemenu &getPlacementmenu() const { return m_placementmenu; }

#ifdef XINERAMA
	const Basemenu *getHeadmenu() const { return m_headmenu.get(); }
#endif // XINERAMA

	void reconfigure();

protected:
	virtual void itemSelected(int button, unsigned int index);
	virtual void internal_hide();

private: 
	class Directionmenu : public Basemenu {
	public:
		Directionmenu(Slitmenu &sm);

	protected:
		virtual void itemSelected(int button, unsigned int index);

	private:
		Slitmenu &slitmenu;
	};

	class Placementmenu : public Basemenu {
	public:
		Placementmenu(Slitmenu &sm);

	protected: 
		virtual void itemSelected(int button, unsigned int index);

	private:
		Slitmenu &slitmenu;
	};

	Slit &slit;

#ifdef XINERAMA
	class Headmenu : public Basemenu {
	public:
		Headmenu(Slitmenu &sm);
	protected: 
		virtual void itemSelected(int button, unsigned int index);
	private:
		Slitmenu &slitmenu;
	};
	friend class Headmenu;
	std::auto_ptr<Headmenu> m_headmenu;
#endif // XINERAMA

	Placementmenu m_placementmenu;
	Directionmenu m_directionmenu;



	friend class Directionmenu;
	friend class Placementmenu;
};


class Slit : public TimeoutHandler {
public:
	explicit Slit(BScreen *screen);
	virtual ~Slit();

	inline bool isOnTop() const { return on_top; }
	inline bool isHidden() const { return hidden; }
	inline bool doAutoHide() const { return do_auto_hide; }

	Slitmenu &menu() { return slitmenu; }

	inline const Window &getWindowID() const { return frame.window; }

	inline int x() const { return ((hidden) ? frame.x_hidden : frame.x); }
	inline int y() const { return ((hidden) ? frame.y_hidden : frame.y); }

	inline unsigned int width() const { return frame.width; }
	inline unsigned int height() const { return frame.height; }
	void setOnTop(bool val);
	void setAutoHide(bool val);
	void addClient(Window clientwin);
	void removeClient(Window clientwin, bool = true);
	void reconfigure();
	void reposition();
	void shutdown();
	void saveClientList();
	BScreen *screen() { return m_screen; }
	const BScreen *screen() const { return m_screen; }
	/**
		@name eventhandlers
	*/
	//@{
	void buttonPressEvent(XButtonEvent *bp);
	void enterNotifyEvent(XCrossingEvent *en);
	void leaveNotifyEvent(XCrossingEvent *ln);
	void configureRequestEvent(XConfigureRequestEvent *cr);
	//@}
	
	virtual void timeout();

	/**
		Client alignment
	*/
	enum { VERTICAL = 1, HORIZONTAL };
	/**
		Screen placement
	*/
	enum { TOPLEFT = 1, CENTERLEFT, BOTTOMLEFT, TOPCENTER, BOTTOMCENTER,
				 TOPRIGHT, CENTERRIGHT, BOTTOMRIGHT };

private:
	class SlitClient {
	public:
		SlitClient(BScreen *screen, Window client_window);	// For adding an actual window
		SlitClient(const char *name);		// For adding a placeholder

		// Now we pre-initialize a list of slit clients with names for
		// comparison with incoming client windows.  This allows the slit
		// to maintain a sorted order based on a saved window name list.
		// Incoming windows not found in the list are appended.  Matching
		// duplicates are inserted after the last found instance of the
		// matching name.
		std::string match_name;

		Window window, client_window, icon_window;

		int x, y;
		unsigned int width, height;

		void initialize(BScreen *screen = 0, Window client_window= None);
	};
	
	void removeClient(SlitClient *client, bool remap, bool destroy);
	void loadClientList();
	
	bool on_top, hidden, do_auto_hide;

	BScreen *m_screen;
	BTimer timer;

	typedef std::list<SlitClient *> SlitClients;

	SlitClients clientList;
	Slitmenu slitmenu;
	std::string clientListPath;

	struct frame {
		Pixmap pixmap;
		Window window;

		int x, y, x_hidden, y_hidden;
		unsigned int width, height;
	} frame;

};


#endif // SLIT_HH
