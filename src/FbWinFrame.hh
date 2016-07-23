// FbWinFrame.hh for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBWINFRAME_HH
#define FBWINFRAME_HH

#include "WindowState.hh"

#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Color.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/Container.hh"
#include "FbTk/Shape.hh"
#include "FbTk/Signal.hh"

#include <vector>
#include <memory>

class FbWinFrameTheme;
class BScreen;
class IconButton;
class Focusable;
template <class T> class FocusableTheme;

namespace FbTk {
class ImageControl;
template <class T> class Command;
class Texture;
}

/// holds a window frame with a client window
/// (see: <a href="fluxbox_fbwinframe.png">image</a>)
class FbWinFrame:public FbTk::EventHandler {
public:
    // STRICTINTERNAL means it doesn't go external automatically when no titlebar
    enum TabMode { NOTSET = 0, INTERNAL = 1, EXTERNAL };

   /// Toolbar placement on the screen
    enum TabPlacement{
        // top and bottom placement
        TOPLEFT = 1, TOP, TOPRIGHT,
        BOTTOMLEFT, BOTTOM, BOTTOMRIGHT,
        // left and right placement
        LEFTBOTTOM, LEFT, LEFTTOP,
        RIGHTBOTTOM, RIGHT, RIGHTTOP,

        DEFAULT = TOPLEFT
    };

    /// create a top level window
    FbWinFrame(BScreen &screen, unsigned int client_depth, WindowState &state,
               FocusableTheme<FbWinFrameTheme> &theme);

    /// destroy frame
    ~FbWinFrame();

    void hide();
    void show();
    bool isVisible() const { return m_visible; }

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    /// resize client to specified size and resize frame to it
    void resizeForClient(unsigned int width, unsigned int height, int win_gravity=ForgetGravity, unsigned int client_bw = 0);

    // for when there needs to be an atomic move+resize operation
    void moveResizeForClient(int x, int y,
                             unsigned int width, unsigned int height,
                             int win_gravity=ForgetGravity, unsigned int client_bw = 0, bool move = true, bool resize = true);

    // can elect to ignore move or resize (mainly for use of move/resize individual functions
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height,
                    bool move = true, bool resize = true, bool force = false);

    // move without transparency or special effects (generally when dragging)
    void quietMoveResize(int x, int y,
                         unsigned int width, unsigned int height);

    /// some outside move/resize happened, and we need to notify all of our windows
    /// in case of transparency
    void notifyMoved(bool clear);
    void clearAll();

    /// set focus/unfocus style
    void setFocus(bool newvalue);

    void setFocusTitle(const FbTk::BiDiString &str) { m_label.setText(str); }
    bool setTabMode(TabMode tabmode);
    void updateTabProperties() { alignTabs(); }

    /// Alpha settings
    void setAlpha(bool focused, int value);
    int getAlpha(bool focused) const;
    void applyAlpha();

    void setDefaultAlpha();
    bool getUseDefaultAlpha() const;

    /// add a button to the left of the label
    void addLeftButton(FbTk::Button *btn);
    /// add a button to the right of the label
    void addRightButton(FbTk::Button *btn);
    /// remove all buttons from titlebar
    void removeAllButtons();
    /// adds a button to tab container
    void createTab(FbTk::Button &button);
    /// removes a specific button from label window
    void removeTab(IconButton *id);
    /// move label button to the left
    void moveLabelButtonLeft(FbTk::TextButton &btn);
    /// move label button to the right
    void moveLabelButtonRight(FbTk::TextButton &btn);
    /// move label button to the given location( x and y are relative to the root window)
    void moveLabelButtonTo(FbTk::TextButton &btn, int x, int y);
    /// move the first label button to the left of the second
    void moveLabelButtonLeftOf(FbTk::TextButton &btn, const FbTk::TextButton &dest);
    //move the first label button to the right of the second
    void moveLabelButtonRightOf(FbTk::TextButton &btn, const FbTk::TextButton &dest);
    /// attach a client window for client area
    void setClientWindow(FbTk::FbWindow &win);
    /// remove attached client window
    void removeClient();
    /// redirect events to another eventhandler
    void setEventHandler(FbTk::EventHandler &evh);
    /// remove any handler for the windows
    void removeEventHandler();

    const SizeHints &sizeHints() const { return m_state.size_hints; }
    void setSizeHints(const SizeHints &hint) { m_state.size_hints = hint; }

    void applySizeHints(unsigned int &width, unsigned int &height,
                        bool maximizing = false) const;
    void displaySize(unsigned int width, unsigned int height) const;

    void setDecorationMask(unsigned int mask) { m_state.deco_mask = mask; }
    void applyDecorations(bool do_move = true);
    void applyState();

    // this function translates its arguments according to win_gravity
    // if win_gravity is negative, it does an inverse translation
    void gravityTranslate(int &x, int &y, int win_gravity, unsigned int client_bw, bool move_frame = false);
    void setActiveGravity(int gravity, unsigned int orig_client_bw) { m_state.size_hints.win_gravity = gravity; m_active_orig_client_bw = orig_client_bw; }

    /**
       @name Event handlers
    */
    //@{
    void exposeEvent(XExposeEvent &event);
    void configureNotifyEvent(XConfigureEvent &event);
    void handleEvent(XEvent &event);
    //@}

    void reconfigure();
    void setShapingClient(FbTk::FbWindow *win, bool always_update);
    void updateShape() { m_shape.update(); }

    /**
       @name accessors
    */
    //@{
    int x() const { return m_window.x(); }
    int y() const { return m_window.y(); }
    unsigned int width() const { return m_window.width(); }
    unsigned int height() const { return m_window.height(); }

    // extra bits for tabs
    int xOffset() const;
    int yOffset() const;
    int widthOffset() const;
    int heightOffset() const;

    const FbTk::FbWindow &window() const { return m_window; }
    FbTk::FbWindow &window() { return m_window; }
    /// @return titlebar window
    const FbTk::FbWindow &titlebar() const { return m_titlebar; }
    FbTk::FbWindow &titlebar() { return m_titlebar; }
    const FbTk::FbWindow &label() const { return m_label; }
    FbTk::FbWindow &label() { return m_label; }

    const FbTk::Container &tabcontainer() const { return m_tab_container; }
    FbTk::Container &tabcontainer() { return m_tab_container; }

    /// @return clientarea window
    const FbTk::FbWindow &clientArea() const { return m_clientarea; }
    FbTk::FbWindow &clientArea() { return m_clientarea; }
    /// @return handle window
    const FbTk::FbWindow &handle() const { return m_handle; }
    FbTk::FbWindow &handle() { return m_handle; }
    const FbTk::FbWindow &gripLeft() const { return m_grip_left; }
    FbTk::FbWindow &gripLeft() { return m_grip_left; }
    const FbTk::FbWindow &gripRight() const { return m_grip_right; }
    FbTk::FbWindow &gripRight() { return m_grip_right; }
    bool focused() const { return m_state.focused; }
    FocusableTheme<FbWinFrameTheme> &theme() const { return m_theme; }
    /// @return titlebar height
    unsigned int titlebarHeight() const { return (m_use_titlebar?m_titlebar.height()+m_titlebar.borderWidth():0); }
    unsigned int handleHeight() const { return (m_use_handle?m_handle.height()+m_handle.borderWidth():0); }
    /// @return size of button
    unsigned int buttonHeight() const;
    bool externalTabMode() const { return m_tabmode == EXTERNAL && m_use_tabs; }

    const FbTk::LayerItem &layerItem() const { return m_layeritem; }
    FbTk::LayerItem &layerItem() { return m_layeritem; }

    FbTk::Signal<> &frameExtentSig() { return m_frame_extent_sig; }
    /// @returns true if the window is inside titlebar, 
    /// assuming window is an event window that was generated for this frame.
    bool insideTitlebar(Window win) const;

    /// @returns context for window,
    /// assuming window is an event window that was generated for this frame.
    int getContext(Window win, int x=0, int y=0, int last_x=0, int last_y=0, bool doBorders=false);

    //@}

private:
    void redrawTitlebar();

    /// reposition titlebar items
    void reconfigureTitlebar();
    /**
       @name render helper functions
    */
    //@{
    void renderAll();
    void renderTitlebar();
    void renderHandles();
    void renderTabContainer(); // and labelbuttons

    void renderButtons(); // subset of renderTitlebar - don't call directly

    //@}

    // these return true/false for if something changed
    bool hideTitlebar();
    bool showTitlebar();
    bool hideTabs();
    bool showTabs();
    bool hideHandle();
    bool showHandle();
    bool setBorderWidth(bool do_move = true);

    // check which corners should be rounded
    int getShape() const;

    /**
       @name apply pixmaps depending on focus
    */
    //@{
    void applyAll();
    void applyTitlebar();
    void applyHandles();
    void applyTabContainer(); // and label buttons
    void applyButtons(); // only called within applyTitlebar

#if 0
    void getCurrentFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                               FbTk::Color &label_color, FbTk::Color &title_color);
#endif

    /// initiate inserted button for current theme
    void applyButton(FbTk::Button &btn);

    void alignTabs();
    //@}

    /// initiate some commont variables
    void init();

    BScreen &m_screen;

    FocusableTheme<FbWinFrameTheme> &m_theme; ///< theme to be used
    FbTk::ImageControl &m_imagectrl; ///< Image control for rendering
    WindowState &m_state;

    /**
       @name windows
    */
    //@{
    FbTk::FbWindow m_window; ///< base window that holds each decorations (ie titlebar, handles)
    // want this deleted before the windows in it
    FbTk::LayerItem m_layeritem;

    FbTk::FbWindow m_titlebar; ///<  titlebar window
    FbTk::Container m_tab_container; ///< Holds tabs
    FbTk::TextButton m_label; ///< holds title
    FbTk::FbWindow m_handle; ///< handle between grips
    FbTk::FbWindow m_grip_right,  ///< rightgrip
        m_grip_left; ///< left grip
    FbTk::FbWindow m_clientarea; ///< window that sits behind client window to fill gaps @see setClientWindow
    //@}

    FbTk::Signal<> m_frame_extent_sig;

    typedef std::vector<FbTk::Button *> ButtonList;
    ButtonList m_buttons_left, ///< buttons to the left
        m_buttons_right; ///< buttons to the right
    typedef std::list<FbTk::TextButton *> LabelList;
    int m_bevel;  ///< bevel between titlebar items and titlebar
    bool m_use_titlebar; ///< if we should use titlebar
    bool m_use_tabs; ///< if we should use tabs (turns them off in external mode only)
    bool m_use_handle; ///< if we should use handle
    bool m_visible; ///< if we are currently showing
    ///< do we use screen or window alpha settings ? (0 = window, 1 = default, 2 = default and window never set)

    /**
       @name pixmaps and colors for rendering
    */
    //@{

    // 0-unfocus, 1-focus
    struct Face { Pixmap pm[2]; FbTk::Color color[2]; };
    // 0-unfocus, 1-focus, 2-pressed
    struct BtnFace { Pixmap pm[3]; FbTk::Color color[3]; };

    Face m_title_face;
    Face m_label_face;
    Face m_tabcontainer_face;
    Face m_handle_face;
    Face m_grip_face;
    BtnFace m_button_face;

    //@}

    TabMode m_tabmode;

    unsigned int m_active_orig_client_bw;

    bool m_need_render;
    int m_button_size; ///< size for all titlebar buttons
    int m_alpha[2]; // 0-unfocused, 1-focused

    FbTk::Shape m_shape;
};

#endif // FBWINFRAME_HH
