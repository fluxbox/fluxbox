// FbWinFrame.cc for Fluxbox Window Manager
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

// $Id: FbWinFrame.cc,v 1.6 2003/02/15 01:54:54 fluxgen Exp $

#include "FbWinFrame.hh"
#include "ImageControl.hh"
#include "EventManager.hh"
#include "App.hh"

#include <iostream>
using namespace std;

FbWinFrame::FbWinFrame(FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, int screen_num, int x, int y,
                       unsigned int width, unsigned int height):
    m_theme(theme),
    m_imagectrl(imgctrl),
    m_window(screen_num, x, y, width, height,  ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | EnterWindowMask, true),
    m_titlebar(m_window, 0, 0, 100, 16, 
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | ExposureMask |
               EnterWindowMask | LeaveWindowMask),
    m_label(m_titlebar, 0, 0, 100, 16,
	    ButtonPressMask | ButtonReleaseMask |
            ButtonMotionMask | ExposureMask |
            EnterWindowMask | LeaveWindowMask),
    m_grip_right(m_window, 0, 0, 10, 4,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_grip_left(m_window, 0, 0, 10, 4,
		ButtonPressMask | ButtonReleaseMask |
		ButtonMotionMask | ExposureMask |
		EnterWindowMask | LeaveWindowMask),
    m_handle(m_window, 0, 0, 100, 5,
             ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | ExposureMask |
             EnterWindowMask | LeaveWindowMask),
    m_clientarea(m_window, 0, 0, 100, 100,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_clientwin(0),
    m_bevel(1),
    m_use_titlebar(true), 
    m_use_handle(true),
    m_button_pm(0),
    m_themelistener(*this) {
    theme.addListener(m_themelistener);
    init();
}
/*
  FbWinFrame::FbWinFrame(FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, const FbTk::FbWindow &parent, int x, int y,
  unsigned int width, unsigned int height):
  m_theme(theme),
  m_imagectrl(imgctrl),
  m_window(parent, x, y, width, height, ExposureMask | StructureNotifyMask),
  m_titlebar(m_window, 0, 0, 100, 16, 
  ExposureMask | ButtonPressMask | ButtonReleaseMask),
  m_label(m_titlebar, 0, 0, 100, 16,
  ExposureMask | ButtonPressMask | ButtonReleaseMask),
  m_grip_right(m_window, 0, 0, 100, 100, ExposureMask | ButtonPressMask | ButtonReleaseMask),
  m_grip_left(m_window, 0, 0, 100, 100, ExposureMask | ButtonPressMask | ButtonReleaseMask),
  m_handle(m_window, 0, 0, 100, 100, ExposureMask | ButtonPressMask | ButtonReleaseMask),
  m_clientarea(m_window, 0, 0, 100, 100, SubstructureRedirectMask),
  m_clientwin(0),
  m_bevel(1),
  m_use_titlebar(true), 
  m_use_handles(true),
  m_button_pm(0) {

  init();
  }
*/

FbWinFrame::~FbWinFrame() {
    removeEventHandler();
    removeAllButtons();

}

bool FbWinFrame::setOnClickTitlebar(FbTk::RefCount<FbTk::Command> &ref, int mousebutton_num, 
                            bool double_click, bool pressed) {
    // find mousebutton_num
    if (mousebutton_num < 1 || mousebutton_num > 5)
        return false;
    if (double_click)
        m_commands[mousebutton_num - 1].double_click = ref;
    else {
        if (pressed)
            m_commands[mousebutton_num - 1].click_pressed = ref;
        else
            m_commands[mousebutton_num - 1].click = ref;
    }

    return true;
}

void FbWinFrame::hide() {
    m_window.hide();
}

void FbWinFrame::show() {
    m_window.showSubwindows();
    m_window.show();
    reconfigure();
}

/**
 Toggle shade state, and resize window
 */
void FbWinFrame::shade() {
    if (!m_shaded) {
        m_width_before_shade = m_window.width();
        m_height_before_shade = m_window.height();
        m_window.resize(m_window.width(), m_titlebar.height() + 2*m_titlebar.borderWidth());
    } else {
        m_window.resize(m_width_before_shade, m_height_before_shade);
        m_grip_left.clear();
        m_grip_right.clear();
        m_handle.clear();
    }
    // toggle shade
    m_shaded = !m_shaded;
}


void FbWinFrame::move(int x, int y) {
    m_window.move(x, y);
}

void FbWinFrame::resize(unsigned int width, unsigned int height) {
    // update unshaded size if  we're in shaded state and just resize width
    if (m_shaded) {
        m_width_before_shade = width;
        m_height_before_shade = height;
        m_window.resize(width, m_window.height());
    } else {
        m_window.resize(width, height);
    }

    reconfigure();
}

void FbWinFrame::resizeForClient(unsigned int width, unsigned int height) {
    // total height for frame without client
    unsigned int total_height = m_handle.height() + m_titlebar.height();
    // resize frame height with total height + specified height
    if (!m_use_titlebar)
        total_height -= m_titlebar.height();
    if (!m_use_handle)
        total_height -= m_handle.height();
    resize(width, total_height + height);
}

void FbWinFrame::moveResize(int x, int y, unsigned int width, unsigned int height) {
    move(x, y);
    resize(width, height);
}

void FbWinFrame::setTitle(const std::string &titletext) {
    m_titletext = titletext;
    redrawTitle();
}

void FbWinFrame::setFocus(bool newvalue) {
    if (m_focused == newvalue) // no need to change focus
        return;

    m_focused = newvalue;
    reconfigure(); // reconfigure rendering for new focus value
}

void FbWinFrame::setDoubleClickTime(unsigned int time) {
    m_double_click_time = time;
}

void FbWinFrame::setBevel(int bevel) {
    m_bevel = bevel;
    reconfigure();
}

void FbWinFrame::addLeftButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    setupButton(*btn); // setup theme and other stuff

    m_buttons_left.push_back(btn);
    reconfigureTitlebar();
}

void FbWinFrame::addRightButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    setupButton(*btn); // setup theme and other stuff

    m_buttons_right.push_back(btn);
    reconfigureTitlebar();
}

void FbWinFrame::removeAllButtons() {
    // destroy left side
    while (!m_buttons_left.empty()) {
        delete m_buttons_left.back();
        m_buttons_left.pop_back();
    }
    // destroy right side
    while (!m_buttons_right.empty()) {
        delete m_buttons_right.back();
        m_buttons_right.pop_back();
    }

    // update titlebar
    reconfigureTitlebar();
}

void FbWinFrame::setClientWindow(Window win) {
    m_clientwin = win;
    Display *display = FbTk::App::instance()->display();
    XSetWindowBorderWidth(display, win, 0);

    XChangeSaveSet(display, win, SetModeInsert);
    XSetWindowAttributes attrib_set;
    // no events for client window while we reparent it
    XSelectInput(display, m_clientarea.window(), NoEventMask);
    XReparentWindow(display, win, m_clientarea.window(), 0, 0);
    XSelectInput(display, m_clientarea.window(), SubstructureRedirectMask);

    XFlush(display);

    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask | FocusChangeMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

    XChangeWindowAttributes(display, win, CWEventMask|CWDontPropagate, &attrib_set);

    m_clientarea.raise();
    m_clientarea.showSubwindows();

}

void FbWinFrame::removeClient() {
    m_clientwin = 0;
}

void FbWinFrame::hideTitlebar() {
    m_titlebar.hide();
    m_use_titlebar = false;
}

void FbWinFrame::showTitlebar() {
    m_titlebar.show();
    m_use_titlebar = true;
}

void FbWinFrame::hideHandle() {
    m_handle.hide();
    m_grip_left.hide();
    m_grip_right.hide();
    m_use_handle = false;
}

void FbWinFrame::showHandle() {
    m_handle.show();
    m_grip_left.show();
    m_grip_right.show();
    m_use_handle = true;
}

void FbWinFrame::hideAllDecorations() {
    hideHandle();
    hideTitlebar();
    resizeForClient(m_clientarea.width(), m_clientarea.height() - m_window.borderWidth());
    reconfigure();
}

void FbWinFrame::showAllDecorations() {
    if (!m_use_handle)
        showHandle();
    if (!m_use_titlebar)
        showTitlebar();
    resizeForClient(m_clientarea.width(), m_clientarea.height() - m_window.borderWidth());
}

/**
   Set new event handler for the frame's windows
*/
void FbWinFrame::setEventHandler(FbTk::EventHandler &evh) {

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(evh, m_label);
    evm.add(evh, m_titlebar);
    evm.add(evh, m_handle);
    evm.add(evh, m_grip_right);
    evm.add(evh, m_grip_left);
    evm.add(evh, m_window);
    evm.add(evh, m_clientarea);
}

/**
   remove event handler from windows
*/
void FbWinFrame::removeEventHandler() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(m_label);
    evm.remove(m_titlebar);
    evm.remove(m_handle);
    evm.remove(m_grip_right);
    evm.remove(m_grip_left);
    evm.remove(m_window);
    evm.remove(m_clientarea);
}

void FbWinFrame::buttonPressEvent(XButtonEvent &event) {
    if (event.window != m_titlebar.window() &&
        event.window != m_label.window())
        return;

    if (event.button > 5 || event.button < 1)
        return;

    if (*m_commands[event.button - 1].click_pressed)
        m_commands[event.button - 1].click_pressed->execute();
}

void FbWinFrame::buttonReleaseEvent(XButtonEvent &event) {
    if (event.window != m_titlebar.window() &&
        event.window != m_label.window())
        return;

    if (event.button < 1 || event.button > 5)
        return;

    static int last_release_time = 0;
    bool double_click = (event.time - last_release_time <= m_double_click_time);
    last_release_time = event.time;
    int real_button = event.button - 1;
    
    if (double_click && *m_commands[real_button].double_click)
        m_commands[real_button].double_click->execute();
    else if (*m_commands[real_button].click)
        m_commands[real_button].click->execute();

}

void FbWinFrame::exposeEvent(XExposeEvent &event) {
    if (m_titlebar == event.window)
        redrawTitlebar();
    else if (m_label == event.window) 
        redrawTitle();
}

void FbWinFrame::handleEvent(XEvent &event) {
    if (event.type == ConfigureNotify)
        configureNotifyEvent(event.xconfigure);
}

void FbWinFrame::configureNotifyEvent(XConfigureEvent &event) {
    resize(event.width, event.height);
}

void FbWinFrame::reconfigure() {
    m_window.clear();

    // align titlebar and render it
    if (m_use_titlebar)
        reconfigureTitlebar();
    // setup client area size/pos
    int next_y =  m_titlebar.height() + 2*m_titlebar.borderWidth();
    unsigned int client_height = m_window.height() - m_titlebar.height() - m_handle.height();

    if (!m_use_titlebar) {
        next_y = 0;
        if (!m_use_handle)
            client_height = m_window.height();
        else
            client_height = m_window.height() - m_handle.height();
    }

    m_clientarea.moveResize(0, next_y,
                            m_window.width(), client_height);

    if (m_clientwin != 0) {
        XMoveResizeWindow(FbTk::App::instance()->display(), m_clientwin,
                          0, 0,
                          m_clientarea.width(), m_clientarea.height());
    }

    if (!m_use_handle) // no need to do anything more
        return;

    // align handle and grips
    const int grip_height = m_handle.height();
    const int grip_width = 20; //TODO
    
    const int ypos = m_window.height() - grip_height;

    m_grip_left.moveResize(0, ypos,
                           grip_width, grip_height);

    m_handle.moveResize(grip_width, ypos,
                        m_window.width() - grip_width*2, grip_height);
   
    m_grip_right.moveResize(m_window.width() - grip_width, ypos,
                            grip_width, grip_height);

    
    // render the theme
    renderButtons();
    renderHandles();
}

unsigned int FbWinFrame::titleHeight() const {
    return m_theme.font().height();
}

unsigned int FbWinFrame::buttonHeight() const {
    return m_titlebar.height() - m_bevel*2;
}


//--------------------- private area

/**
   aligns and redraws title
*/
void FbWinFrame::redrawTitle() {
    GC gc = m_theme.labelTextFocusGC();
    m_label.clear(); // clear window
    unsigned int textlen = m_titletext.size();
    const FbTk::Font &font = m_theme.font();
    // do text alignment
    int align_x = FbTk::doAlignment(m_label.width(),
                                    m_bevel,
                                    m_theme.justify(),
                                    font,
                                    m_titletext.c_str(), m_titletext.size(),
                                    textlen // return new text len
                                    );

    font.drawText(m_label.window(), // drawable
                  m_window.screenNumber(),
                  gc, // graphic context
                  m_titletext.c_str(), textlen, // string and string size
                  align_x, font.ascent());// position
}

void FbWinFrame::redrawTitlebar() {
    m_titlebar.clear();
    m_label.clear();
    redrawTitle();
}

/**
   Align buttons with title text window
*/
void FbWinFrame::reconfigureTitlebar() {
    // resize titlebar to window size with font height
    m_titlebar.resize(m_window.width() - m_titlebar.borderWidth(), m_theme.font().height() == 0 ? 
                      16 : m_theme.font().height() + m_bevel*2 + 2);

    // draw left buttons first
    unsigned int next_x = m_bevel; 
    unsigned int button_size = buttonHeight();
    m_button_size = button_size;
    for (size_t i=0; i < m_buttons_left.size(); i++, next_x += button_size + m_bevel) {
        m_buttons_left[i]->moveResize(next_x, m_bevel, 
                                      button_size, button_size);
    }
	
    next_x += m_bevel;
	
    // space left on titlebar between left and right buttons
    unsigned int space_left = m_titlebar.width() - next_x;
    if (m_buttons_right.size() != 0)
        space_left -= (m_buttons_right.size() + 1)*button_size;
    space_left -= m_bevel;
	
    m_label.moveResize(
                       next_x, m_bevel,
                       space_left, button_size);

    next_x += m_label.width() + m_bevel;;

    // finaly set new buttons to the right
    for (size_t i=0; i < m_buttons_right.size(); 
         ++i, next_x += button_size + m_bevel) {
        m_buttons_right[i]->moveResize(next_x, m_bevel,
                                       button_size, button_size);
    }

    renderTitlebar();
    m_titlebar.raise(); // always on top
}

void FbWinFrame::renderTitlebar() {
    if (!m_use_titlebar)
        return;

    // render pixmaps

    render(m_theme.titleFocusTexture(), m_title_focused_color, 
           m_title_focused_pm,
           m_titlebar.width(), m_titlebar.height());

    render(m_theme.titleUnfocusTexture(), m_title_unfocused_color, 
           m_title_unfocused_pm,
           m_titlebar.width(), m_titlebar.height());


    render(m_theme.labelFocusTexture(), m_label_focused_color, 
           m_label_focused_pm,
           m_label.width(), m_label.height());
		
    render(m_theme.labelUnfocusTexture(), m_label_unfocused_color, 
           m_label_unfocused_pm,
           m_label.width(), m_label.height());

    // finaly set up pixmaps for titlebar windows

    if (m_focused) {
        if (m_label_focused_pm != 0)
            m_label.setBackgroundPixmap(m_label_focused_pm);
        else
            m_label.setBackgroundColor(m_label_focused_color);
			
        if (m_title_focused_pm != 0)
            m_titlebar.setBackgroundPixmap(m_title_focused_pm);
        else
            m_titlebar.setBackgroundColor(m_title_focused_color);
			
    } else {
        if (m_label_unfocused_pm != 0)
            m_label.setBackgroundPixmap(m_label_unfocused_pm);
        else
            m_label.setBackgroundColor(m_label_unfocused_color);
			
        if (m_title_unfocused_pm != 0)
            m_titlebar.setBackgroundPixmap(m_title_unfocused_pm);
        else
            m_titlebar.setBackgroundColor(m_title_unfocused_color);

    }

    redrawTitle();
}


void FbWinFrame::renderHandles() {
    if (!m_use_handle)
        return;
    render(m_theme.handleFocusTexture(), m_handle_focused_color, 
           m_handle_focused_pm,
           m_handle.width(), m_handle.height());
	
    render(m_theme.handleUnfocusTexture(), m_handle_unfocused_color, 
           m_handle_unfocused_pm,
           m_handle.width(), m_handle.height());

    if (m_focused) {
        if (m_handle_focused_pm) {
            m_handle.setBackgroundPixmap(m_handle_focused_pm);
        } else {
            m_handle.setBackgroundColor(m_handle_focused_color);
        }                
    } else {
        if (m_handle_unfocused_pm) {
            m_handle.setBackgroundPixmap(m_handle_unfocused_pm);
        } else {
            m_handle.setBackgroundColor(m_handle_unfocused_color);
        }                
    }

    render(m_theme.gripFocusTexture(), m_grip_focused_color, m_grip_focused_pm,
           m_grip_left.width(), m_grip_left.height());

    render(m_theme.gripUnfocusTexture(), m_grip_unfocused_color, 
           m_grip_unfocused_pm,
           m_grip_left.width(), m_grip_left.height());

    if (m_focused) {
        if (m_grip_focused_pm) {
            m_grip_left.setBackgroundPixmap(m_grip_focused_pm);
            m_grip_right.setBackgroundPixmap(m_grip_focused_pm);
        } else {
            m_grip_left.setBackgroundColor(m_grip_focused_color);
            m_grip_right.setBackgroundColor(m_grip_focused_color);
        }                
    } else {
        if (m_grip_unfocused_pm) {
            m_grip_left.setBackgroundPixmap(m_grip_unfocused_pm);
            m_grip_right.setBackgroundPixmap(m_grip_unfocused_pm);
        } else {
            m_grip_left.setBackgroundColor(m_grip_unfocused_color);
            m_grip_right.setBackgroundColor(m_grip_unfocused_color);
        }                
    }
    /*
      TODO: set border color
    */
    m_grip_left.clear();
    m_grip_right.clear();
    m_handle.clear();
}

void FbWinFrame::renderButtons() {

    render(m_theme.buttonFocusTexture(), m_button_color, m_button_pm,
           m_button_size, m_button_size);
		
    render(m_theme.buttonUnfocusTexture(), m_button_unfocused_color, 
           m_button_unfocused_pm,
           m_button_size, m_button_size);
		
    render(m_theme.buttonPressedTexture(), m_button_pressed_color, 
           m_button_pressed_pm,
           m_button_size, m_button_size);

    // setup left and right buttons
    for (size_t i=0; i < m_buttons_left.size(); ++i)
        setupButton(*m_buttons_left[i]);

    for (size_t i=0; i < m_buttons_right.size(); ++i)
        setupButton(*m_buttons_right[i]);

}

void FbWinFrame::init() {
    // clear pixmaps
    m_title_focused_pm = m_title_unfocused_pm = 0;
    m_label_focused_pm = m_label_unfocused_pm = 0;
    m_button_unfocused_pm = m_button_pressed_pm = 0;
    m_double_click_time = 200;
    m_button_pm = 0;
    m_button_size = 26;
    m_handle_focused_pm = 
        m_handle_unfocused_pm = 0;
    m_grip_unfocused_pm = m_grip_focused_pm = 0;

    m_shaded = false;
    m_label.show();

    showHandle();
    showTitlebar();

    // note: we don't show clientarea yet

    setEventHandler(*this);

    reconfigureTitlebar();
    reconfigure();
}

/**
   Setups upp background, pressed pixmap/color of the button to current theme
*/
void FbWinFrame::setupButton(FbTk::Button &btn) {
    if (m_button_pressed_pm) {
        btn.setPressedPixmap(m_button_pressed_pm);
    } else {
        //        cerr<<"No pixmap for button pressed"<<endl;
    }
    //TODO button pressed color

    if (m_focused) {
        btn.setGC(m_theme.labelTextFocusGC());
        if (m_button_pm)
            btn.setBackgroundPixmap(m_button_pm);
        else
            btn.setBackgroundColor(m_button_color);
    } else {
        btn.setGC(m_theme.labelTextUnfocusGC());
        if (m_button_unfocused_pm)
            btn.setBackgroundPixmap(m_button_unfocused_pm);
        else
            btn.setBackgroundColor(m_button_color);
    }
    btn.clear();
}

void FbWinFrame::render(const FbTk::Texture &tex, FbTk::Color &col, Pixmap &pm,
                        unsigned int w, unsigned int h) {
    Pixmap tmp = pm;
    if (tex.type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        pm = None;
        col = tex.color();
    } else
        pm = m_imagectrl.renderImage(w, h, tex);
		
    if (tmp) 
        m_imagectrl.removeImage(tmp);

}
