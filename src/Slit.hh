// Slit.hh for Fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Slit.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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
	
/// $Id: Slit.hh,v 1.21 2003/04/16 13:43:44 rathnor Exp $

#ifndef	 SLIT_HH
#define	 SLIT_HH

#include "Menu.hh"
#include "FbWindow.hh"
#include "Timer.hh"
#include "XLayerItem.hh"
#include "LayerMenu.hh"
#include "fluxbox.hh"
#include "Screen.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <list>
#include <string>
#include <memory>

class SlitClient;

/// Handles dock apps
class Slit : public FbTk::TimeoutHandler, public FbTk::EventHandler {
public:
    
    /**
       Client alignment
    */
    enum Direction { VERTICAL = 1, HORIZONTAL };
    /**
       Screen placement
    */
    enum Placement { TOPLEFT = 1, CENTERLEFT, BOTTOMLEFT, TOPCENTER, BOTTOMCENTER,
           TOPRIGHT, CENTERRIGHT, BOTTOMRIGHT };

    explicit Slit(BScreen &screen, FbTk::XLayer &layer, const char *filename = 0);
    virtual ~Slit();

    inline bool isHidden() const { return hidden; }
    inline bool doAutoHide() const { return do_auto_hide; }
    inline Direction direction() const { return m_direction; }
    inline Placement placement() const { return m_placement; }
    FbTk::Menu &menu() { return slitmenu; }

    inline Window getWindowID() const { return frame.window.window(); }

    inline int x() const { return ((hidden) ? frame.x_hidden : frame.x); }
    inline int y() const { return ((hidden) ? frame.y_hidden : frame.y); }

    inline unsigned int width() const { return frame.width; }
    inline unsigned int height() const { return frame.height; }

    void setDirection(Direction dir);
    void setPlacement(Placement place);
    void setAutoHide(bool val);
    void addClient(Window clientwin);
    void removeClient(Window clientwin, bool = true);
    void reconfigure();
    void reposition();
    void shutdown();
    /// save clients name in a file
    void saveClientList();
    /// cycle slit clients up one step
    void cycleClientsUp();
    /// cycle slit clients down one step
    void cycleClientsDown();

    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }
    /**
       @name eventhandlers
    */
    //@{
    void handleEvent(XEvent &event);
    void buttonPressEvent(XButtonEvent &event);
    void enterNotifyEvent(XCrossingEvent &event);
    void leaveNotifyEvent(XCrossingEvent &event);
    void configureRequestEvent(XConfigureRequestEvent &event);
    //@}
	
    void moveToLayer(int layernum) { m_layeritem->moveToLayer(layernum); m_screen.saveSlitLayer((Fluxbox::Layer) layernum); }
    FbTk::XLayerItem &getLayerItem() { return *m_layeritem; }

    virtual void timeout();


private:
    void setupMenu();
	
    void removeClient(SlitClient *client, bool remap, bool destroy);
    void loadClientList(const char *filename);
    void updateClientmenu();

    bool hidden, do_auto_hide;
    Direction m_direction;
    Placement m_placement;

    BScreen &m_screen;
    FbTk::Timer timer;

    typedef std::list<SlitClient *> SlitClients;

    SlitClients clientList;
    FbTk::Menu slitmenu, placement_menu, clientlist_menu;
    std::auto_ptr<LayerMenu<Slit> > slit_layermenu;
    std::string clientListPath;
    std::string m_filename;

    struct frame {
        Pixmap pixmap;
        FbTk::FbWindow window;

        int x, y, x_hidden, y_hidden;
        unsigned int width, height;
    } frame;
    // for KDE
    Atom kwm1_dockwindow, kwm2_dockwindow;

    std::auto_ptr<FbTk::XLayerItem> m_layeritem;
};


#endif // SLIT_HH
