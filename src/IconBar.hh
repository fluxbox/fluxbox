// IconBar.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: IconBar.hh,v 1.5 2002/02/04 22:43:15 fluxgen Exp $

#ifndef _ICONBAR_HH_
#define _ICONBAR_HH_

#include <vector>
#include "Window.hh"

#include <list>

class IconBarObj
{
public:	
	IconBarObj(FluxboxWindow *fluxboxwin, Window iconwin);
	~IconBarObj();
	inline Window getIconWin(void) { return m_iconwin; }
	inline FluxboxWindow *getFluxboxWin(void)  { return m_fluxboxwin; }
	unsigned int getWidth(void);
private:
	FluxboxWindow *m_fluxboxwin;
	Window m_iconwin;
};

class IconBar
{
public:
	IconBar(BScreen *scrn, Window parent);
	~IconBar();
	void draw(); //TODO
	void reconfigure();
	Window addIcon(FluxboxWindow *fluxboxwin);
	Window delIcon(FluxboxWindow *fluxboxwin);
	void buttonPressEvent(XButtonEvent *be);	
	FluxboxWindow *findWindow(Window w);
	IconBarObj *findIcon(FluxboxWindow *fluxboxwin);
	void exposeEvent(XExposeEvent *ee);

	void draw(IconBarObj *obj, int width);
private:
	typedef std::list<IconBarObj *> IconList;

//	void draw(IconBarObj *obj, int width);
	void loadTheme(unsigned int width, unsigned int height);
	void decorate(Window win);
//	IconBarObj *findIcon(FluxboxWindow *fluxboxwin);
	void repositionIcons(void);
	Window createIconWindow(FluxboxWindow *fluxboxwin, Window parent);
	BScreen *m_screen;
	Display *m_display;
	Window m_parent;
	IconList m_iconlist;	
	Pixmap m_focus_pm;
	unsigned long m_focus_pixel;
};

#endif // _ICONBAR_HH_
