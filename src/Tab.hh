// Tab.hh for Fluxbox Window Manager
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: Tab.hh,v 1.9 2002/01/27 12:52:02 fluxgen Exp $

#ifndef _TAB_HH_
#define _TAB_HH_

#ifndef _IMAGE_HH_
#include "Image.hh"
#endif

#ifndef _WINDOW_HH_
#include "Window.hh"
#endif

//Note: Tab is a friend of FluxboxWindow


class Tab {
public:
	enum Placement{ PTOP = 0, PBOTTOM = 5, PLEFT = 10, PRIGHT = 15, PNONE = 20};
	enum Alignment{ ALEFT = 0, ACENTER, ARIGHT, ARELATIVE, ANONE };

	Tab(FluxboxWindow *win, Tab *prev=0, Tab *next=0);
	~Tab();
	void draw(bool pressed);
	inline Tab *next() const { return m_next; }
	inline Tab *prev() const { return m_prev; }
	inline Tab *last() { return getLast(this); }
	inline Tab *first() { return getFirst(this); }
	
	inline FluxboxWindow *getWindow() const { return m_win; }
	inline unsigned int getTabWidth() const { return m_size_w; } 
	inline unsigned int getTabHeight() const { return m_size_h; }
	void focus();
	void decorate();
	void deiconify();
	void iconify();
	void raise();
	void lower();
	void withdraw();	
	void stick();
	void resize();
	void shade();
	//position tab to follow (FluxboxWindow *) m_win 
	void setPosition();	
	//event handlers
	void buttonReleaseEvent(XButtonEvent *be);
	void buttonPressEvent(XButtonEvent *be);
	void exposeEvent(XExposeEvent *ee);	
	void motionNotifyEvent(XMotionEvent *me);
	static Tab *getFirst(Tab *current);
	static Tab *getLast(Tab *current);
	void disconnect();

	static const char *getTabPlacementString(Tab::Placement placement);
	static Tab::Placement getTabPlacementNum(const char *string);
	static const char *getTabAlignmentString(Tab::Alignment alignment);
	static Tab::Alignment getTabAlignmentNum(const char *string);
	//TODO: do these have to be public?
	void resizeGroup(void); // used when (un)shading windows
	void calcIncrease(void);
	inline bool configured() { return m_configured; }
	inline void setConfigured(bool value) { m_configured = value; }
private:	
	bool m_configured;
	
	void insert(Tab *next);	
	//The size of the tab
	unsigned int m_size_w;
	unsigned int m_size_h;
	//Increasements
	int m_inc_x;
	int m_inc_y;
	static const int m_max_tabs; 
	bool m_focus, m_moving;  // moving and focus 
	void createTabWindow(); // creates the X win of tab
	void loadTheme(); // loads the textures with right width and height
	int setPositionShadingHelper(bool shaded);
	int setPositionTBAlignHelper(Alignment align);
	int setPositionLRAlignHelper(Alignment align);
	void setTabWidth(unsigned int w);
	void setTabHeight(unsigned int h);
	unsigned int calcRelativeWidth();
	unsigned int calcRelativeHeight();
	unsigned int calcCenterXPos();
	unsigned int calcCenterYPos();
	int m_move_x, m_move_y; // Move coordinates, holds moving coordinates when draging
	Tab *m_prev;	
	Tab *m_next; 
	FluxboxWindow *m_win; 
	Window m_tabwin; 
	Display *m_display; 
	Pixmap	m_focus_pm, m_unfocus_pm;
	unsigned long m_focus_pixel, m_unfocus_pixel;	
	static bool m_stoptabs; //used to "freeze" the tabs functions

	struct t_tabplacementlist{
		int tp;		
		const char *string;		
		inline bool operator == (int p) {
			return (tp==p);
		}
		inline bool operator == (const char *str) {
			if (strcasecmp(string, str)==0)
				return true;
			return false;
		}
	};
	static t_tabplacementlist m_tabplacementlist[];
	static t_tabplacementlist m_tabalignmentlist[];

};

#endif //_TAB_HH_
