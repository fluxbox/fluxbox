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

#ifndef   WORKSPACE_HH
#define   WORKSPACE_HH

#include <X11/Xlib.h>
#include <string>
#include <vector>
#include <list>

class BScreen;
class Clientmenu;
class Workspace;
class FluxboxWindow;


class Workspace {
private:
  BScreen *screen;
  FluxboxWindow *lastfocus;
  Clientmenu *clientmenu;

  typedef std::list<FluxboxWindow *> WindowStack;
  typedef std::vector<FluxboxWindow *> Windows;

  WindowStack stackingList;
  Windows windowList;

  std::string name;
  int id, cascade_x, cascade_y;


protected:
  void placeWindow(FluxboxWindow *);


public:
  Workspace(BScreen *, int = 0);
  ~Workspace(void);

  inline BScreen *getScreen(void) { return screen; }

  inline FluxboxWindow *getLastFocusedWindow(void) { return lastfocus; }
  
  inline Clientmenu *getMenu(void) { return clientmenu; }

  inline const char *getName(void) const { return name.c_str(); }

  inline const int &getWorkspaceID(void) const { return id; }
  
  inline void setLastFocusedWindow(FluxboxWindow *w) { lastfocus = w; }

  FluxboxWindow *getWindow(int);

  bool isCurrent(void);
  bool isLastWindow(FluxboxWindow *);
  
  const int addWindow(FluxboxWindow *, Bool = False);
  const int removeWindow(FluxboxWindow *);
  const int getCount(void);

  void showAll(void);
  void hideAll(void);
  void removeAll(void);
  void raiseWindow(FluxboxWindow *);
  void lowerWindow(FluxboxWindow *);
  void reconfigure();
  void update();
  void setCurrent(void);
  void setName(char *);
  void shutdown(void);
};


#endif // _WORKSPACE_HH_

