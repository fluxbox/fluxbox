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
	
/// $Id: Slit.hh,v 1.35 2003/06/24 13:42:23 fluxgen Exp $

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
class Strut;

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
    inline bool doAutoHide() const { return *m_rc_auto_hide; }
    inline Direction direction() const { return *m_rc_direction; }
    inline Placement placement() const { return *m_rc_placement; }
    inline int getOnHead() const { return *m_rc_on_head; }
    inline void saveOnHead(int head) { m_rc_on_head = head; }
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
    SlitTheme &theme() { return *m_slit_theme.get(); }
    const SlitTheme &theme() const { return *m_slit_theme.get(); }
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
    FbTk::XLayerItem &layerItem() { return *m_layeritem; }

    virtual void timeout();

private:
    void clearWindow();
    void setupMenu();
	
    void removeClient(SlitClient *client, bool remap, bool destroy);
    void loadClientList(const char *filename);
    void updateClientmenu();
    void clearStrut();
    void updateStrut();

    bool m_hidden;

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
    std::auto_ptr<FbTk::Transparent> m_transp;
    static unsigned int s_eventmask;
    Strut *m_strut;

    FbTk::Resource<bool> m_rc_auto_hide, m_rc_maximize_over;
    FbTk::Resource<Slit::Placement> m_rc_placement;
    FbTk::Resource<Slit::Direction> m_rc_direction;
    FbTk::Resource<int> m_rc_alpha, m_rc_on_head;
    FbTk::Resource<Fluxbox::Layer> m_rc_layernum;
};


#endif // SLIT_HH
