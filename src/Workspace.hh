// Workspace.hh for Fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Workspace.hh for Blackbox - an X11 Window manager
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

#ifndef	 WORKSPACE_HH
#define	 WORKSPACE_HH

#include "Clientmenu.hh"
#include "Window.hh"
#include "NotCopyable.hh"

#include <X11/Xlib.h>
#include <string>
#include <vector>
#include <list>

class BScreen;


class Workspace:private NotCopyable {
public:
	typedef std::vector<FluxboxWindow *> Windows;
	
	Workspace(BScreen *screen, unsigned int workspaceid= 0);
	~Workspace();
	inline void setLastFocusedWindow(FluxboxWindow *w) { lastfocus = w; }
	void setName(const char *name);
	void showAll();
	void hideAll();
	void removeAll();
	void raiseWindow(FluxboxWindow *);
	void lowerWindow(FluxboxWindow *);
	void reconfigure();
	void update();
	void setCurrent();
	void shutdown();
	int addWindow(FluxboxWindow *window, bool place = false);
	int removeWindow(FluxboxWindow *);

	inline BScreen *getScreen() const { return screen; }	
	inline FluxboxWindow *getLastFocusedWindow(void) const { return lastfocus; }	
	inline Clientmenu *menu() { return &m_clientmenu; }
	inline const Clientmenu *menu() const { return &m_clientmenu; }
	inline const std::string &name() const { return m_name; }
	inline unsigned int workspaceID() const { return m_id; }	
	
	FluxboxWindow *getWindow(unsigned int id);
	const FluxboxWindow *getWindow(unsigned int id) const;
	inline const Windows &getWindowList() const { return windowList; }
	bool isCurrent() const;
	bool isLastWindow(FluxboxWindow *window) const;
	int getCount() const;

protected:
	void placeWindow(FluxboxWindow *);

private:
	BScreen *screen;
	FluxboxWindow *lastfocus;
	Clientmenu m_clientmenu;

	typedef std::list<FluxboxWindow *> WindowStack;
 

	WindowStack stackingList;
	Windows windowList;

	std::string m_name;
	unsigned int m_id;
	int cascade_x, cascade_y;
};


#endif // _WORKSPACE_HH_

