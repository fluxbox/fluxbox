// Windowmenu.hh for Blackbox - an X11 Window manager
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

#ifndef   WINDOWMENU_HH
#define   WINDOWMENU_HH

// forward declaration
class Windowmenu;
class SendtoWorkspaceMenu;

class Fluxbox;
class FluxboxWindow;
class Toolbar;

#include "Basemenu.hh"


class Windowmenu : public Basemenu {
private:
  FluxboxWindow *window;
  BScreen *screen;

 
	class SendtoWorkspacemenu : public Basemenu {
  private:
    Windowmenu *windowmenu;

  protected:
    virtual void itemSelected(int, int);

  public:
    SendtoWorkspacemenu(Windowmenu *);
		inline Windowmenu *getWindowMenu() const { return windowmenu; }
    void update(void);

    virtual void show(void);
  };
 	
	class SendGroupToWorkspacemenu : public SendtoWorkspacemenu {	
	protected:	
		virtual void itemSelected(int, int);
	public:
		SendGroupToWorkspacemenu(Windowmenu *);
				
	};
	
  SendtoWorkspacemenu *sendToMenu;
	SendGroupToWorkspacemenu *sendGroupToMenu;
	
  friend class SendtoWorkspacemenu;
	friend class SendGroupToWorkspacemenu;

protected:
  virtual void itemSelected(int, int);


public:
  Windowmenu(FluxboxWindow *);
  virtual ~Windowmenu(void);

  inline Basemenu *getSendToMenu(void) { return static_cast<Basemenu *>(sendToMenu); }
	inline Basemenu *getSendGroupToMenu(void) { return static_cast<Basemenu *>(sendGroupToMenu); }
  void reconfigure(void);
  void setClosable(void);

  virtual void show(void);
};


#endif // __Windowmenu_hh
