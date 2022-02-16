// Toolbar.hh for Fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Toolbar.hh for Blackbox - an X11 Window manager
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

#ifndef TOOLBAR_HH
#define TOOLBAR_HH

#include "ToolbarTheme.hh"
#include "LayerMenu.hh"
#include "ToolFactory.hh"
#include "ToolTheme.hh"
#include "Layer.hh"

#ifdef XINERAMA
#include "Xinerama.hh"
#endif // XINERAMA

#include "FbTk/Timer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Layer.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/Signal.hh"

#include <memory>

class BScreen;
class Strut;
class FbMenu;
class ToolbarItem;

namespace FbTk {
class ImageControl;
class Shape;
class TextButton;
}

/// The toolbar.
/// Handles iconbar, workspace name view and clock view
class Toolbar: public FbTk::EventHandler,
               public LayerObject {
public:

    /// Toolbar placement on the screen
    enum Placement {
        // top and bottom placement
        TOPLEFT = 1, TOPCENTER, TOPRIGHT,
        BOTTOMLEFT, BOTTOMCENTER, BOTTOMRIGHT,
        // left and right placement
        LEFTBOTTOM, LEFTCENTER, LEFTTOP,
        RIGHTBOTTOM, RIGHTCENTER, RIGHTTOP,

        DEFAULT = BOTTOMRIGHT
    };

    /// Create a toolbar on the screen with specific width
    Toolbar(BScreen &screen, FbTk::Layer &layer, size_t width = 200);

    virtual ~Toolbar();

    void raise();
    void lower();
    void updateVisibleState();
    void toggleHidden();

    void toggleAboveDock();

    void moveToLayer(int layernum);

    void saveOnHead(int head);

    /**
       @name eventhandlers
    */
    //@{
    void buttonPressEvent(XButtonEvent &be);
    void enterNotifyEvent(XCrossingEvent &ce);
    void leaveNotifyEvent(XCrossingEvent &ce);
    void exposeEvent(XExposeEvent &ee);
    void handleEvent(XEvent &event);
    //@}

    void relayout();
    void reconfigure();
    void setPlacement(Placement where);

    int layerNumber() const { return const_cast<FbTk::LayerItem &>(m_layeritem).getLayerNum(); }

    const FbTk::Menu &menu() const { return m_toolbarmenu; }
    FbTk::Menu &menu() { return m_toolbarmenu; }
    FbTk::Menu &placementMenu() { return m_placementmenu; }
    const FbTk::Menu &placementMenu() const { return m_placementmenu; }

    FbTk::Menu &layerMenu() { return m_layermenu; }
    const FbTk::Menu &layerMenu() const { return m_layermenu; }

    /// are we hidden?
    bool isHidden() const { return m_hidden; }
    /// do we auto hide the toolbar?
    bool doAutoHide() const { return *m_rc_auto_hide; }
    ///	@return X window of the toolbar
    const FbTk::FbWindow &window() const { return frame.window; }
    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }
    unsigned int width() const { return frame.window.width(); }
    unsigned int height() const { return frame.window.height(); }
    int x() const { return isHidden() ? frame.x_hidden : frame.x; }
    int y() const { return isHidden() ? frame.y_hidden : frame.y; }
    Placement placement() const { return *m_rc_placement; }
    /// @return pointer to iconbar if it got one, else 0
    const FbTk::ThemeProxy<ToolbarTheme> &theme() const { return m_theme; }
    FbTk::ThemeProxy<ToolbarTheme> &theme() { return m_theme; }
    bool isVertical() const;

    int getOnHead() const { return *m_rc_on_head; }

    unsigned char alpha() const { return *m_rc_alpha; }
private:
    void rearrangeItems();
    void deleteItems();

    void setupMenus(bool skip_new_placement=false);
    void clearStrut();
    void updateStrut();
    void updateAlpha();

    void updateCrossingState();

    /// Called when the screen changed property.
    void screenChanged(BScreen &screen);

    bool m_hidden;       ///< hidden state

    /// Toolbar frame
    struct Frame {
        Frame(FbTk::EventHandler &evh, int screen_num);
        ~Frame();

        FbTk::FbWindow window;

        int x, y, x_hidden, y_hidden, grab_x, grab_y;
        unsigned int width, height, bevel_w;
    } frame;
    // background pixmap
    Pixmap m_window_pm;

    BScreen &m_screen; ///< screen connection

    FbTk::Timer m_hide_timer; ///< timer to for auto hide toolbar

    FbTk::LayerItem m_layeritem; ///< layer item, must be declared before layermenu
    LayerMenu m_layermenu;
    FbMenu m_placementmenu, m_toolbarmenu;
#ifdef XINERAMA
    XineramaHeadMenu<Toolbar> *m_xineramaheadmenu;
#endif // XINERAMA


    // themes
    ToolbarTheme m_theme;


    typedef std::list<ToolbarItem *> ItemList;
    ItemList m_item_list;

    ToolFactory m_tool_factory;

    Strut *m_strut; ///< created and destroyed by BScreen

    // resources
    FbTk::Resource<bool> m_rc_auto_hide, m_rc_auto_raise, m_rc_maximize_over, m_rc_visible;
    FbTk::Resource<int> m_rc_width_percent;
    FbTk::Resource<int> m_rc_alpha;
    FbTk::Resource<class ResourceLayer> m_rc_layernum;
    FbTk::Resource<int> m_rc_on_head;
    FbTk::Resource<Placement> m_rc_placement;
    FbTk::Resource<int> m_rc_height;
    FbTk::Resource<std::string> m_rc_tools;
    std::unique_ptr<FbTk::Shape> m_shape;
    typedef std::list<std::string> StringList;
    StringList m_tools;

    bool m_resize_lock; ///< to lock rearrangeItems or not
    FbTk::SignalTracker m_signal_tracker;
};


#endif // TOOLBAR_HH
