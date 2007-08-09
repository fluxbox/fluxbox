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

// $Id$

#ifndef FBWINFRAME_HH
#define FBWINFRAME_HH

#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Observer.hh"
#include "FbTk/Color.hh"
#include "FbTk/FbPixmap.hh"
#include "FbTk/XLayerItem.hh"
#include "FbTk/TextButton.hh"
#include "Container.hh"
#include "Shape.hh"

#include <vector>
#include <list>
#include <string>
#include <memory>

class Shape;
class FbWinFrameTheme;
class BScreen;
class IconButton;
class Focusable;

namespace FbTk {
class TextButton;
class ImageControl;
class Command;
class Button;
class Texture;
class XLayer;
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
        TOPLEFT = 1, BOTTOMLEFT,
        TOPRIGHT, BOTTOMRIGHT,
        // left and right placement
        LEFTBOTTOM, LEFTTOP,
        RIGHTBOTTOM, RIGHTTOP        
    };


    /// create a top level window
    FbWinFrame(BScreen &screen, FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl,
               FbTk::XLayer &layer,
               int x, int y,
               unsigned int width, unsigned int height);

/*    /// create a frame window inside another FbWindow, NOT IMPLEMENTED!
    FbWinFrame(BScreen &screen, FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, 
               const FbTk::FbWindow &parent,
               int x, int y, 
               unsigned int width, unsigned int height);
*/
    /// destroy frame
    ~FbWinFrame();

    /// setup actions for titlebar
    bool setOnClickTitlebar(FbTk::RefCount<FbTk::Command> &cmd, int button_num,
                            bool double_click=false, bool pressed=false);

    void hide();
    void show();
    inline bool isVisible() const { return m_visible; }
    /// shade frame (ie resize to titlebar size)
    void shade();
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
                    bool move = true, bool resize = true);

    // move without transparency or special effects (generally when dragging)
    void quietMoveResize(int x, int y, 
                         unsigned int width, unsigned int height);

    /// some outside move/resize happened, and we need to notify all of our windows
    /// in case of transparency
    void notifyMoved(bool clear);
    void clearAll();

    /// set focus/unfocus style
    void setFocus(bool newvalue);
    inline void setFocusTitle(const std::string &str) { m_label.setText(str); }
    void setDoubleClickTime(unsigned int time);
    bool setTabMode(TabMode tabmode);
    inline void updateTabProperties() { alignTabs(); }

    /// Alpha settings
    void setAlpha(bool focused, unsigned char value);
    unsigned char getAlpha(bool focused) const;

    void setUseDefaultAlpha(bool use_default);
    bool getUseDefaultAlpha() const { return m_use_default_alpha; }

    /// add a button to the left of the label
    void addLeftButton(FbTk::Button *btn);
    /// add a button to the right of the label
    void addRightButton(FbTk::Button *btn);
    /// remove all buttons from titlebar
    void removeAllButtons();
    /// adds a button to label window with specified title and command
    IconButton *createTab(Focusable &client);
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
    /// which button is to be rendered focused
    void setLabelButtonFocus(IconButton &btn);
    /// attach a client window for client area
    void setClientWindow(FbTk::FbWindow &win);
    /// remove attached client window
    void removeClient();
    /// redirect events to another eventhandler
    void setEventHandler(FbTk::EventHandler &evh);
    /// remove any handler for the windows
    void removeEventHandler();

    // these return true/false for if something changed
    bool hideTitlebar();
    bool showTitlebar();
    bool hideTabs();
    bool showTabs();
    bool hideHandle();
    bool showHandle();
    bool hideAllDecorations();
    bool showAllDecorations();

    // this function translates its arguments according to win_gravity
    // if win_gravity is negative, it does an inverse translation
    void gravityTranslate(int &x, int &y, int win_gravity, unsigned int client_bw, bool move_frame = false);
    void setActiveGravity(int gravity, unsigned int orig_client_bw) { m_active_gravity = gravity; m_active_orig_client_bw = orig_client_bw; }
    void setBorderWidth(unsigned int borderW);

    /**
       @name Event handlers
    */
    //@{
    void buttonPressEvent(XButtonEvent &event);
    void buttonReleaseEvent(XButtonEvent &event);
    void exposeEvent(XExposeEvent &event);
    void configureNotifyEvent(XConfigureEvent &event);
    void handleEvent(XEvent &event);
    //@}
 
    void reconfigure();
    void setUseShape(bool value);
    void setShapingClient(FbTk::FbWindow *win, bool always_update);
    void updateShape() { m_shape.update(); }

    /**
       @name accessors
    */
    //@{
    inline int x() const { return m_window.x(); }
    inline int y() const { return m_window.y(); }
    inline unsigned int width() const { return m_window.width(); }
    inline unsigned int height() const { return m_window.height(); }
    unsigned int normalHeight() const;

    // extra bits for tabs
    int xOffset() const;
    int yOffset() const;
    int widthOffset() const;
    int heightOffset() const;

    inline const FbTk::FbWindow &window() const { return m_window; }
    inline FbTk::FbWindow &window() { return m_window; }
    /// @return titlebar window
    inline const FbTk::FbWindow &titlebar() const { return m_titlebar; }
    inline FbTk::FbWindow &titlebar() { return m_titlebar; }
    inline const FbTk::FbWindow &label() const { return m_label; }
    inline FbTk::FbWindow &label() { return m_label; }

    inline const Container &tabcontainer() const { return m_tab_container; }
    inline Container &tabcontainer() { return m_tab_container; }
    
    /// @return clientarea window
    inline const FbTk::FbWindow &clientArea() const { return m_clientarea; }
    inline FbTk::FbWindow &clientArea() { return m_clientarea; }
    /// @return handle window
    inline const FbTk::FbWindow &handle() const { return m_handle; }
    inline FbTk::FbWindow &handle() { return m_handle; }
    inline const FbTk::FbWindow &gripLeft() const { return m_grip_left; }
    inline FbTk::FbWindow &gripLeft() { return m_grip_left; }
    inline const FbTk::FbWindow &gripRight() const { return m_grip_right; }
    inline FbTk::FbWindow &gripRight() { return m_grip_right; }
    inline const IconButton *currentLabel() const { return m_current_label; }
    inline bool focused() const { return m_focused; }
    inline bool isShaded() const { return m_shaded; }
    inline FbWinFrameTheme &theme() const { return m_theme; }
    /// @return titlebar height
    unsigned int titlebarHeight() const { return (m_use_titlebar?m_titlebar.height()+m_window.borderWidth():0); }
    unsigned int handleHeight() const { return (m_use_handle?m_handle.height()+m_window.borderWidth():0); }
    /// @return size of button
    unsigned int buttonHeight() const;
    bool externalTabMode() const { return m_tabmode == EXTERNAL && m_use_tabs; }

    inline const FbTk::XLayerItem &layerItem() const { return m_layeritem; }
    inline FbTk::XLayerItem &layerItem() { return m_layeritem; }

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

    /// renders to pixmap or sets color
    void render(const FbTk::Texture &tex, FbTk::Color &col, Pixmap &pm,
                unsigned int width, unsigned int height, FbTk::Orientation orient = FbTk::ROT0);

    //@}

    /**
       @name apply pixmaps depending on focus
    */
    //@{
    void applyAll();
    void applyTitlebar();
    void applyHandles();
    void applyTabContainer(); // and label buttons
    void applyButtons(); // only called within applyTitlebar

    void getCurrentFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                               FbTk::Color &label_color, FbTk::Color &title_color);

    /// initiate inserted button for current theme
    void applyButton(FbTk::Button &btn);

    void alignTabs();
    //@}

    /// initiate some commont variables
    void init();

    BScreen &m_screen;

    FbWinFrameTheme &m_theme; ///< theme to be used 
    FbTk::ImageControl &m_imagectrl; ///< Image control for rendering
    /**
       @name windows
    */
    //@{
    FbTk::FbWindow m_window; ///< base window that holds each decorations (ie titlebar, handles)
    // want this deleted before the windows in it
    FbTk::XLayerItem m_layeritem;

    FbTk::FbWindow m_titlebar; ///<  titlebar window
    Container      m_tab_container; ///< Holds tabs
    FbTk::TextButton m_label; ///< holds title
    FbTk::FbWindow m_handle; ///< handle between grips
    FbTk::FbWindow m_grip_right,  ///< rightgrip
        m_grip_left; ///< left grip
    FbTk::FbWindow m_clientarea; ///< window that sits behind client window to fill gaps @see setClientWindow
    //@}
    typedef std::vector<FbTk::Button *> ButtonList;
    ButtonList m_buttons_left, ///< buttons to the left
        m_buttons_right; ///< buttons to the right
    typedef std::list<FbTk::TextButton *> LabelList;
    IconButton *m_current_label; ///< which client button is focused at the moment
    int m_bevel;  ///< bevel between titlebar items and titlebar
    bool m_use_titlebar; ///< if we should use titlebar
    bool m_use_tabs; ///< if we should use tabs (turns them off in external mode only)
    bool m_use_handle; ///< if we should use handle
    bool m_focused; ///< focused/unfocused mode
    bool m_visible; ///< if we are currently showing
    unsigned char m_use_default_alpha;   
    ///< do we use screen or window alpha settings ? (0 = window, 1 = default, 2 = default and window never set)

    /**
       @name pixmaps and colors for rendering
    */
    //@{
    Pixmap m_title_focused_pm; ///< pixmap for focused title
    FbTk::Color m_title_focused_color; ///< color for focused title
    Pixmap m_title_unfocused_pm; ///< pixmap for unfocused title
    FbTk::Color m_title_unfocused_color; ///< color for unfocused title

    Pixmap m_label_focused_pm; ///< pixmap for focused label (only visible with external tabs)
    FbTk::Color m_label_focused_color; ///< color for focused label
    Pixmap m_label_unfocused_pm; ///< pixmap for unfocused label
    FbTk::Color m_label_unfocused_color; ///< color for unfocused label

    Pixmap m_tabcontainer_focused_pm; ///< pixmap for focused tab container
    FbTk::Color m_tabcontainer_focused_color; ///< color for focused tab container
    Pixmap m_tabcontainer_unfocused_pm; ///< pixmap for unfocused tab container
    FbTk::Color m_tabcontainer_unfocused_color; ///< color for unfocused tab container

    FbTk::Color m_handle_focused_color, m_handle_unfocused_color;
    Pixmap m_handle_focused_pm, m_handle_unfocused_pm;
    

    Pixmap m_button_pm;  ///< normal button     
    FbTk::Color m_button_color; ///< normal color button
    Pixmap m_button_unfocused_pm; ///< unfocused button
    FbTk::Color m_button_unfocused_color; ///< unfocused color button
    Pixmap m_button_pressed_pm; ///< pressed button
    FbTk::Color m_button_pressed_color; ///< pressed button color

    Pixmap m_grip_focused_pm;
    FbTk::Color m_grip_focused_color; ///< if no pixmap is given for grip, use this color
    Pixmap m_grip_unfocused_pm; ///< unfocused pixmap for grip
    FbTk::Color m_grip_unfocused_color; ///< unfocused color for grip if no pixmap is given
    //@}

    TabMode m_tabmode;

    // last gravity that this window was *actively* placed with
    int m_active_gravity;
    unsigned int m_active_orig_client_bw;

    bool m_need_render;
    int m_button_size; ///< size for all titlebar buttons
    unsigned int m_height_before_shade; ///< height before shade, so we can restore it when we unshade
    bool m_shaded; ///< wheter we're shaded or not
    unsigned char  m_focused_alpha; ///< focused alpha value
    unsigned char  m_unfocused_alpha; ///< unfocused alpha value
    unsigned int m_double_click_time; ///< the time period that's considerd to be a double click
    struct MouseButtonAction {
        FbTk::RefCount<FbTk::Command> click; ///< what to do when we release mouse button
        FbTk::RefCount<FbTk::Command> click_pressed; ///< what to do when we press mouse button
        FbTk::RefCount<FbTk::Command> double_click; ///< what to do when we double click
    };
    MouseButtonAction m_commands[5]; ///< hardcoded to five ... //!! TODO, change this

    class ThemeListener: public FbTk::Observer {
    public:
        ThemeListener(FbWinFrame &frame):m_frame(frame) { }
        void update(FbTk::Subject *) {
            m_frame.reconfigure();
        }
    private:
        FbWinFrame &m_frame;
    };
    ThemeListener m_themelistener;
    Shape m_shape;

    bool m_disable_themeshape;
};

#endif // FBWINFRAME_HH
