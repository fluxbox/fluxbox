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

#ifndef   _CONFIGMENU_HH_
#define   _CONFIGMENU_HH_

// forward declaration
class Configmenu;

#include "Basemenu.hh"
#include "Screen.hh"
#include "fluxbox.hh"


class Configmenu : public Basemenu {
private:
  class Focusmenu : public Basemenu {
  private:
    Configmenu *configmenu;

  protected:
    virtual void itemSelected(int, int);

  public:
    Focusmenu(Configmenu *);
  };

  class Placementmenu : public Basemenu {
  private:
    Configmenu *configmenu;

  protected:
    virtual void itemSelected(int, int);

  public:
    Placementmenu(Configmenu *);
  };

  class Tabmenu : public Basemenu {
  private:
    Configmenu *configmenu;
		void setSelected(void);
  protected:
    virtual void itemSelected(int, int);

  public:
    Tabmenu(Configmenu *);
  };

//  Fluxbox *fluxbox;
  BScreen *screen;
  Focusmenu *focusmenu;
  Placementmenu *placementmenu;
  Tabmenu *tabmenu;

  friend class Focusmenu;
  friend class Placementmenu;
  friend class Tabmenu;


protected:
  virtual void itemSelected(int, int);


public:
  Configmenu(BScreen *);
  virtual ~Configmenu(void);

  inline Basemenu *getFocusmenu(void) { return focusmenu; }
  inline Basemenu *getPlacementmenu(void) { return placementmenu; }
  inline Basemenu *getTabmenu(void) { return tabmenu; }

  void reconfigure(void);
};


#endif // _CONFIGMENU_HH_
