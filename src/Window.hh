// Window.hh for Fluxbox Window Manager
// Copyright (c) 2001-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Window.hh for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Window.hh,v 1.51 2003/02/22 15:10:43 rathnor Exp $

#ifndef	 WINDOW_HH
#define	 WINDOW_HH

#include "BaseDisplay.hh"
#include "Timer.hh"
#include "Menu.hh"
#include "Subject.hh"
#include "FbWinFrame.hh"
#include "EventHandler.hh"
#include "XLayerItem.hh"
#include "LayerMenu.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <vector>
#include <string>
#include <memory>

#define PropMwmHintsElements	3

class Tab;
class FbWinFrameTheme;
class BScreen;

namespace FbTk {
class MenuTheme;
class ImageControl;
class XLayer;
};


/// Creates the window frame and handles any window event for it
class FluxboxWindow : public FbTk::TimeoutHandler, public FbTk::EventHandler {
public:
    /// decoration bit
    enum Decoration {
        DECOR_NONE=0, ///< no decor at all
        DECOR_NORMAL, ///< normal normal
        DECOR_TINY,   ///< tiny decoration
        DECOR_TOOL    ///< decor tool
    };

    /// Motif wm Hints
    enum {
        MwmHintsFunctions   = (1l << 0), ///< use motif wm functions
        MwmHintsDecorations	= (1l << 1) ///< use motif wm decorations
    };
	
    /// Motif wm functions
    enum MwmFunc{
        MwmFuncAll          = (1l << 0), ///< all motif wm functions
        MwmFuncResize       = (1l << 1), ///< resize 
        MwmFuncMove         = (1l << 2), ///< move 
        MwmFuncIconify      = (1l << 3), ///< iconify
        MwmFuncMaximize     = (1l << 4), ///< maximize
        MwmFuncClose        = (1l << 5)  ///< close
    };

    /// Motif wm decorations
    enum MwmDecor {
        MwmDecorAll         = (1l << 0), /// all decorations
        MwmDecorBorder      = (1l << 1), /// border
        MwmDecorHandle      = (1l << 2), /// handle
        MwmDecorTitle       = (1l << 3), /// title
        MwmDecorMenu        = (1l << 4), /// menu
        MwmDecorIconify     = (1l << 5), /// iconify
        MwmDecorMaximize    = (1l << 6)  /// maximize
    };

    /// create fluxbox window with parent win and screen connection
    FluxboxWindow(Window win, BScreen *scr, 
                  int screen_num, FbTk::ImageControl &imgctrl, FbWinFrameTheme &tm,
                  FbTk::MenuTheme &menutheme,
                  FbTk::XLayer &layer);
    virtual ~FluxboxWindow();


    void setWindowNumber(int n) { window_number = n; }
      
    bool validateClient();
    bool setInputFocus();
    void raiseAndFocus() { raise(); setInputFocus(); }
    void setTab(bool flag);
    void setFocusFlag(bool flag);
    // map this window
    void show();
    // unmap this window
    void hide();
    void iconify();
    void deiconify(bool = true, bool = true);
    /// destroy this window
    void close();
    /// set the window in withdrawn state
    void withdraw();
    /// toggel maximize
    void maximize();
    /// maximizes the window horizontal
    void maximizeHorizontal();
    /// maximizes the window vertical
    void maximizeVertical();
    /// toggles shade
    void shade();
    /// toggles sticky
    void stick(); 
    void raise();
    void lower();
    void raiseLayer();
    void lowerLayer();
    void moveToLayer(int layernum);

    void reconfigure();
    void installColormap(bool);
    void restore(bool remap);
    /// move frame to x, y
    void move(int x, int y);
    /// resize frame to width, height
    void resize(unsigned int width, unsigned int height);
    /// move and resize frame to pox x,y and size width, height
    void moveResize(int x, int y, unsigned int width, unsigned int height);

    void setWorkspace(int n);
    void changeBlackboxHints(const BaseDisplay::BlackboxHints &bh);
    void restoreAttributes();
    void showMenu(int mx, int my);
    // popup menu on last button press position
    void popupMenu();

    void pauseMoving();
    void resumeMoving();
    /**
       @name event handlers
    */
    //@{
    void handleEvent(XEvent &event);
    void buttonPressEvent(XButtonEvent &be);
    void buttonReleaseEvent(XButtonEvent &be);
    void motionNotifyEvent(XMotionEvent &me);
    void destroyNotifyEvent(XDestroyWindowEvent &dwe);
    void mapRequestEvent(XMapRequestEvent &mre);
    void mapNotifyEvent(XMapEvent &mapev);
    void unmapNotifyEvent(XUnmapEvent &unmapev);
    void exposeEvent(XExposeEvent &ee);
    void configureRequestEvent(XConfigureRequestEvent &ce);
    void propertyNotifyEvent(Atom a);
    void enterNotifyEvent(XCrossingEvent &ev);
    void leaveNotifyEvent(XCrossingEvent &ev);
    //@}

    void setDecoration(Decoration decoration);
    void toggleDecoration();
	
#ifdef SHAPE
    void shapeEvent(XShapeEvent *event);
#endif // SHAPE

    virtual void timeout();

    /**
       @name accessors		
    */
    //@{
    bool isTransient() const { return ((client.transient_for) ? true : false); }
    bool hasTransient() const { return ((client.transients.size()) ? true : false); }
    bool isManaged() const { return m_managed; }
    bool isFocused() const { return focused; }
    bool isVisible() const { return visible; }
    bool isIconic() const { return iconic; }
    bool isShaded() const { return shaded; }
    bool isMaximized() const { return maximized; }
    bool isIconifiable() const { return functions.iconify; }
    bool isMaximizable() const { return functions.maximize; }
    bool isResizable() const { return functions.resize; }
    bool isClosable() const { return functions.close; }
    bool isStuck() const { return stuck; }
    bool hasTitlebar() const { return decorations.titlebar; }
    bool hasTab() const { return (tab!=0 ? true : false); }
    bool isMoving() const { return moving; }
    bool isResizing() const { return resizing; }
    bool isGroupable() const;

    const BScreen *getScreen() const { return screen; }
    BScreen *getScreen() { return screen; }

    const FbTk::XLayerItem &getLayerItem() const { return m_layeritem; }
    FbTk::XLayerItem &getLayerItem() { return m_layeritem; }

    const Tab *getTab() const { return tab; }
    Tab *getTab() { return tab; }

    const std::list<FluxboxWindow *> &getTransients() const { return client.transients; } 
    std::list<FluxboxWindow *> &getTransients() { return client.transients; } 	

    const FluxboxWindow *getTransientFor() const { return client.transient_for; }
    FluxboxWindow *getTransientFor() { return client.transient_for; }
	
    Window getFrameWindow() const { return m_frame.window().window(); }
    Window getClientWindow() const { return client.window; }

    FbTk::FbWindow &getFbWindow() { return m_frame.window(); }
    const FbTk::FbWindow &getFbWindow() const { return m_frame.window(); }

    FbTk::Menu &getWindowmenu() { return m_windowmenu; }
    const FbTk::Menu &getWindowmenu() const { return m_windowmenu; }

    FbTk::Menu *getLayermenu() { return m_layermenu; }
    const FbTk::Menu *getLayermenu() const { return m_layermenu; }
	
    const std::string &getTitle() const { return client.title; }
    const std::string &getIconTitle() const { return client.icon_title; }
    int getXFrame() const { return m_frame.x(); }
    int getYFrame() const { return m_frame.y(); }
    int getXClient() const { return client.x; }
    int getYClient() const { return client.y; }
    unsigned int getWorkspaceNumber() const { return workspace_number; }
    int getWindowNumber() const { return window_number; }
    int getLayerNum() const { return m_layernum; }
    void setLayerNum(int layernum);
    unsigned int getWidth() const { return m_frame.width(); }
    unsigned int getHeight() const { return m_frame.height(); }
    unsigned int getClientHeight() const { return client.height; }
    unsigned int getClientWidth() const { return client.width; }
    unsigned int getTitleHeight() const { return m_frame.titleHeight(); }
    const std::string &className() const { return m_class_name; }
    const std::string &instanceName() const { return m_instance_name; }
    bool isLowerTab() const;
    int initialState() const { return client.initial_state; }

    FbWinFrame &frame() { return m_frame; }
    const FbWinFrame &frame() const { return m_frame; }

    /**
       @name signals
       @{
    */
    FbTk::Subject &stateSig() { return m_statesig; }
    const FbTk::Subject &stateSig() const { return m_statesig; }
    FbTk::Subject &layerSig() { return m_layersig; }
    const FbTk::Subject &layerSig() const { return m_layersig; }
    FbTk::Subject &hintSig() { return m_hintsig; }
    const FbTk::Subject &hintSig() const { return m_hintsig; }
    FbTk::Subject &workspaceSig() { return m_workspacesig; }
    const FbTk::Subject &workspaceSig() const { return m_workspacesig; }
    FbTk::Subject &dieSig() { return m_diesig; }
    const FbTk::Subject &dieSig() const { return m_diesig; }
    /** @} */ // end group signals

    const timeval &getLastFocusTime() const {return lastFocusTime;}

    //@}
	
    class WinSubject: public FbTk::Subject {
    public:
        WinSubject(FluxboxWindow &w):m_win(w) { }
        FluxboxWindow &win() { return m_win; }
        const FluxboxWindow &win() const { return m_win; }
    private:
        FluxboxWindow &m_win;
    };

private:
    // this structure only contains 3 elements... the Motif 2.0 structure contains
    // 5... we only need the first 3... so that is all we will define
    typedef struct MwmHints {
        unsigned long flags;       // Motif wm flags
        unsigned long functions;   // Motif wm functions
        unsigned long decorations; // Motif wm decorations
    } MwmHints;

    void grabButtons();
	
    void startMoving(Window win);
    void stopMoving();
    void startResizing(Window win, int x, int y, bool left); 
    void stopResizing(Window win=0);
    void updateIcon();
	
    void updateTransientInfo();

    bool getState();
    /// gets title string from client window and updates frame's title
    void updateTitleFromClient();
    /// gets icon name from client window
    void updateIconNameFromClient();
    void getWMNormalHints();
    void getWMProtocols();
    void getWMHints();
    void getMWMHints();
    void getBlackboxHints();
    void saveBlackboxHints();
    void setNetWMAttributes();
    void associateClientWindow();
    void createWinButtons();
    void decorateLabel();
    void positionWindows();
	
    void restoreGravity();
    void setGravityOffsets();
    void setState(unsigned long stateval);
    void upsize();
    void downsize();
    void right_fixsize(int *x = 0, int *y = 0);
    void left_fixsize(int *x = 0, int *y = 0);

    // state and hint signals
    WinSubject m_hintsig, m_statesig, m_layersig, m_workspacesig, m_diesig;

    std::string m_instance_name; /// instance name from WM_CLASS
    std::string m_class_name; /// class name from WM_CLASS
	
    //Window state
    bool moving, resizing, shaded, maximized, visible, iconic, transient,
        focused, stuck, modal, send_focus_message, m_managed;

    BScreen *screen; /// screen on which this window exist
    FbTk::Timer timer;
    Display *display; /// display connection (obsolete by FbTk)
    BaseDisplay::BlackboxAttributes blackbox_attrib;

    Time lastButtonPressTime;
    FbTk::Menu m_windowmenu;
    LayerMenu<FluxboxWindow> *m_layermenu;
    
    timeval lastFocusTime;
	
    int button_grab_x, button_grab_y; // handles last button press event for move
    int last_resize_x, last_resize_y; // handles last button press event for resize
    int last_move_x, last_move_y; // handles last pos for non opaque moving
    unsigned int last_resize_h, last_resize_w; // handles height/width for resize "window"

    int focus_mode, window_number;
    unsigned int workspace_number;
    unsigned long current_state;

    Decoration old_decoration;

    struct _client {
        FluxboxWindow *transient_for; // which window are we a transient for?
        std::list<FluxboxWindow *>	transients;  // which windows are our transients?
        Window window, window_group;

        std::string title, icon_title;
        int x, y, old_bw;
        unsigned int width, height, title_text_w,
            min_width, min_height, max_width, max_height, width_inc, height_inc,
            min_aspect_x, min_aspect_y, max_aspect_x, max_aspect_y,
            base_width, base_height, win_gravity;
        unsigned long initial_state, normal_hint_flags, wm_hint_flags;

        MwmHints *mwm_hint;
        BaseDisplay::BlackboxHints *blackbox_hint;
		
    } client;

    struct _decorations {
        bool titlebar, handle, border, iconify,
            maximize, close, menu, sticky, shade, tab, enabled;
    } decorations;

    struct _functions {
        bool resize, move, iconify, maximize, close;
    } functions;
	

    Tab *tab;
    friend class Tab; //TODO: Don't like long distant friendship
	
    
    int frame_resize_x, frame_resize_w;
    int frame_resize_y, frame_resize_h;
    int m_old_pos_x, m_old_pos_y; ///< old position so we can restore from maximized
    unsigned int m_old_width, m_old_height; ///< old size so we can restore from maximized state
    int m_last_button_x, ///< last known x position of the mouse button
        m_last_button_y; ///< last known y position of the mouse button
    FbWinFrame m_frame;

    FbTk::XLayerItem m_layeritem;
    int m_layernum;

    enum { F_NOINPUT = 0, F_PASSIVE, F_LOCALLYACTIVE, F_GLOBALLYACTIVE };
  
};

template <>
void LayerMenuItem<FluxboxWindow>::click(int button, int time) {
    m_object->moveToLayer(m_layernum);
}


#endif // WINDOW_HH
