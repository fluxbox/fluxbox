// IconBar.hh
// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
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
#ifndef _ICONBAR_HH_
#define _ICONBAR_HH_

#include <vector>
#include "Window.hh"
#include "LinkedList.hh"

class Fluxbox;

class IconBarObj
{
public:	
	IconBarObj(FluxboxWindow *fluxboxwin, Window iconwin);
	~IconBarObj();
	inline Window getIconWin(void) { return m_iconwin; }
	inline FluxboxWindow *getFluxboxWin(void)  { return m_fluxboxwin; }
private:
	Window m_iconwin;
	FluxboxWindow *m_fluxboxwin;
};

typedef LinkedList<IconBarObj> IconList;
typedef LinkedListIterator<IconBarObj> IconListIterator;

class IconBar
{
public:
	IconBar(BScreen *scrn, Window parent);
	~IconBar();
	void draw();
	void reconfigure();
	Window addIcon(FluxboxWindow *fluxboxwin);
	Window delIcon(FluxboxWindow *fluxboxwin);
	void buttonPressEvent(XButtonEvent *be);	
	FluxboxWindow *findWindow(Window w);
	void exposeEvent(XExposeEvent *ee);
private:
	void draw(IconBarObj *obj, int width);
	void loadTheme(unsigned int width, unsigned int height);
	void decorate(Window win);
	IconBarObj *findIcon(FluxboxWindow *fluxboxwin);
	void repositionIcons(void);
	Window createIconWindow(FluxboxWindow *fluxboxwin, Window parent);
	BScreen *m_screen;
	Display *m_display;
	Window m_parent;
	IconList *m_iconlist;	
	Pixmap m_focus_pm;
	unsigned long m_focus_pixel;
};

#endif // _ICONBAR_HH_
