// Window.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef WINDOW_HH
#define WINDOW_HH

#include "FbWinFrame.hh"
#include "Focusable.hh"
#include "FocusableTheme.hh"
#include "FocusControl.hh"
#include "WinButton.hh"

#include "FbTk/DefaultValue.hh"
#include "FbTk/Timer.hh"
#include "FbTk/FbTime.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/Signal.hh"

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <inttypes.h>

class WinClient;
class FbWinFrameTheme;
class BScreen;
class FbMenu;

namespace FbTk {
class TextButton;
class MenuTheme;
class ImageControl;
class Layer;
}

namespace Focus {
    enum {
        NoProtection = 0,
        Gain = 1,
        Refuse = 2,
        Lock = 4,
        Deny = 8
    };
    typedef unsigned int Protection;
}


/// Creates the window frame and handles any window event for it
class FluxboxWindow: public Focusable,
                     public FbTk::EventHandler,
                     private FbTk::SignalTracker {
public:
    /// Motif wm Hints
    enum {
        MwmHintsFunctions   = (1l << 0), ///< use motif wm functions
        MwmHintsDecorations = (1l << 1) ///< use motif wm decorations
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

    /// Different resize modes when resizing a window
    enum ResizeModel {
        CENTERRESIZE,                     ///< resizes from center
        TOPLEFTRESIZE,                    ///< resizes top left corner
        TOPRESIZE,                        ///< resizes top edge
        TOPRIGHTRESIZE,                   ///< resizes top right corner
        LEFTRESIZE,                       ///< resizes left edge
        RIGHTRESIZE,                      ///< resizes right edge
        BOTTOMLEFTRESIZE,                 ///< resizes bottom left corner
        BOTTOMRESIZE,                     ///< resizes bottom edge
        BOTTOMRIGHTRESIZE,                ///< resizes bottom right corner
        EDGEORCORNERRESIZE,               ///< resizes nearest edge or corner
        DEFAULTRESIZE = BOTTOMRIGHTRESIZE ///< default resize mode
    };

    /**
     * Reference corner for moves and resizes
     */
     enum ReferenceCorner {
         ERROR        = -1,
         LEFTTOP      = 0,
         TOP          = 1,
         RIGHTTOP     = 2,
         RIGHT        = 3,
         RIGHTBOTTOM  = 4,
         BOTTOM       = 5,
         LEFTBOTTOM   = 6,
         LEFT         = 7,
         CENTER       = 8
    };

    typedef std::list<WinClient *> ClientList;

    /// create a window from a client
    FluxboxWindow(WinClient &client);

    virtual ~FluxboxWindow();

    /// attach client to our client list and remove it from old window
    void attachClient(WinClient &client, int x=-1, int y=-1);
    /// detach client (remove it from list) and create a new window for it
    bool detachClient(WinClient &client);
    /// detach current working client if we have more than one
    void detachCurrentClient();
    /// remove client from client list
    bool removeClient(WinClient &client);
    /// set new current client and raise it
    bool setCurrentClient(WinClient &client, bool setinput = true);
    /**
     * Searches for a client
     * @param win the client X window
     * @return pointer to client matching the window or NULL
     */
    WinClient *findClient(Window win);
    /// select next client
    void nextClient();
    /// select previous client
    void prevClient();
    /// move the current client to the left
    void moveClientLeft();
    /// move the current client to the right
    void moveClientRight();
    /**
     * Move a client to the right of dest.
     * @param win the client to move
     * @param dest the left-of-client
     */
    void moveClientRightOf(WinClient &win, WinClient &dest);
    /**
     * Move a client to the right of dest.
     * @param win the client to move
     * @param dest the left-of-client
     */
    void moveClientLeftOf(WinClient &win, WinClient &dest);
    /**
     * Move client to place specified by pixel position
     * @param win the client to move
     * @param x position
     * @param y position
     */
    void moveClientTo(WinClient &win, int x, int y);
    /**
     * Take focus.
     * @see Focusable
     * @return true if it took focus.
     */
    bool focus();
    bool focusRequestFromClient(WinClient &from);

    /// Raises the window and takes focus (if possible).
    void raiseAndFocus() { raise(); focus(); }
    /// sets the internal focus flag
    void setFocusFlag(bool flag);
    /// make this window visible
    void show();
    /// hide window
    void hide(bool interrupt_moving = true);
    /// iconify window
    void iconify();
    /**
     * Deiconify window
     * @param do_raise raise the window when its been deiconfied
     */
    void deiconify(bool do_raise = true);

    // ------------------
    // Per window transparency addons
    int getFocusedAlpha() const { return frame().getAlpha(true); }
    int getUnfocusedAlpha() const { return frame().getAlpha(false); }
    void setFocusedAlpha(int alpha) { frame().setAlpha(true, alpha); }
    void setUnfocusedAlpha(int alpha) { frame().setAlpha(false, alpha); }
    void updateAlpha(bool focused, int alpha)  { frame().setAlpha(focused, alpha); }

    bool getUseDefaultAlpha() const { return frame().getUseDefaultAlpha(); }
    void setDefaultAlpha() { frame().setDefaultAlpha(); }
    // ------------------

    /// close current client
    void close();
    /// kill current client
    void kill();
    /// set fullscreen
    void setFullscreen(bool flag);
    /// toggle maximize
    void maximize(int type = WindowState::MAX_FULL);
    /// sets the maximized state
    void setMaximizedState(int type);
    /// maximizes the window horizontal
    void maximizeHorizontal();
    /// maximizes the window vertical
    void maximizeVertical();
    /// maximizes the window fully
    void maximizeFull();

    /// disables maximization, without restoring the old size
    void disableMaximization();

    /// toggles shade
    void shade();
    /// shades window
    void shadeOn();
    /// unshades window
    void shadeOff();
    /// sets shaded state
    void setShaded(bool val);
    /// toggles sticky
    void stick();
    /// sets stuck state
    void setStuck(bool val);
    /// toggles iconic
    void toggleIconic();
    /// sets iconic state
    void setIconic(bool val);
    void raise();
    void lower();
    void tempRaise();
    void changeLayer(int diff);
    /// moves the window to a new layer
    void moveToLayer(int layernum, bool force = false);
    int getOnHead() const;
    void setOnHead(int head);
    /// sets the window focus hidden state
    void placeWindow(int head);
    void setFocusHidden(bool value);
    /// sets the window icon hidden state
    void setIconHidden(bool value);
    /// sets whether or not the window normally gets focus when mapped
    void setFocusNew(bool value) {
        if (value)
            m_focus_protection = (m_focus_protection & ~Focus::Refuse) | Focus::Gain;
        else
            m_focus_protection = (m_focus_protection & ~Focus::Gain) | Focus::Refuse;
    }
    /// sets how to protect the focus on or against this window
    void setFocusProtection(Focus::Protection value) { m_focus_protection = value; }
    /// sets whether or not the window gets focused with mouse
    void setMouseFocus(bool value) { m_mouse_focus = value; }
    /// sets whether or not the window gets focused with click
    void setClickFocus(bool value) { m_click_focus = value; }
    void reconfigure();


    void installColormap(bool);
    void restore(WinClient *client, bool remap);
    void restore(bool remap);
    /// move frame to x, y
    void move(int x, int y);
    /// resize frame to width, height
    void resize(unsigned int width, unsigned int height);
    /// move and resize frame to pox x,y and size width, height
    void moveResize(int x, int y, unsigned int width, unsigned int height, bool send_event = false);
    /// move to pos x,y and resize client window to size width, height
    void moveResizeForClient(int x, int y, unsigned int width, unsigned int height, int gravity = ForgetGravity, unsigned int client_bw = 0);
    /**
     * Determines maximum size using all clients that this window can have.
     * @param width will be filled in with maximum width
     * @param height will be filled in with maximum height
     */
    void getMaxSize(unsigned int* width, unsigned int* height) const;
    void setWorkspace(int n);
    void updateFunctions();
    /**
     * Show window meny at at given position
     * @param mx position
     * @param my position
     */
    void showMenu(int mx, int my);

    /** popup window menu at specific location
     * @param x 
     * @param y
     */
    void popupMenu(int x, int y);
    // popup menu on last button press position
    void popupMenu();

    /**
       @name event handlers
    */
    //@{
    void handleEvent(XEvent &event);
    void keyPressEvent(XKeyEvent &ke);
    void buttonPressEvent(XButtonEvent &be);
    void buttonReleaseEvent(XButtonEvent &be);
    void motionNotifyEvent(XMotionEvent &me);
    void destroyNotifyEvent(XDestroyWindowEvent &dwe);
    void mapRequestEvent(XMapRequestEvent &mre);
    void mapNotifyEvent(XMapEvent &mapev);
    void unmapNotifyEvent(XUnmapEvent &unmapev);
    void exposeEvent(XExposeEvent &ee);
    void configureRequestEvent(XConfigureRequestEvent &ce);
    void propertyNotifyEvent(WinClient &client, Atom a);
    void enterNotifyEvent(XCrossingEvent &ev);
    void leaveNotifyEvent(XCrossingEvent &ev);
    //@}

    void applyDecorations();
    void toggleDecoration();

    unsigned int decorationMask() const;
    void setDecorationMask(unsigned int mask, bool apply = true);
    /**
     * Start moving process, grabs the pointer and draws move rectangle
     * @param x position of pointer
     * @param y position of pointer
     */
    void startMoving(int x, int y);
    /**
     * Stop moving process
     * @param interrupted whether the move was interrupted by hide or destroy
     */
    void stopMoving(bool interrupted = false);
    /**
     * Starts resizing process
     * @param x start position
     * @param y start position
     * @param dir the resize direction
     */
    void startResizing(int x, int y, ReferenceCorner dir);
    /// determine which edge or corner to resize
    ReferenceCorner getResizeDirection(int x, int y, ResizeModel model, int corner_size_px, int corner_size_pc) const;
    /// stops the resizing
    void stopResizing(bool interrupted = false);
    /// starts tabbing
    void startTabbing(const XButtonEvent &be);

    /// determine the reference corner from a string
    static ReferenceCorner getCorner(std::string str);
    /// convert to coordinates on the root window
    void translateXCoords(int &x, ReferenceCorner dir = LEFTTOP) const;
    void translateYCoords(int &y, ReferenceCorner dir = LEFTTOP) const;
    void translateCoords(int &x, int &y, ReferenceCorner dir = LEFTTOP) const;

    /**
       @name accessors
    */
    //@{

    // whether this window can be tabbed with other windows,
    // and others tabbed with it
    void setTabable(bool tabable) { functions.tabable = tabable; }
    bool isTabable() const { return functions.tabable; }
    void setMovable(bool movable) { functions.move = movable; }
    void setResizable(bool resizable) { functions.resize = resizable; }

    bool isFocusHidden() const { return m_state.focus_hidden; }
    bool isIconHidden() const { return m_state.icon_hidden; }
    bool isManaged() const { return m_initialized; }
    bool isVisible() const;
    bool isIconic() const { return m_state.iconic; }
    bool isShaded() const { return m_state.shaded; }
    bool isFullscreen() const { return m_state.fullscreen; }
    bool isMaximized() const { return m_state.isMaximized(); }
    bool isMaximizedVert() const { return m_state.isMaximizedVert(); }
    bool isMaximizedHorz() const { return m_state.isMaximizedHorz(); }
    int maximizedState() const { return m_state.maximized; }
    bool isIconifiable() const { return functions.iconify; }
    bool isMaximizable() const { return functions.maximize; }
    bool isResizable() const { return functions.resize; }
    bool isClosable() const { return functions.close; }
    bool isMoveable() const { return functions.move; }
    bool isStuck() const { return m_state.stuck; }
    bool isFocusNew() const;
    Focus::Protection focusProtection() const { return m_focus_protection; }
    bool hasTitlebar() const { return decorations.titlebar; }
    bool isMoving() const { return moving; }
    bool isResizing() const { return resizing; }
    bool isGroupable() const;
    int numClients() const { return m_clientlist.size(); }
    bool empty() const { return m_clientlist.empty(); }
    ClientList &clientList() { return m_clientlist; }
    const ClientList &clientList() const { return m_clientlist; }
    WinClient &winClient() { return *m_client; }
    const WinClient &winClient() const { return *m_client; }

    WinClient* winClientOfLabelButtonWindow(Window w);

    bool isTyping() const;

    const FbTk::LayerItem &layerItem() const { return m_frame.layerItem(); }
    FbTk::LayerItem &layerItem() { return m_frame.layerItem(); }

    Window clientWindow() const;

    FbTk::FbWindow &fbWindow();
    const FbTk::FbWindow &fbWindow() const;

    FbMenu &menu();
    const FbMenu &menu() const;

    const FbTk::FbWindow &parent() const { return m_parent; }
    FbTk::FbWindow &parent() { return m_parent; }

    bool acceptsFocus() const;
    bool isModal() const;
    const FbTk::PixmapWithMask &icon() const;
    const FbTk::BiDiString &title() const;
    const FbTk::FbString &getWMClassName() const;
    const FbTk::FbString &getWMClassClass() const;
    std::string getWMRole() const;
    long getCardinalProperty(Atom prop,bool*exists=NULL) const;
    FbTk::FbString getTextProperty(Atom prop,bool*exists=NULL) const;
    void setWindowType(WindowState::WindowType type);
    bool isTransient() const;

    int x() const { return frame().x(); }
    int y() const { return frame().y(); }
    unsigned int width() const { return frame().width(); }
    unsigned int height() const { return frame().height(); }

    int normalX() const { return m_state.x; }
    int normalY() const { return m_state.y; }
    unsigned int normalWidth() const { return m_state.width; }
    unsigned int normalHeight() const { return m_state.height; }

    int xOffset() const { return frame().xOffset(); }
    int yOffset() const { return frame().yOffset(); }
    int widthOffset() const { return frame().widthOffset(); }
    int heightOffset() const { return frame().heightOffset(); }

    unsigned int workspaceNumber() const { return m_workspace_number; }

    int layerNum() const { return m_state.layernum; }
    void setLayerNum(int layernum);

    unsigned int titlebarHeight() const;

    int initialState() const;

    FbWinFrame &frame() { return m_frame; }
    const FbWinFrame &frame() const { return m_frame; }

    /**
       @name signals
       @{
    */
    FbTk::Signal<FluxboxWindow &> &stateSig() { return m_statesig; }
    FbTk::Signal<FluxboxWindow &> &layerSig() { return m_layersig; }
    FbTk::Signal<FluxboxWindow &> &hintSig() { return m_hintsig; }
    FbTk::Signal<FluxboxWindow &> &workspaceSig() { return m_workspacesig; }
    /** @} */ // end group signals

    //@}

    bool oplock; ///< Used to help stop transient loops occurring by locking a window during certain operations

private:
    /// signal callback for title changes by clients
    void setTitle(const std::string &title, Focusable &client);

    void setupWindow();
    void updateButtons();

    void init();
    void updateClientLeftWindow();
    void grabButtons();

    void themeReconfigured();

    /**
     * Calculates insertition position in the list by
     * using pixel position x and y.
     * @param x position
     * @param y position
     * @return iterator position for insertion
     */
    ClientList::iterator getClientInsertPosition(int x, int y);

    /// try to attach current attaching client to a window at pos x, y
    void attachTo(int x, int y, bool interrupted = false);

    bool getState();
    void updateMWMHintsFromClient(WinClient &client);
    void updateSizeHints();
    void associateClientWindow();

    void setState(unsigned long stateval, bool setting_up);
    /// set the layer of a fullscreen window
    void setFullscreenLayer();
    void attachWorkAreaSig();

    // modifies left and top if snap is necessary
    void doSnapping(int &left, int &top, bool resize = false);
    // user_w/h return the values that should be shown to the user
    void fixSize();
    void moveResizeClient(WinClient &client);
    /// sends configurenotify to all clients
    void sendConfigureNotify();
    void updateResize() { moveResize(m_last_resize_x, m_last_resize_y, m_last_resize_w, m_last_resize_h); }

    static void grabPointer(Window grab_window,
                     Bool owner_events,
                     unsigned int event_mask,
                     int pointer_mode, int keyboard_mode,
                     Window confine_to,
                     Cursor cursor,
                     Time time);
    static void ungrabPointer(Time time);

    void associateClient(WinClient &client);
    /// Called when focused changed, and is attached when it is not in fullscreen mode
    void focusedWindowChanged(BScreen &screen, FluxboxWindow *focused_win, WinClient* client);
    /// Called when workspace area on screen changed.
    void workspaceAreaChanged(BScreen &screen);
    void frameExtentChanged();


    // state and hint signals
    FbTk::Signal<FluxboxWindow &> m_workspacesig, m_statesig, m_layersig, m_hintsig;

    uint64_t m_creation_time;
    uint64_t m_last_keypress_time;
    FbTk::Timer m_timer;
    FbTk::Timer m_tabActivationTimer;
    FbTk::Timer m_resizeTimer;

    // Window states
    bool moving, resizing, m_initialized;

    WinClient *m_attaching_tab;

    Display *display; /// display connection

    int m_button_grab_x, m_button_grab_y; // handles last button press event for move
    int m_last_resize_x, m_last_resize_y; // handles last button press event for resize
    int m_last_move_x, m_last_move_y; // handles last pos for non opaque moving
    int m_last_resize_h, m_last_resize_w; // handles height/width for resize "window"
    int resize_base_x, resize_base_y, resize_base_w, resize_base_h; // opaque and transparent resize alignment
    int m_last_pressed_button;

    unsigned int m_workspace_number;
    unsigned long m_current_state; // NormalState | IconicState | Withdrawn

    unsigned int m_old_decoration_mask;

    ClientList m_clientlist;
    WinClient *m_client; ///< current client
    typedef std::map<WinClient *, IconButton *> Client2ButtonMap;
    Client2ButtonMap m_labelbuttons;
    FbTk::Timer m_reposLabels_timer;
    void emitLabelReposSig();
    bool m_has_tooltip;

    SizeHints m_size_hint;
    struct {
        bool titlebar:1, handle:1, border:1, iconify:1,
            maximize:1, close:1, menu:1, sticky:1, shade:1, tab:1, enabled:1;
    } decorations;

    std::vector<WinButton::Type> m_titlebar_buttons[2];
    bool m_toggled_decos;

    struct {
        bool resize:1, move:1, iconify:1, maximize:1, close:1, tabable:1;
    } functions;

    typedef FbTk::ConstObjectAccessor<bool, FocusControl> BoolAcc;
    /// if the window is normally focused when mapped
    /// special focus permissions
    Focus::Protection m_focus_protection;
    /// if the window is focused with EnterNotify
    FbTk::DefaultValue<bool, BoolAcc> m_mouse_focus;
    bool m_click_focus;  ///< if the window is focused by clicking
    int m_last_button_x, ///< last known x position of the mouse button
        m_last_button_y; ///< last known y position of the mouse button

    FocusableTheme<WinButtonTheme> m_button_theme;
    FocusableTheme<FbWinFrameTheme> m_theme;

    WindowState m_state;
    FbWinFrame m_frame;  ///< the actual window frame

    bool m_placed; ///< determine whether or not we should place the window

    int m_old_layernum;

    FbTk::FbWindow &m_parent; ///< window on which we draw move/resize rectangle  (the "root window")

    ReferenceCorner m_resize_corner; //< the current corner used while resizing

    static int s_num_grabs; ///< number of XGrabPointer's
};


#endif // WINDOW_HH
