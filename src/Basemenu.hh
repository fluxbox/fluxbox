// Basemenu.hh for Fluxbox Window manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Basemenu.hh for Blackbox - an X11 Window manager
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

// $Id: Basemenu.hh,v 1.11 2002/04/03 23:08:19 fluxgen Exp $

#ifndef	 BASEMENU_HH
#define	 BASEMENU_HH

#include <X11/Xlib.h>
#include <vector>
#include <string>

// forward declarations
class Basemenu;
class BasemenuItem;

class Fluxbox;
class BImageControl;
class BScreen;

class Basemenu {
public:
	explicit Basemenu(BScreen *);
	virtual ~Basemenu(void);

	inline const Bool &isTorn(void) const { return torn; }
	inline const Bool &isVisible(void) const { return visible; }

	inline BScreen *getScreen(void) const { return screen; }

	inline const Window &getWindowID(void) const { return menu.window; }

	inline const char *getLabel(void) const { return menu.label; }

	int insert(const char *, int = 0, const char * = 0, int = -1);

	int insert(const char *, Basemenu *, int = -1);
	int remove(unsigned int item);

	inline const int &getX(void) const { return menu.x; }
	inline const int &getY(void) const { return menu.y; }
	inline int getCount(void) { return menuitems.size(); }
	inline const int &getCurrentSubmenu(void) const { return which_sub; }

	inline const unsigned int &getWidth(void) const { return menu.width; }
	inline const unsigned int &getHeight(void) const { return menu.height; }
	inline const unsigned int &getTitleHeight(void) const { return menu.title_h; }

	inline void setInternalMenu(void) { internal_menu = True; }
	inline void setAlignment(int a) { alignment = a; }
	inline void setTorn(void) { torn = True; }
	inline void removeParent(void) { if (internal_menu) parent = (Basemenu *) 0; }

	bool hasSubmenu(unsigned int index);
	bool isItemSelected(unsigned int index);
	bool isItemEnabled(unsigned int index);

	void buttonPressEvent(XButtonEvent *);
	void buttonReleaseEvent(XButtonEvent *);
	void motionNotifyEvent(XMotionEvent *);
	void enterNotifyEvent(XCrossingEvent *);
	void leaveNotifyEvent(XCrossingEvent *);
	void exposeEvent(XExposeEvent *);
	void reconfigure(void);
	void setLabel(const char *n);
	void move(int, int);
	void update(void);
	void setItemSelected(unsigned int index, bool val);
	void setItemEnabled(unsigned int, bool val);

	virtual void drawSubmenu(unsigned int index);
	virtual void show(void);
	virtual void hide(void);

	enum { ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
	enum { RIGHT = 1, LEFT };
	enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };

private:
	typedef std::vector<BasemenuItem *> Menuitems;
	Menuitems menuitems;
	Fluxbox *fluxbox;
	Basemenu *parent;
	BImageControl *image_ctrl;
	BScreen *screen;

	Bool moving, visible, movable, torn, internal_menu, title_vis, shifted,
		hide_tree;
	Display *display;
	int which_sub, which_press, which_sbl, alignment;

	struct _menu {
		Pixmap frame_pixmap, title_pixmap, hilite_pixmap, sel_pixmap;
		Window window, frame, title;

		char *label;
		int x, y, x_move, y_move, x_shift, y_shift, sublevels, persub, minsub,
			grab_x, grab_y;
		unsigned int width, height, title_h, frame_h, item_w, item_h, bevel_w,
			bevel_h;
	} menu;


protected:
	inline BasemenuItem *find(unsigned int index) { return menuitems[index]; }
	inline void setTitleVisibility(bool b) { title_vis = b; }
	inline void setMovable(bool b) { movable = b; }
	inline void setHideTree(bool h) { hide_tree = h; }
	inline void setMinimumSublevels(int m) { menu.minsub = m; }

	virtual void itemSelected(int button, unsigned int index) = 0;
	virtual void drawItem(unsigned int index, bool highlight= false, bool clear= false,
			int x= -1, int y= -1, unsigned int width= 0, unsigned int height= 0);
	virtual void redrawTitle();
	virtual void internal_hide(void);
};

class BasemenuItem {
public:
	BasemenuItem(
			const char *label,
			int function,
			const char *exec = (const char *) 0)
		: m_label(label ? label : "")
		, m_exec(exec ? exec : "")
		, m_submenu(0)
		, m_function(function)
		, m_enabled(true)
		, m_selected(false)
	{ }

	BasemenuItem(const char *label, Basemenu *submenu)
		: m_label(label ? label : "")
		, m_exec("")
		, m_submenu(submenu)
		, m_function(0)
		, m_enabled(true)
		, m_selected(false)
	{ }

	inline const char *exec(void) const { return m_exec.c_str(); }
	inline const char *label(void) const { return m_label.c_str(); }
	inline int function(void) const { return m_function; }
	inline Basemenu *submenu(void) { return m_submenu; }

	inline bool isEnabled(void) const { return m_enabled; }
	inline void setEnabled(bool enabled) { m_enabled = enabled; }
	inline bool isSelected(void) const { return m_selected; }
	inline void setSelected(bool selected) { m_selected = selected; }

private:
	std::string m_label, m_exec;
	Basemenu *m_submenu;
	int m_function;
	bool m_enabled, m_selected;

	friend class Basemenu;
};


#endif // _BASEMENU_HH_
