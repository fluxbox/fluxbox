// Configmenu.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Configmenu.hh for Blackbox - An X11 Window Manager
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

// $Id: Configmenu.hh,v 1.5 2002/04/28 16:58:36 fluxgen Exp $

#ifndef	 CONFIGMENU_HH
#define	 CONFIGMENU_HH

// forward declaration
class Configmenu;

#include "Basemenu.hh"
#include "Screen.hh"
#include "fluxbox.hh"

class Configmenu : public Basemenu {
private:
	class Focusmenu : public Basemenu {
	public:
		Focusmenu(Configmenu *);
	protected:
		virtual void itemSelected(int button, unsigned int index);

	private:
		Configmenu *configmenu;

	};

	class Placementmenu : public Basemenu {
	public:
		Placementmenu(Configmenu *);
	
	protected:
		virtual void itemSelected(int button, unsigned int index);
		
	private:
		Configmenu *configmenu;
	};

	class Tabmenu : public Basemenu {
	public:
		Tabmenu(Configmenu *);
	protected:
		virtual void itemSelected(int button, unsigned int index);
	private:
		Configmenu *configmenu;
		void setSelected();	
	};

	BScreen *screen;
	Focusmenu *focusmenu;
	Placementmenu *placementmenu;
	Tabmenu *tabmenu;

	friend class Focusmenu;
	friend class Placementmenu;
	friend class Tabmenu;


protected:
	virtual void itemSelected(int button, unsigned int index);


public:
	Configmenu(BScreen *);
	virtual ~Configmenu();
	inline Basemenu *getFocusmenu() { return focusmenu; }
	inline Basemenu *getPlacementmenu() { return placementmenu; }
	inline Basemenu *getTabmenu() { return tabmenu; }
	
	inline const Basemenu *getFocusmenu() const { return focusmenu; }
	inline const Basemenu *getPlacementmenu() const { return placementmenu; }
	inline const Basemenu *getTabmenu() const { return tabmenu; }

	void reconfigure();
};


#endif // _CONFIGMENU_HH_
