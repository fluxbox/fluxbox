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
	
/// $Id: Slit.hh,v 1.27 2003/05/11 17:42:51 fluxgen Exp $

#ifndef	 SLIT_HH
#define	 SLIT_HH

#include "Menu.hh"
#include "FbWindow.hh"
#include "Timer.hh"
#include "XLayerItem.hh"
#include "LayerMenu.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <list>
#include <string>
#include <memory>

class SlitTheme;
class SlitClient;
class BScreen;
class FbMenu;

/// Handles dock apps
class Slit : public FbTk::TimeoutHandler, public FbTk::EventHandler {
public:
    
    /**
       Client alignment
    */
    enum Direction { VERTICAL = 1, HORIZONTAL };
    /**
       Placement on screen
    */
    enum Placement { TOPLEFT = 1, CENTERLEFT, BOTTOMLEFT, TOPCENTER, BOTTOMCENTER,
           TOPRIGHT, CENTERRIGHT, BOTTOMRIGHT };
    Slit(BScreen &screen, FbTk::XLayer &layer, const char *filename = 0);
    virtual ~Slit();

    inline bool isHidden() const { return m_hidden; }
    inline bool doAutoHide() const { return m_do_auto_hide; }
    inline Direction direction() const { return m_direction; }
    inline Placement placement() const { return m_placement; }
    FbTk::Menu &menu() { return m_slitmenu; }

    inline const FbTk::FbWindow &window() const { return frame.window; }

    inline int x() const { return (m_hidden ? frame.x_hidden : frame.x); }
    inline int y() const { return (m_hidden ? frame.y_hidden : frame.y); }

    inline unsigned int width() const { return frame.width; }
    inline unsigned int height() const { return frame.height; }

    void setDirection(Direction dir);
    void setPlacement(Placement place);
    void setAutoHide(bool val);
    void addClient(Window clientwin);
    void removeClient(Window clientwin, bool remap = true);
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
    void exposeEvent(XExposeEvent &event);
    //@}
	
    void moveToLayer(int layernum);
    FbTk::XLayerItem &getLayerItem() { return *m_layeritem; }

    virtual void timeout();


private:
    void setupMenu();
	
    void removeClient(SlitClient *client, bool remap, bool destroy);
    void loadClientList(const char *filename);
    void updateClientmenu();

    bool m_hidden, m_do_auto_hide;
    Direction m_direction;
    Placement m_placement;

    BScreen &m_screen;
    FbTk::Timer m_timer;

    typedef std::list<SlitClient *> SlitClients;

    SlitClients m_client_list;
    FbMenu m_slitmenu, m_placement_menu, m_clientlist_menu;
    std::auto_ptr<LayerMenu<Slit> > m_layermenu;
    std::string m_filename;

    struct frame {
        Pixmap pixmap;
        FbTk::FbWindow window;

        int x, y, x_hidden, y_hidden;
        unsigned int width, height;
    } frame;

    // for KDE
    Atom m_kwm1_dockwindow, m_kwm2_dockwindow;

    std::auto_ptr<FbTk::XLayerItem> m_layeritem;
    std::auto_ptr<SlitTheme> m_slit_theme;
};


#endif // SLIT_HH
