// FbWinFrame.hh for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbWinFrame.hh,v 1.22 2003/10/02 14:14:46 rathnor Exp $

#ifndef FBWINFRAME_HH
#define FBWINFRAME_HH

#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Observer.hh"
#include "FbTk/Color.hh"
#include "FbTk/FbPixmap.hh"
#include "FbTk/Timer.hh"

#include <vector>
#include <list>
#include <string>
#include <memory>

class Shape;
class FbWinFrameTheme;

namespace FbTk {
class TextButton;
class ImageControl;
class Command;
class Button;
class Texture;
};

/// holds a window frame with a client window
/// (see: <a href="fluxbox_fbwinframe.png">image</a>)
class FbWinFrame:public FbTk::EventHandler {
public:

    /// create a top level window
    FbWinFrame(FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, 
               int screen_num, int x, int y,
               unsigned int width, unsigned int height);

    /// create a frame window inside another FbWindow, NOT IMPLEMENTED!
    FbWinFrame(FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, 
               const FbTk::FbWindow &parent,
               int x, int y, 
               unsigned int width, unsigned int height);

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
    void resizeForClient(unsigned int width, unsigned int height);

    // for when there needs to be an atomic move+resize operation
    void moveResizeForClient(int x, int y, unsigned int width, unsigned int height, bool move = true, bool resize = true);

    // can elect to ignore move or resize (mainly for use of move/resize individual functions
    void moveResize(int x, int y, unsigned int width, unsigned int height, bool move = true, bool resize = true);

    /// set focus/unfocus style
    void setFocus(bool newvalue);
    void setDoubleClickTime(unsigned int time);

    /// add a button to the left of the label
    void addLeftButton(FbTk::Button *btn);
    /// add a button to the right of the label
    void addRightButton(FbTk::Button *btn);
    /// remove all buttons from titlebar
    void removeAllButtons();
    /// adds a button to label window
    void addLabelButton(FbTk::TextButton &btn);
    /// removes a specific button from label window
    void removeLabelButton(FbTk::TextButton &btn);
    /// move label button to the left
    void moveLabelButtonLeft(const FbTk::TextButton &btn);
    /// move label button to the right
    void moveLabelButtonRight(const FbTk::TextButton &btn);
    /// which button is to be rendered focused
    void setLabelButtonFocus(FbTk::TextButton &btn);
    /// attach a client window for client area
    void setClientWindow(Window win);
    /// same as above but with FbWindow
    void setClientWindow(FbTk::FbWindow &win);
    /// remove attached client window
    void removeClient();
    /// redirect events to another eventhandler
    void setEventHandler(FbTk::EventHandler &evh);
    /// remove any handler for the windows
    void removeEventHandler();

    void hideTitlebar();
    void showTitlebar();
    void hideHandle();
    void showHandle();
    void hideAllDecorations();
    void showAllDecorations();

    // this function translates its arguments according to win_gravity
    // if win_gravity is negative, it does an inverse translation
    void gravityTranslate(int &x, int &y, int win_gravity, bool move_frame = false);

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

    void setUpdateDelayTime(long t) { m_update_timer.setTimeout(t); }

    /**
       @name accessors
    */
    //@{
    inline int x() const { return m_window.x(); }
    inline int y() const { return m_window.y(); }
    inline unsigned int width() const { return m_window.width(); }
    inline unsigned int height() const { return m_window.height(); }
    inline const FbTk::FbWindow &window() const { return m_window; }
    inline FbTk::FbWindow &window() { return m_window; }
    /// @return titlebar window
    inline const FbTk::FbWindow &titlebar() const { return m_titlebar; }
    inline FbTk::FbWindow &titlebar() { return m_titlebar; }
    inline const FbTk::FbWindow &label() const { return m_label; }
    inline FbTk::FbWindow &label() { return m_label; }
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
    inline const FbTk::TextButton *currentLabel() const { return m_current_label; }
    inline bool focused() const { return m_focused; }
    inline bool isShaded() const { return m_shaded; }
    inline const FbWinFrameTheme &theme() const { return m_theme; }
    /// @return titlebar height
    unsigned int titlebarHeight() const { return m_titlebar.height(); }
    /// @return size of button
    unsigned int buttonHeight() const;

    //@}

private:
    void redrawTitlebar();
    void redrawTitle();

    /// reposition titlebar items
    void reconfigureTitlebar();
    /**
       @name render helper functions
    */
    //@{
    void renderTitlebar();
    void renderHandles();
    void renderButtons();
    void renderButtonFocus(FbTk::TextButton &button);
    void renderButtonUnfocus(FbTk::TextButton &button);
    void renderLabel();
    /// renders to pixmap or sets color
    void render(const FbTk::Texture &tex, FbTk::Color &col, Pixmap &pm,
                unsigned int width, unsigned int height);
    void getUnfocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                          FbTk::Color &label_color, FbTk::Color &title_color);
    void getCurrentFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                               FbTk::Color &label_color, FbTk::Color &title_color);
    void renderLabelButtons();
    //@}

    /// initiate some commont variables
    void init();
    /// initiate inserted buttons for current theme
    void setupButton(FbTk::Button &btn);
    void updateTransparent();

    FbWinFrameTheme &m_theme; ///< theme to be used 
    FbTk::ImageControl &m_imagectrl; ///< Image control for rendering
    /**
       @name windows
    */
    //@{
    FbTk::FbWindow m_window; ///< base window that holds each decorations (ie titlebar, handles)
    FbTk::FbWindow m_titlebar; ///<  titlebar window
    FbTk::FbWindow m_label; ///< holds title
    FbTk::FbWindow m_handle; ///< handle between grips
    FbTk::FbWindow m_grip_right,  ///< rightgrip
        m_grip_left; ///< left grip
    FbTk::FbWindow m_clientarea; ///< window that holds client window @see setClientWindow
    //@}
    typedef std::vector<FbTk::Button *> ButtonList;
    ButtonList m_buttons_left, ///< buttons to the left
        m_buttons_right; ///< buttons to the right
    typedef std::list<FbTk::TextButton *> LabelList;
    LabelList m_labelbuttons; ///< holds label buttons inside label window
    FbTk::TextButton *m_current_label; ///< which client button is focused at the moment
    std::string m_titletext; ///< text to be displayed int m_label
    int m_bevel;  ///< bevel between titlebar items and titlebar
    bool m_use_titlebar; ///< if we should use titlebar
    bool m_use_handle; ///< if we should use handle
    bool m_focused; ///< focused/unfocused mode
    bool m_visible; ///< if we are currently showing

    /**
       @name pixmaps and colors for rendering
    */
    //@{
    Pixmap m_title_focused_pm; ///< pixmap for focused title
    FbTk::Color m_title_focused_color; ///< color for focused title
    Pixmap m_title_unfocused_pm; ///< pixmap for unfocused title
    FbTk::Color m_title_unfocused_color; ///< color for unfocued title

    Pixmap m_label_focused_pm; ///< pixmap for focused label
    FbTk::Color m_label_focused_color; ///< color for focused label
    Pixmap m_label_unfocused_pm; ///< pixmap for unfocused label
    FbTk::Color m_label_unfocused_color; ///< color for unfocued label
    
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

    int m_button_size; ///< size for all titlebar buttons
    unsigned int m_width_before_shade,  ///< width before shade, so we can restore it when we unshade
        m_height_before_shade; ///< height before shade, so we can restore it when we unshade
    bool m_shaded; ///< wheter we're shaded or not
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
        void update(FbTk::Subject *subj) {
            m_frame.reconfigure();
        }
    private:
        FbWinFrame &m_frame;
    };
    ThemeListener m_themelistener;
    std::auto_ptr<Shape> m_shape;
    bool m_disable_shape;
    FbTk::Timer m_update_timer;
};

#endif // FBWINFRAME_HH
