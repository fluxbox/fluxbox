// Slit.hh for Fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef SLIT_HH
#define SLIT_HH

#include "SlitTheme.hh"
#include "LayerMenu.hh"
#include "Layer.hh"

#ifdef XINERAMA
#include "Xinerama.hh"
#endif // XINERAMA

#include "FbTk/Menu.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/Timer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/Signal.hh"

#include <X11/Xutil.h>

#include <list>
#include <memory>

class SlitClient;
class BScreen;
class FbMenu;
class Strut;
class Layer;

/// Handles dock apps
class Slit: public FbTk::EventHandler, public LayerObject, private FbTk::SignalTracker
{
public:
    typedef std::list<SlitClient *> SlitClients;
    /**
       Placement on screen
    */
    enum Placement {
        // top and bottom placement
        TOPLEFT = 1, TOPCENTER, TOPRIGHT,
        BOTTOMLEFT, BOTTOMCENTER, BOTTOMRIGHT,
        // left and right placement
        LEFTBOTTOM, LEFTCENTER, LEFTTOP,
        RIGHTBOTTOM, RIGHTCENTER, RIGHTTOP
    };

    Slit(BScreen &screen, FbTk::Layer &layer, const char *filename = 0);
    virtual ~Slit();

    void show() { frame.window.show(); m_visible = true; }
    void hide() { frame.window.hide(); m_visible = false; }
    void setPlacement(Placement place);
    void addClient(Window clientwin);
    void removeClient(Window clientwin, bool remap = true);
    void reconfigure();
    void reposition();
    void shutdown();
    /// save clients name in a file
    void saveClientList();
    /// move client one position up
    void clientUp(SlitClient*);
    /// move client one position down
    void clientDown(SlitClient*);
    /// cycle slit clients up one step
    void cycleClientsUp();
    /// cycle slit clients down one step
    void cycleClientsDown();
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
    void toggleHidden();
    void toggleAboveDock();

    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }
    FbTk::ThemeProxy<SlitTheme> &theme() { return *m_slit_theme.get(); }
    const FbTk::ThemeProxy<SlitTheme> &theme() const { return *m_slit_theme.get(); }

    int layerNumber() const { return m_layeritem->getLayerNum(); }

    bool isHidden() const { return m_hidden; }
    bool acceptKdeDockapp() const { return *m_rc_kde_dockapp; }
    bool doAutoHide() const { return *m_rc_auto_hide; }
    Placement placement() const { return *m_rc_placement; }
    int getOnHead() const { return *m_rc_on_head; }
    void saveOnHead(int head);
    FbTk::Menu &menu() { return m_slitmenu; }

    const FbTk::FbWindow &window() const { return frame.window; }

    int x() const { return (m_hidden ? frame.x_hidden : frame.x); }
    int y() const { return (m_hidden ? frame.y_hidden : frame.y); }

    unsigned int width() const { return frame.width; }
    unsigned int height() const { return frame.height; }
    const SlitClients &clients() const { return m_client_list; }
    SlitClients &clients() { return m_client_list; }
private:
    /// Called when screen has changed
    void screenSizeChanged(BScreen &screen);

    void updateAlpha();
    void updateCrossingState();
    void clearWindow();
    void setupMenu();

    void removeClient(SlitClient *client, bool remap, bool destroy);
    void loadClientList(const char *filename);
    void updateClientmenu();
    void clearStrut();
    void updateStrut();

    // m_hidden is for autohide, m_visible is the FbWindow state
    bool m_hidden, m_visible;
    bool m_pending_reconfigure;

    BScreen &m_screen;
    FbTk::Timer m_timer;

    SlitClients m_client_list;
    std::unique_ptr<LayerMenu> m_layermenu;
    FbMenu m_clientlist_menu, m_slitmenu;
#ifdef XINERAMA
    XineramaHeadMenu<Slit> *m_xineramaheadmenu;
#endif // XINERAMA
    std::string m_filename;

    struct frame {
        frame(const FbTk::FbWindow &parent):
            pixmap(0),
            window(parent, 0, 0, 10, 10,
                   SubstructureRedirectMask |  ButtonPressMask |
                   EnterWindowMask | LeaveWindowMask | ExposureMask,
                   true),  // override redirect
            x(0), y(0), x_hidden(0), y_hidden(0),
        width(10), height(10) {  }

        Pixmap pixmap;
        FbTk::FbWindow window;
        int x, y, x_hidden, y_hidden;
        unsigned int width, height;
    } frame;

    // for KDE
    Atom m_kwm1_dockwindow, m_kwm2_dockwindow;

    std::unique_ptr<FbTk::LayerItem> m_layeritem;
    std::unique_ptr<SlitTheme> m_slit_theme;
    static unsigned int s_eventmask;
    Strut *m_strut;

    FbTk::Resource<bool> m_rc_kde_dockapp, m_rc_auto_hide, m_rc_auto_raise, m_rc_maximize_over;
    FbTk::Resource<Slit::Placement> m_rc_placement;
    FbTk::Resource<int> m_rc_alpha, m_rc_on_head;
    FbTk::Resource<class ResourceLayer> m_rc_layernum;
};


#endif // SLIT_HH
