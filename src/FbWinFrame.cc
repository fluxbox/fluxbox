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

// $Id: FbWinFrame.cc,v 1.26 2003/06/05 13:09:08 fluxgen Exp $

#include "FbWinFrame.hh"
#include "ImageControl.hh"
#include "EventManager.hh"
#include "TextButton.hh"
#include "App.hh"
#ifdef SHAPE
//#include "Shape.hh"
#endif // SHAPE

#include <algorithm>
#include <iostream>
using namespace std;

FbWinFrame::FbWinFrame(FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, 
                       int screen_num, int x, int y,
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
    m_bevel(1),
    m_use_titlebar(true), 
    m_use_handle(true),
    m_focused(false),
    m_visible(false),
    m_button_pm(0),
    m_themelistener(*this) {

    //    m_shape(new Shape(m_window, 0)) { //Shape::TOPLEFT | Shape::TOPRIGHT)) {
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
    m_visible = false;
}

void FbWinFrame::show() {
    m_visible = true;
    reconfigure();
    m_window.showSubwindows();
    m_window.show();
}

/**
 Toggle shade state, and resize window
 */
void FbWinFrame::shade() {
    if (!m_use_titlebar)
        return;

    // toggle shade
    m_shaded = !m_shaded;
    if (m_shaded) { // i.e. should be shaded now
        m_width_before_shade = m_window.width();
        m_height_before_shade = m_window.height();
        m_window.resize(m_window.width(), m_titlebar.height());
    } else { // should be unshaded
        m_window.resize(m_width_before_shade, m_height_before_shade);
        reconfigure();
    }
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
    // total height for frame
    unsigned int total_height = height;

    // having a titlebar = 1 extra border + titlebar height
    if (m_use_titlebar)
        total_height += m_titlebar.height() + m_titlebar.borderWidth();
    // having a handle = 1 extra border + handle height
    if (m_use_handle)
        total_height += m_handle.height() + m_handle.borderWidth();
    resize(width, total_height);
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
}

void FbWinFrame::addRightButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    setupButton(*btn); // setup theme and other stuff

    m_buttons_right.push_back(btn);
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
}

void FbWinFrame::addLabelButton(FbTk::Button &btn) {
    ButtonList::iterator found_it = find(m_labelbuttons.begin(),
                                         m_labelbuttons.end(),
                                         &btn);
    
    if (found_it != m_labelbuttons.end())
        return;
    
    m_labelbuttons.push_back(&btn);
}

void FbWinFrame::removeLabelButton(FbTk::Button &btn) {
    ButtonList::iterator erase_it = remove(m_labelbuttons.begin(),
                                           m_labelbuttons.end(),
                                           &btn);
    if (erase_it == m_labelbuttons.end())
        return;

    m_labelbuttons.erase(erase_it);
}

void FbWinFrame::setLabelButtonFocus(FbTk::Button &btn) {
    ButtonList::iterator it = find(m_labelbuttons.begin(),
                                   m_labelbuttons.end(),
                                   &btn);
    if (it == m_labelbuttons.end())
        return;

    m_current_label = *it; // current focused button
    renderLabelButtons();
}

void FbWinFrame::setClientWindow(FbTk::FbWindow &win) {
    setClientWindow(win.window());
}

void FbWinFrame::setClientWindow(Window win) {
    Display *display = FbTk::App::instance()->display();
    XSetWindowBorderWidth(display, win, 0);

    XChangeSaveSet(display, win, SetModeInsert);
    
    XSelectInput(display, m_clientarea.window(), NoEventMask);
    // we need to mask this so we don't get unmap event
    XSelectInput(display, win, NoEventMask);
    XReparentWindow(display, win, m_clientarea.window(), 0, 0);
    // remask window so we get events
    XSelectInput(display, win, PropertyChangeMask | StructureNotifyMask | 
                 FocusChangeMask);
    XSelectInput(display, m_clientarea.window(), SubstructureRedirectMask);

    XFlush(display);

    XSetWindowAttributes attrib_set;
    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask | FocusChangeMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | 
        ButtonMotionMask;

    XChangeWindowAttributes(display, win, CWEventMask|CWDontPropagate, &attrib_set);

    m_clientarea.raise();
    m_clientarea.showSubwindows();

}

void FbWinFrame::hideTitlebar() {
    if (!m_use_titlebar)
        return;

    m_titlebar.hide();
    m_use_titlebar = false;
    m_clientarea.raise();

    // only take away one borderwidth (as the other border is still the "top" border)
    m_window.resize(m_window.width(), m_window.height() - m_titlebar.height() -
                    m_titlebar.borderWidth());
#ifdef DEBUG
    cerr<<__FILE__<<": Hide Titlebar"<<endl;
#endif // DEBUG
}

void FbWinFrame::showTitlebar() {
    if (m_use_titlebar)
        return;

    m_titlebar.show();
    m_use_titlebar = true;

    // only add one borderwidth (as the other border is still the "top" border)
    m_window.resize(m_window.width(), m_window.height() + m_titlebar.height() +
                    m_titlebar.borderWidth());

#ifdef DEBUG
    cerr<<__FILE__<<": Show Titlebar"<<endl;
#endif // DEBUG
}

void FbWinFrame::hideHandle() {
    if (!m_use_handle)
        return;
    m_handle.hide();
    m_grip_left.hide();
    m_grip_right.hide();
    m_use_handle = false;
    m_window.resize(m_window.width(), m_window.height() - m_handle.height() -
                    m_handle.borderWidth());

}

void FbWinFrame::showHandle() {
    if (m_use_handle)
        return;
    m_handle.show();
    m_grip_left.show();
    m_grip_right.show();
    m_use_handle = true;
    m_window.resize(m_window.width(), m_window.height() + m_handle.height() +
                    m_handle.borderWidth());
}

void FbWinFrame::hideAllDecorations() {
    hideHandle();
    hideTitlebar();
    // resize done by hide*
    reconfigure();
}

void FbWinFrame::showAllDecorations() {
    if (!m_use_handle)
        showHandle();
    if (!m_use_titlebar)
        showTitlebar();
    // resize shouldn't be necessary
    reconfigure();
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
    // we can ignore which window the event was generated for
    ButtonList::iterator btn_it = m_labelbuttons.begin();
    ButtonList::iterator btn_it_end = m_labelbuttons.end();
    for (; btn_it != btn_it_end; ++btn_it) {
        if ((*btn_it)->window() == event.window) {
            (*btn_it)->buttonPressEvent(event);
            break;
        }
    }
    if (event.window == m_grip_right.window() ||
        event.window == m_grip_left.window() ||
        event.window == m_clientarea.window() ||
        event.window == m_handle.window() ||
        event.window == m_window.window())
        return;
    if (event.button > 5 || event.button < 1)
        return;

    if (*m_commands[event.button - 1].click_pressed)
        m_commands[event.button - 1].click_pressed->execute();
}

void FbWinFrame::buttonReleaseEvent(XButtonEvent &event) {
    // we can ignore which window the event was generated for
    
    ButtonList::iterator btn_it = m_labelbuttons.begin();
    ButtonList::iterator btn_it_end = m_labelbuttons.end();
    for (; btn_it != btn_it_end; ++btn_it) {
        if ((*btn_it)->window() == event.window) {
            (*btn_it)->buttonReleaseEvent(event);
            break;
        }
    }

    if (event.window == m_grip_right.window() ||
        event.window == m_grip_left.window() ||
        event.window == m_clientarea.window() ||
        event.window == m_handle.window() ||
        event.window == m_window.window())
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
    if (m_label == event.window) 
        redrawTitle();
    else if (m_handle == event.window ||
             m_grip_left == event.window ||
             m_grip_right == event.window) 
        renderHandles();
    else
        redrawTitlebar();
    
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
    //    if (m_shape.get())
    //        m_shape->update();

    // align titlebar and render it
    if (m_use_titlebar)
        reconfigureTitlebar();


    // leave client+grips alone if we're shaded (it'll get fixed when we unshade)
    if (!m_shaded) {
        int client_top = 0;
        int client_height = m_window.height();
        if (m_use_titlebar) {
            // only one borderwidth as titlebar is really at -borderwidth
            int titlebar_height = m_titlebar.height() + m_titlebar.borderWidth();
            client_top += titlebar_height;
            client_height -= titlebar_height;
        }
        
        if (m_use_handle) {
            client_height -= m_handle.height() + m_handle.borderWidth();
        }
        
        m_clientarea.moveResize(0, client_top,
                                m_window.width(), client_height);
        
        
        if (m_use_handle) {

        
            // align handle and grips
            const int grip_height = m_handle.height();
            const int grip_width = 20; //TODO
        
            const int ypos = m_window.height() - grip_height - m_handle.borderWidth();
        
            m_grip_left.moveResize(-m_handle.borderWidth(), ypos,
                                   grip_width, grip_height);
        
            m_handle.moveResize(grip_width, ypos,
                                m_window.width() - grip_width*2 - m_handle.borderWidth()*2, 
                                grip_height);
        
            m_grip_right.moveResize(m_window.width() - grip_width -  m_handle.borderWidth(), ypos,
                                    grip_width, grip_height);
        }
    }        

    if (!m_visible) return;

    // render the theme
    renderButtons();
    if (!m_shaded)
        renderHandles();
    // titlebar stuff rendered already by reconftitlebar
}

unsigned int FbWinFrame::buttonHeight() const {
    return m_titlebar.height() - m_bevel*2;
}


//--------------------- private area

/**
   aligns and redraws title
*/
void FbWinFrame::redrawTitle() {
    if (m_labelbuttons.size() == 0 || !m_visible)
        return;

    int button_width = label().width()/m_labelbuttons.size();
    //!! TODO: bevel
    int border_width = m_labelbuttons.front()->window().borderWidth();

    ButtonList::iterator btn_it = m_labelbuttons.begin();
    ButtonList::iterator btn_it_end = m_labelbuttons.end();
    for (unsigned int last_x = 0;
         btn_it != btn_it_end; 
         ++btn_it, last_x += button_width + border_width) {        
        // since we add border width pixel we should remove
        // the same size for inside width so we can fit all buttons into label
        (*btn_it)->moveResize(last_x - (last_x ? border_width : 0), - border_width,
                              button_width, 
                              label().height() + border_width);
        (*btn_it)->clear();
    }
        
}

void FbWinFrame::redrawTitlebar() {
    if (!m_use_titlebar || !m_visible)
        return;
    m_titlebar.clear();
    m_label.clear();
    redrawTitle();

    if (m_current_label)
        m_current_label->clear();
 }

/**
   Align buttons with title text window
*/
void FbWinFrame::reconfigureTitlebar() {
    if (!m_use_titlebar)
        return;

    // resize titlebar to window size with font height
    m_titlebar.moveResize(-m_titlebar.borderWidth(), -m_titlebar.borderWidth(),
                          m_window.width(),
                          m_theme.font().height() == 0 ? 16 : 
                          m_theme.font().height() + m_bevel*2 + 2);

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
        space_left -= m_buttons_right.size() * (button_size + m_bevel);

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
    if (!m_use_titlebar || !m_visible)
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
    Pixmap label_pm = None;
    Pixmap title_pm = None;    
    FbTk::Color label_color;
    FbTk::Color title_color;
    getCurrentFocusPixmap(label_pm, title_pm,
                          label_color, title_color);


    if (label_pm != 0)
        m_label.setBackgroundPixmap(label_pm);
    else            
        m_label.setBackgroundColor(label_color);
    
    if (title_pm != 0)
        m_titlebar.setBackgroundPixmap(title_pm);
    else
        m_titlebar.setBackgroundColor(title_color);

    renderLabelButtons();
    redrawTitlebar();
}


void FbWinFrame::renderHandles() {
    if (!m_use_handle || !m_visible)
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
    if (!m_visible) return;

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
    m_current_label = 0; // no focused button at first
    m_title_focused_pm = m_title_unfocused_pm = 0;
    m_label_focused_pm = m_label_unfocused_pm = 0;
    m_button_unfocused_pm = m_button_pressed_pm = 0;
    m_double_click_time = 200;
    m_button_pm = 0;
    m_button_size = 26;
    m_handle_focused_pm = 
        m_handle_unfocused_pm = 0;
    m_grip_unfocused_pm = m_grip_focused_pm = 0;
    m_clientarea.setBorderWidth(0);
    m_shaded = false;
    m_label.show();

    showHandle();
    showTitlebar();

    // note: we don't show clientarea yet

    setEventHandler(*this);

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
        btn.setGC(m_theme.buttonPicFocusGC());
        if (m_button_pm)
            btn.setBackgroundPixmap(m_button_pm);
        else
            btn.setBackgroundColor(m_button_color);
    } else {
        btn.setGC(m_theme.buttonPicUnfocusGC());
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

void FbWinFrame::getCurrentFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                                FbTk::Color &label_color, FbTk::Color &title_color) {
    if (m_focused) {
        if (m_label_focused_pm != 0)
            label_pm = m_label_focused_pm;
        else
            label_color = m_label_focused_color;
    
        if (m_title_focused_pm != 0)
            title_pm = m_title_focused_pm;
        else
            title_color = m_title_focused_color;
     
    } else {
        getUnFocusPixmap(label_pm, title_pm,
                         label_color, title_color);
    }

}

void FbWinFrame::getUnFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                                  FbTk::Color &label_color, FbTk::Color &title_color) {
    if (m_label_unfocused_pm != 0)
        label_pm = m_label_unfocused_pm;            
    else
        label_color = m_label_unfocused_color;            

    if (m_title_unfocused_pm != 0)
        title_pm  = m_title_unfocused_pm;
    else
        title_color = m_title_unfocused_color;
}

void FbWinFrame::renderLabelButtons() {
    if (!m_visible) return;
    Pixmap label_pm = None;
    Pixmap not_used_pm = None;
    FbTk::Color label_color;
    FbTk::Color not_used_color;
    getCurrentFocusPixmap(label_pm, not_used_pm,
                          label_color, not_used_color);

    ButtonList::iterator btn_it = m_labelbuttons.begin();
    ButtonList::iterator btn_it_end = m_labelbuttons.end();        
    for (; btn_it != btn_it_end; ++btn_it) {
        (*btn_it)->setGC(theme().labelTextFocusGC());
        (*btn_it)->window().setBorderWidth(1);
        if (m_label_unfocused_pm != 0)
            (*btn_it)->setBackgroundPixmap(m_label_unfocused_pm);
        else
            (*btn_it)->setBackgroundColor(m_label_unfocused_color);
            
        (*btn_it)->clear();
    }

    if (m_current_label != 0) {

        if (label_pm) {
            m_current_label->setBackgroundPixmap(label_pm);
            cerr<<"label_pm = "<<hex<<label_pm<<dec<<endl;
        } else
            m_current_label->setBackgroundColor(label_color);
        m_current_label->clear();
    }
    
}
