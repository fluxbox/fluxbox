// Toolbar.cc for Fluxbox
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Toolbar.cc for Blackbox - an X11 Window manager
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

// $Id: Toolbar.cc,v 1.99 2003/07/10 13:46:47 fluxgen Exp $

#include "Toolbar.hh"

#include "IconBar.hh"
#include "I18n.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "ImageControl.hh"
#include "ToolbarTheme.hh"
#include "EventManager.hh"
#include "Text.hh"
#include "ArrowButton.hh"
#include "SimpleCommand.hh"
#include "IntResMenuItem.hh"
#include "MacroCommand.hh"
#include "RootTheme.hh"
#include "BoolMenuItem.hh"
#include "FbWinFrameTheme.hh"
#include "Xinerama.hh"
#include "Strut.hh"

// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Shape.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <cstring>
#include <cstdio>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else // !TIME_WITH_SYS_TIME
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else // !HAVE_SYS_TIME_H
#include <time.h>
#endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME


#include <iostream>

using namespace std;

template<>
void FbTk::Resource<Toolbar::Placement>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Toolbar::TOPLEFT;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Toolbar::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Toolbar::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Toolbar::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Toolbar::TOPRIGHT;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Toolbar::BOTTOMRIGHT;
    else if (strcasecmp(strval, "LeftTop") == 0)
        m_value = Toolbar::LEFTTOP;
    else if (strcasecmp(strval, "LeftCenter") == 0)
        m_value = Toolbar::LEFTCENTER;
    else if (strcasecmp(strval, "LeftBottom") == 0)
        m_value = Toolbar::LEFTBOTTOM;
    else if (strcasecmp(strval, "RightTop") == 0)
        m_value = Toolbar::RIGHTTOP;
    else if (strcasecmp(strval, "RightCenter") == 0)
        m_value = Toolbar::RIGHTCENTER;
    else if (strcasecmp(strval, "RightBottom") == 0)
        m_value = Toolbar::RIGHTBOTTOM;
    else
        setDefaultValue();
}

string FbTk::Resource<Toolbar::Placement>::
getString() {
    switch (m_value) {
    case Toolbar::TOPLEFT:
        return string("TopLeft");
        break;
    case Toolbar::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case Toolbar::TOPCENTER:
        return string("TopCenter");
        break;			
    case Toolbar::BOTTOMCENTER:
        return string("BottomCenter");
        break;
    case Toolbar::TOPRIGHT:
        return string("TopRight");
        break;
    case Toolbar::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    case Toolbar::LEFTTOP:
        return string("LeftTop");
        break;
    case Toolbar::LEFTCENTER:
        return string("LeftCenter");
        break;
    case Toolbar::LEFTBOTTOM:
        return string("LeftBottom");
        break;
    case Toolbar::RIGHTTOP:
        return string("RightTop");
        break;
    case Toolbar::RIGHTCENTER:
        return string("RightCenter");
        break;
    case Toolbar::RIGHTBOTTOM:
        return string("RightBottom");
        break;
    }
    //default string
    return string("BottomCenter");
}


namespace {
class SetToolbarPlacementCmd: public FbTk::Command {
public:
    explicit SetToolbarPlacementCmd(Toolbar &tbar, Toolbar::Placement place):m_tbar(tbar), m_place(place) { }
    void execute() {
        m_tbar.setPlacement(m_place);
        m_tbar.reconfigure();        
        Fluxbox::instance()->save_rc();
    }
private:
    Toolbar &m_tbar;
    Toolbar::Placement m_place;
};

}; // end anonymous

// toolbar frame
Toolbar::Frame::Frame(FbTk::EventHandler &evh, int screen_num):
    window(screen_num, // screen (parent)
           0, 0, // pos
           10, 10, // size
           // event mask
           ButtonPressMask | ButtonReleaseMask | 
           EnterWindowMask | LeaveWindowMask,
           true), // override redirect 
    workspace_label(window, // parent
                    0, 0, //pos
                    1, 1, // size
                    // event mask
                    ButtonPressMask | ButtonReleaseMask | 
                    ExposureMask | KeyPressMask |
                    EnterWindowMask | LeaveWindowMask),
    window_label(window, // parent
                  0, 0, // pos
                  1, 1, // size
                 // event mask
                 ButtonPressMask | ButtonReleaseMask | 
                 ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    clock(window, //parent
          0, 0, // pos
          1, 1, // size
          // event mask
          ButtonPressMask | ButtonReleaseMask | 
          ExposureMask |
          EnterWindowMask | LeaveWindowMask),
    psbutton(ArrowButton::LEFT, // arrow type
             window, // parent
             0, 0, // pos
             1, 1), // size
    nsbutton(ArrowButton::RIGHT, // arrow type
             window, // parent
             0, 0, // pos
             1, 1), // size
    pwbutton(ArrowButton::LEFT, // arrow type
             window, // parent
             0, 0, // pos
             1, 1), // size
    nwbutton(ArrowButton::RIGHT, // arrow type
             window, // parent
             0, 0, // pos 
             1, 1), // size
    hour(-1), // start with invalid number to force update
    minute(-1) {

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // add windows to eventmanager
    evm.add(evh, window);
    evm.add(evh, workspace_label);
    evm.add(evh, window_label);
    evm.add(evh, clock);

    psbutton.setMouseMotionHandler(&evh);
    nsbutton.setMouseMotionHandler(&evh);
    pwbutton.setMouseMotionHandler(&evh);
    nwbutton.setMouseMotionHandler(&evh);
}

Toolbar::Frame::~Frame() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // remove windows from eventmanager
    evm.remove(window);
    evm.remove(workspace_label);
    evm.remove(window_label);
    evm.remove(clock);
}

Toolbar::Toolbar(BScreen &scrn, FbTk::XLayer &layer, FbTk::Menu &menu, size_t width):
    m_editing(false),
    m_hidden(false),
    frame(*this, scrn.screenNumber()),
    m_screen(scrn),
    m_clock_timer(this), // get the clock updating every minute
    m_hide_timer(&hide_handler),
    m_toolbarmenu(menu),
    m_placementmenu(*scrn.menuTheme(),
                    scrn.screenNumber(), scrn.imageControl()),
    m_layermenu(*scrn.menuTheme(), 
                scrn.screenNumber(), 
                scrn.imageControl(),
                *scrn.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()), 
                this,
                true),
    m_theme(scrn.screenNumber()),
    m_themelistener(*this),
    m_layeritem(frame.window, layer),
    m_strut(0),
    m_rc_auto_hide(scrn.resourceManager(), false, 
                   scrn.name() + ".toolbar.autoHide", scrn.altName() + ".Toolbar.AutoHide"),
    m_rc_width_percent(scrn.resourceManager(), 65, 
                       scrn.name() + ".toolbar.widthPercent", scrn.altName() + ".Toolbar.WidthPercent"),  
    m_rc_layernum(scrn.resourceManager(), Fluxbox::Layer(Fluxbox::instance()->getDesktopLayer()), 
                  scrn.name() + ".toolbar.layer", scrn.altName() + ".Toolbar.Layer"),
    m_rc_on_head(scrn.resourceManager(), 0,
                 scrn.name() + ".toolbar.onhead", scrn.altName() + ".Toolbar.onHead"),
    m_rc_placement(scrn.resourceManager(), Toolbar::BOTTOMCENTER, 
                   scrn.name() + ".toolbar.placement", scrn.altName() + ".Toolbar.Placement"),
    m_shape(new Shape(frame.window, 0)) {

    // we need to get notified when the theme is reloaded
    m_theme.addListener(m_themelistener);
    // listen to screen reconfigure
    screen().reconfigureSig().attach(&m_themelistener);

    m_layermenu.setInternalMenu();
    m_placementmenu.setInternalMenu();
    setupMenus();

    // geometry settings
    frame.width = width;
    frame.height = frame.label_h = 10;
    frame.window_label_w = 
        frame.workspace_label_w = frame.clock_w = width/3; 
    frame.button_w = 20;
    frame.bevel_w = 1;

    timeval delay;
    delay.tv_sec = 1;
    delay.tv_usec = 0;
    m_clock_timer.setTimeout(delay);
    m_clock_timer.start();

    m_theme.font().setAntialias(screen().antialias());

    hide_handler.toolbar = this;
    m_hide_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay());
    m_hide_timer.fireOnce(true);

    frame.grab_x = frame.grab_y = 0;

    frame.base = frame.label = frame.wlabel = frame.clk = frame.button =
        frame.pbutton = None;

    m_iconbar.reset(new IconBar(screen(), frame.window_label.window(), m_theme.font()));

    // finaly: setup Commands for the buttons in the frame
    typedef FbTk::SimpleCommand<BScreen> ScreenCmd;
    FbTk::RefCount<FbTk::Command> nextworkspace(new ScreenCmd(screen(), 
                                                              &BScreen::nextWorkspace));
    FbTk::RefCount<FbTk::Command> prevworkspace(new ScreenCmd(screen(), 
                                                              &BScreen::prevWorkspace));
    FbTk::RefCount<FbTk::Command> nextwindow(new ScreenCmd(screen(), 
                                                           &BScreen::nextFocus));
    FbTk::RefCount<FbTk::Command> prevwindow(new ScreenCmd(screen(), 
                                                           &BScreen::prevFocus));
    frame.psbutton.setOnClick(prevworkspace);
    frame.nsbutton.setOnClick(nextworkspace);
    frame.pwbutton.setOnClick(prevwindow);
    frame.nwbutton.setOnClick(nextwindow);

    reconfigure(); // get everything together
    frame.window.showSubwindows();
    frame.window.show();
}


Toolbar::~Toolbar() {
    clearStrut();
    FbTk::ImageControl &image_ctrl = screen().imageControl();
    if (frame.base) image_ctrl.removeImage(frame.base);
    if (frame.label) image_ctrl.removeImage(frame.label);
    if (frame.wlabel) image_ctrl.removeImage(frame.wlabel);
    if (frame.clk) image_ctrl.removeImage(frame.clk);
    if (frame.button) image_ctrl.removeImage(frame.button);
    if (frame.pbutton) image_ctrl.removeImage(frame.pbutton);

}

void Toolbar::clearStrut() {
    if (m_strut) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

void Toolbar::updateStrut() {
    bool had_strut = m_strut ? true : false;
    clearStrut();
    // we should request space if we're in autohide mode or
    // if the user dont want to request space for toolbar.
    if (doAutoHide()) {
        if (had_strut)
            screen().updateAvailableWorkspaceArea();            
        return;
    }

    // request area on screen
    int top = 0, bottom = 0, left = 0, right = 0;
    switch (placement()) {
    case TOPLEFT:
    case TOPCENTER:
    case TOPRIGHT:
        top = height();
        break;
    case BOTTOMLEFT:
    case BOTTOMCENTER:
    case BOTTOMRIGHT:
        bottom = height();
        break;
    case RIGHTTOP:
    case RIGHTCENTER:
    case RIGHTBOTTOM:
        right = width();
        break;
    case LEFTTOP:
    case LEFTCENTER:
    case LEFTBOTTOM:
        left = width();
        break;
    };
    m_strut = screen().requestStrut(left, right, top, bottom);
    screen().updateAvailableWorkspaceArea();
}

bool Toolbar::isVertical() const {
    return (placement() == RIGHTCENTER ||
            placement() == RIGHTTOP ||
            placement() == RIGHTBOTTOM ||
            placement() == LEFTCENTER ||
            placement() == LEFTTOP ||
            placement() == LEFTBOTTOM);
}

void Toolbar::addIcon(FluxboxWindow *w) {
    if (w == 0)
        return;

    if (m_iconbar.get() != 0)
        FbTk::EventManager::instance()->add(*this, m_iconbar->addIcon(w));
}

void Toolbar::delIcon(FluxboxWindow *w) {
    if (w == 0)
        return;

    if (m_iconbar.get() != 0)
        FbTk::EventManager::instance()->remove(m_iconbar->delIcon(w));
}

void Toolbar::delAllIcons(bool ignore_stuck) {
    if (m_iconbar.get() == 0)
        return;

    IconBar::WindowList *deleted = m_iconbar->delAllIcons(ignore_stuck);
    IconBar::WindowList::iterator it = deleted->begin();
    IconBar::WindowList::iterator it_end = deleted->end();
    for (; it != it_end; ++it) {
        FbTk::EventManager::instance()->remove(*it);
    }
    delete deleted;
}
    
bool Toolbar::containsIcon(const FluxboxWindow &win) const {
    return (m_iconbar->findIcon(&win) != 0);
}

void Toolbar::enableUpdates() {
    if (m_iconbar.get() != 0)
        m_iconbar->enableUpdates();
}

void Toolbar::disableUpdates() {
    if (m_iconbar.get() != 0)
        m_iconbar->disableUpdates();
}

void Toolbar::enableIconBar() {
    if (m_iconbar.get() != 0) 
        return; // already on

    m_iconbar.reset(new IconBar(screen(), 
                                frame.window_label.window(),
                                m_theme.font()));
}

void Toolbar::disableIconBar() {
    if (m_iconbar.get() == 0) 
        return; // already off

    delAllIcons();

    m_iconbar.reset(0); // destroy iconbar

}

void Toolbar::raise() {
    m_layeritem.raise();
}

void Toolbar::lower() {
    m_layeritem.lower();
}

void Toolbar::reconfigure() {

    theme().font().setAntialias(screen().antialias());

    if (doAutoHide())
        m_hide_timer.start();

    bool vertical = isVertical();

    if (m_iconbar.get())
        m_iconbar->setVertical(vertical);

    frame.bevel_w = theme().bevelWidth();
    // destroy shape if the theme wasn't specified with one,
    // or create one 
    if (theme().shape() == false && m_shape.get())
        m_shape.reset(0);
    else if (theme().shape() && m_shape.get() == 0) {
        m_shape.reset(new Shape(frame.window, 0));
    }
    // recallibrate size
    setPlacement(placement());

#ifdef HAVE_STRFTIME
    time_t ttmp = time(0);
    struct tm *tt = 0;

    if (ttmp != -1) {
        tt = localtime(&ttmp);
        if (tt) {
            char t[1024], *time_string = (char *) 0;
            int len = strftime(t, 1024, screen().getStrftimeFormat(), tt);

            time_string = new char[len + 1];

            memset(time_string, '0', len);
            *(time_string + len) = '\0';

            frame.clock_w = m_theme.font().textWidth(time_string, len);
            frame.clock_w += (frame.bevel_w * 4);
			
            delete [] time_string;
        } else
            frame.clock_w = 0;
    } else
        frame.clock_w = 0;
#else // !HAVE_STRFTIME
	
    I18n *i18n = I18n::instance();
    frame.clock_w = m_theme.font().textWidth(
                                             i18n->
                                             getMessage(FBNLS::ToolbarSet, 
                                                        FBNLS::ToolbarNoStrftimeLength,
                                                        "00:00000"),
                                             strlen(i18n->
                                                    getMessage(FBNLS::ToolbarSet, 
                                                               FBNLS::ToolbarNoStrftimeLength,
                                                               "00:00000"))) + (frame.bevel_w * 4);
	
#endif // HAVE_STRFTIME

    unsigned int i;
    unsigned int w = 0;
    frame.workspace_label_w = 0;

    for (i = 0; i < screen().getCount(); i++) {
        w = m_theme.font().textWidth(screen().getWorkspace(i)->name().c_str(),
                                     screen().getWorkspace(i)->name().size());

        w += (frame.bevel_w * 4);

        if (w > frame.workspace_label_w)
            frame.workspace_label_w = w;
    }
    
    if (frame.workspace_label_w < frame.clock_w)
        frame.workspace_label_w = frame.clock_w;
    else if (frame.workspace_label_w > frame.clock_w)
        frame.clock_w = frame.workspace_label_w;

    frame.psbutton.window().setBorderWidth(theme().buttonBorderWidth());
    frame.nsbutton.window().setBorderWidth(theme().buttonBorderWidth());
    frame.pwbutton.window().setBorderWidth(theme().buttonBorderWidth());
    frame.nwbutton.window().setBorderWidth(theme().buttonBorderWidth());

    // Right, let's break this one down....
    // full width, minus clock, workspace label and the 4 arrow buttons.
    // each of the (6) aforementioned items are separated by a bevel width, 
    // plus outside (+1), plus the window label (+1).

    i = frame.clock_w + (frame.button_w * 4) +
        frame.workspace_label_w + (frame.bevel_w * 8) + 6;

    // of course if your toolbar is set too small, this could go negative.
    // which is bad mmmkay. Since we are unsigned, we check that *first*.
    if (vertical) 
        w = frame.height;
    else
        w = frame.width;

    if (i > w)
        frame.window_label_w = 0;
    else 
        frame.window_label_w = w - i;


    if (isHidden())
        frame.window.moveResize(frame.x_hidden, frame.y_hidden,
                                frame.width, frame.height);
    else {
        frame.window.moveResize(frame.x, frame.y,
                                frame.width, frame.height);
    }


    unsigned int next_x = frame.workspace_label_w;
    unsigned int next_y = frame.window.height();
    unsigned int text_x=0, text_y=0;
    if (vertical) 
        text_x = frame.bevel_w;
    else
        text_y = frame.bevel_w;

    
    if (vertical) {
        next_x = frame.window.width();
        next_y = frame.workspace_label_w;
    }
    
    frame.workspace_label.moveResize(frame.bevel_w, frame.bevel_w, next_x, next_y);
    next_x = 0;
    next_y = 0;
    if (vertical) {
        next_y += frame.workspace_label.height() + 1 + frame.bevel_w * 2;
    } else {
        next_x += frame.workspace_label.width() + 1 + frame.bevel_w * 2;
    }

    frame.psbutton.moveResize(next_x , next_y,
                              frame.button_w, frame.button_w);
    if (vertical)
        next_y += frame.psbutton.height() + 1;
    else
        next_x += frame.psbutton.width() + 1;

    frame.nsbutton.moveResize(next_x, next_y,
                              frame.button_w, frame.button_w);
    size_t label_w = frame.window_label_w;
    size_t label_h = frame.height;

    if (vertical) {
        next_y += frame.nsbutton.height() + 1;
        label_w = frame.width;
        label_h = frame.window_label_w/* - frame.width + frame.height*/;

    } else
        next_x += frame.nsbutton.width() + 1;
       
    frame.window_label.moveResize(next_x, next_y,
                                  label_w, label_h);
    if (vertical)
        next_y += frame.window_label.height() + 1;
    else
        next_x += frame.window_label.width() + 1;

    frame.pwbutton.moveResize(next_x, next_y,
                              frame.button_w, frame.button_w);
    if (vertical)
        next_y += frame.pwbutton.height() + 1;
    else
        next_x += frame.pwbutton.width() + 1;

    frame.nwbutton.moveResize(next_x, next_y,
                              frame.button_w, frame.button_w);

    size_t clock_w = frame.width - next_x - frame.nwbutton.width() - 1;
    size_t clock_h = frame.height;
    if (vertical) {
        next_y += frame.nwbutton.height() + 1;
        clock_w = frame.width;
        clock_h = frame.height - next_y;
    } else
        next_x += frame.nwbutton.width() + 1;

    frame.clock.moveResize(next_x + text_x, next_y + text_y,
                           clock_w, clock_h);

    FbTk::ImageControl &image_ctrl = screen().imageControl();

    Pixmap tmp = frame.base;
    const FbTk::Texture *texture = &(m_theme.toolbar());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.base = None;
        frame.window.setBackgroundColor(texture->color());
    } else {
        frame.base = image_ctrl.renderImage(frame.window.width(), 
                                            frame.window.height(), *texture);
        frame.window.setBackgroundPixmap(frame.base);
    }
    if (tmp) 
        image_ctrl.removeImage(tmp);
    
    tmp = frame.label;
    texture = &(m_theme.window());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.label = None;
        frame.window_label.setBackgroundColor(texture->color());
    } else {
        frame.label =
            image_ctrl.renderImage(frame.window_label.width(), 
                                   frame.window_label.height(), *texture);
        frame.window_label.setBackgroundPixmap(frame.label);
    }
    if (tmp) image_ctrl.removeImage(tmp);

    tmp = frame.wlabel;
    texture = &(m_theme.label());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.wlabel = None;
        frame.workspace_label.setBackgroundColor(texture->color());
    } else {
        frame.wlabel =
            image_ctrl.renderImage(frame.workspace_label.width(), 
                                   frame.workspace_label.height(), *texture);
        frame.workspace_label.setBackgroundPixmap(frame.wlabel);
    }
    if (tmp) image_ctrl.removeImage(tmp);

    tmp = frame.clk;
    texture = &(m_theme.clock());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.clk = None;
        frame.clock.setBackgroundColor(texture->color());
    } else {
        frame.clk =
            image_ctrl.renderImage(frame.clock.width(), frame.clock.height(), *texture);
        frame.clock.setBackgroundPixmap(frame.clk);
    }
    if (tmp) image_ctrl.removeImage(tmp);

    tmp = frame.button;
    texture = &(m_theme.button());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.button = None;

        const FbTk::Color &color = texture->color();
        frame.psbutton.setBackgroundColor(color);
        frame.nsbutton.setBackgroundColor(color);
        frame.pwbutton.setBackgroundColor(color);
        frame.nwbutton.setBackgroundColor(color);
    } else {
        frame.button =
            image_ctrl.renderImage(frame.button_w, frame.button_w, *texture);
        
        frame.psbutton.setBackgroundPixmap(frame.button);
        frame.nsbutton.setBackgroundPixmap(frame.button);
        frame.pwbutton.setBackgroundPixmap(frame.button);
        frame.nwbutton.setBackgroundPixmap(frame.button);
    }
    if (tmp) 
        image_ctrl.removeImage(tmp);

    // pressed button pixmap
    tmp = frame.pbutton;
    texture = &(m_theme.pressedButton());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.pbutton = None;
    } else {
        frame.pbutton =
            image_ctrl.renderImage(frame.button_w, frame.button_w, *texture);
        frame.psbutton.setPressedPixmap(frame.pbutton);
        frame.nsbutton.setPressedPixmap(frame.pbutton);
        frame.pwbutton.setPressedPixmap(frame.pbutton);
        frame.nwbutton.setPressedPixmap(frame.pbutton);
    }
    // setup button gc
    frame.psbutton.setGC(m_theme.buttonPicGC());
    frame.nsbutton.setGC(m_theme.buttonPicGC());
    frame.pwbutton.setGC(m_theme.buttonPicGC());
    frame.nwbutton.setGC(m_theme.buttonPicGC());

    if (tmp) 
        image_ctrl.removeImage(tmp);

    frame.window.setBorderColor(theme().borderColor());
    frame.window.setBorderWidth(theme().borderWidth());

    frame.window.clear();

    frame.workspace_label.clear();
    frame.window_label.clear();
    frame.clock.clear();    
    frame.psbutton.clear();
    frame.nsbutton.clear();
    frame.pwbutton.clear();
    frame.nwbutton.clear();

    if (theme().shape() && m_shape.get())
        m_shape->update();

    redrawWindowLabel();
    if (m_iconbar.get()) 
        m_iconbar->reconfigure();

    redrawWorkspaceLabel();
    checkClock(true);

    m_toolbarmenu.reconfigure();
    // we're done with all resizing and stuff now we can request a new 
    // area to be reserv on screen
    updateStrut();
}



void Toolbar::checkClock(bool redraw, bool date) {
    time_t tmp = 0;
    struct tm *tt = 0;

    if ((tmp = time(0)) != -1) {
        if (! (tt = localtime(&tmp)))
            return;

        if (tt->tm_min != frame.minute || tt->tm_hour != frame.hour) {
            frame.hour = tt->tm_hour;
            frame.minute = tt->tm_min;
            frame.clock.clear();
            redraw = true;
        }
    }

    if (!redraw)
        return;

    frame.clock.clear();
#ifdef HAVE_STRFTIME
    char t[1024];
    if (! strftime(t, 1024, screen().getStrftimeFormat(), tt))
        return;
#else // !HAVE_STRFTIME
    I18n *i18n = I18n::instance();
    char t[9];
    if (date) {
        // format the date... with special consideration for y2k ;)
        if (screen().getDateFormat() == Fluxbox::B_EUROPEANDATE) {
            sprintf(t,
                    i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeDateFormatEu,
                                     "%02d.%02d.%02d"),
                    tt->tm_mday, tt->tm_mon + 1,
                    (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
        } else {
            sprintf(t,
                    i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeDateFormat,
                                     "%02d/%02d/%02d"),
                    tt->tm_mon + 1, tt->tm_mday,
                    (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
        }
    } else {
        if (screen().isClock24Hour()) {
            sprintf(t,
                    i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormat24,
                                     "	%02d:%02d "),
                    frame.hour, frame.minute);
        } else {
            sprintf(t,
                    i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormat12,
                                     "%02d:%02d %sm"),
                    ((frame.hour > 12) ? frame.hour - 12 :
                     ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
                    ((frame.hour >= 12) ?
                     i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormatP,
                                      "p") :
                     i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormatA,
                                      "a")));
        }
    }
#endif // HAVE_STRFTIME

    unsigned int newlen = strlen(t);
    int dx = FbTk::doAlignment(frame.clock_w,
                               frame.bevel_w*2,
                               m_theme.justify(),
                               m_theme.font(),
                               t, strlen(t), newlen);
    int dy = 1 + m_theme.font().ascent();
    if (m_theme.font().isRotated()) {
        int tmp = dy;
        dy = frame.clock.height() - dx;
        dx = tmp;
    }		
    frame.clock.clear();
    m_theme.font().drawText(frame.clock.window(),
                            screen().screenNumber(),
                            m_theme.clockTextGC(),
                            t, newlen,
                            dx, dy);

}


void Toolbar::redrawWindowLabel(bool redraw) {
    if (Fluxbox::instance()->getFocusedWindow()) {
        if (redraw)
            frame.window_label.clear();

        FluxboxWindow *foc = Fluxbox::instance()->getFocusedWindow();
        // don't draw focused window if it's not on the same screen
        if (&foc->screen() != &screen() || foc->title().size() == 0)
            return;
		
        unsigned int newlen = foc->title().size();
        int dx = FbTk::doAlignment(frame.window_label_w, frame.bevel_w*2,
                                   m_theme.justify(),
                                   m_theme.font(),
                                   foc->title().c_str(), 
                                   foc->title().size(), newlen);
	int dy = 1 + m_theme.font().ascent();

        if (m_theme.font().isRotated()) {
            int tmp = dy;
            dy = frame.window_label.height() - dx;
            dx = tmp + frame.bevel_w;
        } else 
            dy += frame.bevel_w;
    
        m_theme.font().drawText(frame.window_label.window(),
                                screen().screenNumber(),
                                m_theme.windowTextGC(),
                                foc->title().c_str(), newlen,
                                dx, dy);
    } else
        frame.window_label.clear();
}
 
 
void Toolbar::redrawWorkspaceLabel(bool redraw) {
    if (screen().currentWorkspace()->name().size()==0)
        return;
		
    if (redraw)
        frame.workspace_label.clear();
		
    const char *text = screen().currentWorkspace()->name().c_str();
    size_t textlen = screen().currentWorkspace()->name().size();
    unsigned int newlen = textlen;
    int dx = FbTk::doAlignment(frame.workspace_label_w, frame.bevel_w,
                               m_theme.justify(),
                               m_theme.font(),
                               text, textlen, newlen);
    int dy = 1 + m_theme.font().ascent();
    if (m_theme.font().isRotated()) {
        int tmp = dy;
        dy = frame.workspace_label_w - dx;
        dx = tmp;
    }
    m_theme.font().drawText(
                            frame.workspace_label.window(),
                            screen().screenNumber(),
                            m_theme.labelTextGC(),
                            text, newlen,
                            dx, dy);

}

void Toolbar::edit() {
    Window window;
    int foo;

    m_editing = true;	//mark for editing
    Display *display = FbTk::App::instance()->display();
    //workspace label already has intput focus ?
    if (XGetInputFocus(display, &window, &foo) &&
        window == frame.workspace_label)
        return;

    //set input focus to workspace label
    XSetInputFocus(display, frame.workspace_label.window(),
                   ((screen().isSloppyFocus() || screen().isSemiSloppyFocus()) ?
                    RevertToPointerRoot : RevertToParent), CurrentTime);

    frame.workspace_label.clear();
    Fluxbox * const fluxbox = Fluxbox::instance();
    if (fluxbox->getFocusedWindow())	//disable focus on current focused window
        fluxbox->getFocusedWindow()->setFocusFlag(false);

    frame.workspace_label.drawRectangle(screen().winFrameTheme().labelTextFocusGC(),
                                        frame.workspace_label_w / 2, 0, 1,
                                        frame.label_h - 1);
}


void Toolbar::buttonPressEvent(XButtonEvent &be) {
    FluxboxWindow *fluxboxwin=0;
    if (be.button == 1) {
        if ( m_iconbar.get() != 0 ) {
            if ( (fluxboxwin = m_iconbar->findWindow(be.window)) )
                fluxboxwin->deiconify();
        }
#ifndef	 HAVE_STRFTIME
        else if (be.window == frame.clock) {
            frame.clock.clear();
            checkClock(true, true);
        }
#endif // HAVE_STRFTIME

    } else if (be.button == 3) {
        FluxboxWindow *fluxboxwin = 0;
        // if we clicked on a icon then show window menu
        if ( m_iconbar.get() != 0 && (fluxboxwin = m_iconbar->findWindow(be.window)) ) {
            const FbTk::Menu &wm = fluxboxwin->menu();

            int menu_y = be.y_root - wm.height();
            int menu_x = be.x_root;
            // make sure the menu is visible
            if (menu_y < 0) {
                menu_y = 0;
            }
            if (menu_x < 0) {
                menu_x = 0;
            } else if (menu_x + wm.width() > screen().width()) {
                menu_x = screen().width() - wm.width();
            }
            fluxboxwin->showMenu(menu_x, menu_y);

        } else if (! m_toolbarmenu.isVisible()) {
            int x, y;

            x = be.x_root - (m_toolbarmenu.width() / 2);
            y = be.y_root - (m_toolbarmenu.height() / 2);

            if (x < 0)
                x = 0;
            else if (x + m_toolbarmenu.width() > screen().width())
                x = screen().width() - m_toolbarmenu.width();

            if (y < 0)
                y = 0;
            else if (y + m_toolbarmenu.height() > screen().height())
                y = screen().height() - m_toolbarmenu.height();

            m_toolbarmenu.move(x, y);
            m_toolbarmenu.show();
        } else
            m_toolbarmenu.hide();
			
    } 
	
}


void Toolbar::buttonReleaseEvent(XButtonEvent &re) {
    if (re.button == 1) {
        raise();
        if (re.window == frame.workspace_label) {
            FbTk::Menu *menu = screen().getWorkspacemenu();
            //move the workspace label and make it visible
            menu->move(re.x_root, re.y_root);
            // make sure the entire menu is visible 
            //!!TODO: this is repeated by other menus, make a function!)
            int newx = menu->x(); // new x position of menu
            int newy = menu->y(); // new y position of menu
            if (menu->x() < 0)
                newx = 0;
            else if (menu->x() + menu->width() > screen().width())
                newx = screen().width() - menu->width();
			
            if (menu->y() < 0)
                newy = 0;
            else if (menu->y() + menu->height() > screen().height())
                newy = screen().height() - menu->height();
            // move and show menu
            menu->move(newx, newy);
            menu->show();
        } else if (re.window == frame.window_label)
            screen().raiseFocus();
#ifndef	 HAVE_STRFTIME
        else if (re.window == frame.clock) {
            frame.clock.clear();
            checkClock(true);
        }
#endif // HAVE_STRFTIME
    } else if (re.button == 4) //mousewheel scroll up
        screen().nextWorkspace(1);
    else if (re.button == 5)	//mousewheel scroll down
        screen().prevWorkspace(1);
}


void Toolbar::enterNotifyEvent(XCrossingEvent &not_used) {
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (! m_hide_timer.isTiming())
            m_hide_timer.start();
    } else {
        if (m_hide_timer.isTiming())
            m_hide_timer.stop();
    }
}

void Toolbar::leaveNotifyEvent(XCrossingEvent &not_used) {
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (m_hide_timer.isTiming()) 
            m_hide_timer.stop();
    } else if (! m_toolbarmenu.isVisible() && ! m_hide_timer.isTiming()) 
        m_hide_timer.start();

}


void Toolbar::exposeEvent(XExposeEvent &ee) {
    if (ee.window == frame.clock) 
        checkClock(true);
    else if (ee.window == frame.workspace_label && (! isEditing()))
        redrawWorkspaceLabel();
    else if (m_iconbar.get() != 0)
        m_iconbar->exposeEvent(&ee);
}


void Toolbar::keyPressEvent(XKeyEvent &ke) {
    if (ke.window != frame.workspace_label.window() || !isEditing())
        return;
		
    KeySym ks;
    char keychar[1];
    XLookupString(&ke, keychar, 1, &ks, 0);

    if (ks == XK_Return || ks == XK_Escape) {			
        m_editing = false;
        Fluxbox * const fluxbox = Fluxbox::instance();			

        if (fluxbox->getFocusedWindow()) {
            fluxbox->getFocusedWindow()->setInputFocus();
            fluxbox->getFocusedWindow()->setFocusFlag(true);
        } else
            XSetInputFocus(FbTk::App::instance()->display(), PointerRoot, None, CurrentTime);
			
        if (ks == XK_Return)	//change workspace name if keypress = Return
            screen().currentWorkspace()->setName(m_new_workspace_name.c_str());

        m_new_workspace_name.erase(); //erase temporary workspace name
        reconfigure();
        //save workspace names
        Fluxbox::instance()->save_rc();

    } else if (! IsModifierKey(ks) && !IsCursorKey(ks)) {

        if (ks == XK_BackSpace && m_new_workspace_name.size())
            m_new_workspace_name.erase(m_new_workspace_name.size() - 1);
        else
            m_new_workspace_name += keychar[0];


        frame.workspace_label.clear();
        int l = m_new_workspace_name.size(), tw, x;

        tw = m_theme.font().textWidth(m_new_workspace_name.c_str(), l);
        x = (frame.workspace_label_w - tw) / 2;

        if (x < (signed) frame.bevel_w)
            x = frame.bevel_w;
        int dy = 1 + m_theme.font().ascent();
        if (m_theme.font().isRotated()) {
            int tmp = dy;
            dy = frame.workspace_label_w - x;
            x = tmp;
        }

        m_theme.font().drawText(frame.workspace_label.window(),
                                screen().screenNumber(),
                                screen().winFrameTheme().labelTextFocusGC(),
                                m_new_workspace_name.c_str(), l,
                                x, dy);

        frame.workspace_label.drawRectangle(screen().winFrameTheme().labelTextFocusGC(),
                                            x + tw, 0, 1,
                                            frame.label_h - 1);
    }		
}


void Toolbar::timeout() {
    checkClock(true);

    timeval delay;
    delay.tv_sec = 1;
    delay.tv_usec = 0;	
    m_clock_timer.setTimeout(delay);
}


void Toolbar::setPlacement(Toolbar::Placement where) {
    *m_rc_placement = where;
    int head_x = 0,
        head_y = 0,
        head_w,
        head_h;

#ifdef XINERAMA
    if (screen().hasXinerama()) {
        int head = *m_rc_on_head;
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    } else 
#endif // XINERAMA
    {
        head_w = screen().width();
        head_h = screen().height();
    }


    frame.width = head_w * (*m_rc_width_percent) / 100;
    frame.height = m_theme.font().height();

    frame.height += 2;
    frame.height += (frame.bevel_w * 2);

    int bevel_width = theme().bevelWidth();
    int border_width = theme().borderWidth();

    // should we flipp sizes?
    if (isVertical()) {
        frame.width = frame.height;
        frame.height = head_h * (*m_rc_width_percent) / 100;
        if (!m_theme.font().isRotated())
            m_theme.font().rotate(90); // rotate to vertical text

        frame.label_h = frame.width;
        frame.button_w = frame.width;
    } else { // horizontal toolbar
        if (m_theme.font().isRotated()) 
            m_theme.font().rotate(0); // rotate to horizontal text
        frame.label_h = frame.height;
        frame.button_w = frame.height;    
    }

    // So we get at least one pixel visible in hidden mode
    if (bevel_width <= border_width)
        bevel_width = border_width + 1;

    switch (where) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;

    case TOPCENTER:
        frame.x = head_x + (head_w - frame.width) / 2;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;
    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;

    case BOTTOMCENTER: // default is BOTTOMCENTER
    default:
        frame.x = head_x + (head_w - frame.width) / 2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;
    case LEFTCENTER:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case LEFTTOP:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case LEFTBOTTOM:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case RIGHTCENTER:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    case RIGHTTOP:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    case RIGHTBOTTOM:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    }
}

void Toolbar::HideHandler::timeout() {
    if (toolbar->isEditing()) { // don't hide if we're editing workspace label
        toolbar->m_hide_timer.fireOnce(false);
        toolbar->m_hide_timer.start(); // restart timer and try next timeout
        return;
    }

    toolbar->m_hide_timer.fireOnce(true);

    // toggle hidden
    toolbar->m_hidden = ! toolbar->m_hidden;
    if (toolbar->isHidden()) {
        toolbar->frame.window.move(toolbar->frame.x_hidden, toolbar->frame.y_hidden);
    } else {
        toolbar->frame.window.move(toolbar->frame.x, toolbar->frame.y);
    }
}

void Toolbar::moveToLayer(int layernum) {
    m_layeritem.moveToLayer(layernum); 
    *m_rc_layernum = layernum;
}

void Toolbar::setupMenus() {
    Toolbar &tbar = *this;
    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    using namespace FbTk;

    FbTk::Menu &menu = tbar.menu();
    
    RefCount<Command> start_edit(new SimpleCommand<Toolbar>(tbar, &Toolbar::edit));
    menu.insert(i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarEditWkspcName,
                                 "Edit current workspace name"),
                start_edit);

    menu.setLabel(i18n->getMessage(FBNLS::ToolbarSet, FBNLS::ToolbarToolbarTitle,
                                   "Toolbar")); 

    FbTk::MenuItem *toolbar_menuitem = new IntResMenuItem("Toolbar width percent",
                                                          m_rc_width_percent,
                                                          0, 100); // min/max value


    FbTk::RefCount<FbTk::Command> reconfig_toolbar(new FbTk::
                                                   SimpleCommand<Toolbar>
                                                   (tbar, &Toolbar::reconfigure));
    FbTk::RefCount<FbTk::Command> save_resources(new FbCommands::SaveResources());
    FbTk::MacroCommand *toolbar_menuitem_macro = new FbTk::MacroCommand();
    toolbar_menuitem_macro->add(reconfig_toolbar);
    toolbar_menuitem_macro->add(save_resources);

    FbTk::RefCount<FbTk::Command> reconfig_toolbar_and_save_resource(toolbar_menuitem_macro);
    toolbar_menuitem->setCommand(reconfig_toolbar_and_save_resource);  

    menu.insert(toolbar_menuitem);

    menu.insert(new BoolMenuItem(i18n->getMessage(FBNLS::CommonSet, FBNLS::CommonAutoHide,
                                                  "Auto hide"),
                                 *m_rc_auto_hide,
                                 reconfig_toolbar_and_save_resource));

    menu.insert("Layer...", &tbar.layermenu());

    if (tbar.screen().hasXinerama()) {
        menu.insert("On Head...", new XineramaHeadMenu<Toolbar>(
                        *tbar.screen().menuTheme(),
                        tbar.screen(),
                        tbar.screen().imageControl(),
                        *tbar.screen().layerManager().getLayer(Fluxbox::instance()->getMenuLayer()),
                        &tbar
                        ));
    }

    // setup items in placement menu
    struct {
        int set;
        int base;
        const char *default_str;
        Toolbar::Placement placement;
    } place_menu[]  = {
        {0, 0, "Top Left", Toolbar::TOPLEFT},
        {0, 0, "Left Top", Toolbar::LEFTTOP},
        {0, 0, "Left Center", Toolbar::LEFTCENTER},
        {0, 0, "Left Bottom", Toolbar::LEFTBOTTOM}, 
        {0, 0, "Bottom Left", Toolbar::BOTTOMLEFT},
        {0, 0, "Top Center", Toolbar::TOPCENTER},
        {0, 0, 0, Toolbar::TOPCENTER},
        {0, 0, 0, Toolbar::BOTTOMCENTER},
        {0, 0, 0, Toolbar::BOTTOMCENTER},
        {0, 0, "Bottom Center", Toolbar::BOTTOMCENTER},
        {0, 0, "Top Right", Toolbar::TOPRIGHT},
        {0, 0, "Right Top", Toolbar::RIGHTTOP},
        {0, 0, "Right Center", Toolbar::RIGHTCENTER},
        {0, 0, "Right Bottom", Toolbar::RIGHTBOTTOM},
        {0, 0, "Bottom Right", Toolbar::BOTTOMRIGHT}
    };
    tbar.placementMenu().setMinimumSublevels(3);
    // create items in sub menu
    for (size_t i=0; i<15; ++i) {
        if (place_menu[i].default_str == 0) {
            tbar.placementMenu().insert("");
        } else {
            const char *i18n_str = i18n->getMessage(place_menu[i].set, 
                                                    place_menu[i].base,
                                                    place_menu[i].default_str);
            RefCount<FbTk::Command> setplace(new SetToolbarPlacementCmd(tbar, place_menu[i].placement));
            tbar.placementMenu().insert(i18n_str, setplace);
                                                              
        }
    }
    menu.insert("Placement", &tbar.placementMenu());
    tbar.placementMenu().update();
    menu.update();
}
