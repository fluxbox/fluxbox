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

// $Id: Configmenu.hh,v 1.6 2002/10/15 09:50:38 fluxgen Exp $

#ifndef	 CONFIGMENU_HH
#define	 CONFIGMENU_HH

#include "Basemenu.hh"

class BScreen;

class Configmenu : public Basemenu {
public:
	explicit Configmenu(BScreen *scr);
	virtual ~Configmenu();

	Basemenu &focusmenu() { return m_focusmenu; }
	Basemenu &getPlacementmenu() { return m_placementmenu; }
	Basemenu &getTabmenu() { return m_tabmenu; }
	
	const Basemenu &focusmenu() const { return m_focusmenu; }
	const Basemenu &placementmenu() const { return m_placementmenu; }
	const Basemenu &tabmenu() const { return m_tabmenu; }

	void reconfigure();

protected:
	virtual void itemSelected(int button, unsigned int index);

private:
	class Focusmenu : public Basemenu {
	public:
		explicit Focusmenu(BScreen *scr);
	protected:
		virtual void itemSelected(int button, unsigned int index);
	};

	class Placementmenu : public Basemenu {
	public:
		explicit Placementmenu(BScreen *scr);
	
	protected:
		virtual void itemSelected(int button, unsigned int index);
	};

	class Tabmenu : public Basemenu {
	public:
		explicit Tabmenu(BScreen *scr);
	protected:
		virtual void itemSelected(int button, unsigned int index);
	private:
		void setSelected();	
	};

	Focusmenu m_focusmenu;
	Placementmenu m_placementmenu;
	Tabmenu m_tabmenu;
};


#endif // _CONFIGMENU_HH_
