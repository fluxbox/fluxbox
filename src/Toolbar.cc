// Toolbar.cc for Fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Toolbar.cc for Blackbox - an X11 Window manager
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

// $Id: Toolbar.cc,v 1.49 2002/12/04 22:36:47 fluxgen Exp $

#include "Toolbar.hh"

#include "i18n.hh"
#include "fluxbox.hh"
#include "Clientmenu.hh"
#include "Iconmenu.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "ImageControl.hh"
#include "ToolbarTheme.hh"
#include "EventManager.hh"
#include "Text.hh"

// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

// toolbar frame
Toolbar::Frame::Frame(FbTk::EventHandler &evh, int screen_num):
    window(screen_num, // screen (parent)
           0, 0, // pos
           1, 1, //
           ButtonPressMask | ButtonReleaseMask | 
           EnterWindowMask | LeaveWindowMask,
           true), // override redirect 
    workspace_label(window, // parent
                    0, 0, //pos
                    1, 1, // size
                    ButtonPressMask | ButtonReleaseMask | 
                    ExposureMask | KeyPressMask),
    window_label(window, // parent
                  0, 0, // pos
                  1, 1, // size
                  ButtonPressMask | ButtonReleaseMask | 
                  ExposureMask),
    clock (window, //parent
            0, 0, // pos
            1, 1, // size
            ButtonPressMask | ButtonReleaseMask | 
            ExposureMask),
    psbutton(window, // parent
             0, 0, // pos
             1, 1, // size
              ButtonPressMask | ButtonReleaseMask | 
             ExposureMask),
    nsbutton(window,
             0, 0,
             1, 1,
             ButtonPressMask | ButtonReleaseMask | 
             ExposureMask),
    pwbutton(window,
             0, 0,
             1, 1,
             ButtonPressMask | ButtonReleaseMask | 
             ExposureMask),
    nwbutton(window,
              0, 0,
              1, 1,
              ButtonPressMask | ButtonReleaseMask | 
              ExposureMask) 
{
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(evh, window);
    evm.add(evh, workspace_label);
    evm.add(evh, window_label);
    evm.add(evh, clock);
    evm.add(evh, psbutton);
    evm.add(evh, nsbutton);
    evm.add(evh, pwbutton);
    evm.add(evh, nwbutton);
    
}

Toolbar::Frame::~Frame() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(window);
    evm.remove(workspace_label);
    evm.remove(window_label);
    evm.remove(clock);
    evm.remove(psbutton);
    evm.remove(nsbutton);
    evm.remove(pwbutton);
    evm.remove(nwbutton);
}

Toolbar::Toolbar(BScreen *scrn, size_t width):
    on_top(scrn->isToolbarOnTop()),
    editing(false),
    hidden(scrn->doToolbarAutoHide()), 
    do_auto_hide(scrn->doToolbarAutoHide()),
    frame(*this, scrn->getScreenNumber()),
    m_screen(scrn),
    image_ctrl(*scrn->getImageControl()),
    clock_timer(this), 	// get the clock updating every minute
    hide_timer(&hide_handler),
    m_toolbarmenu(*this),
    m_theme(scrn->getScreenNumber()),
    m_place(BOTTOMCENTER) {

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
    clock_timer.setTimeout(delay);
    clock_timer.start();

    hide_handler.toolbar = this;
    hide_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay());
    hide_timer.fireOnce(true);

    frame.grab_x = frame.grab_y = 0;

    display = FbTk::App::instance()->display();

    frame.base = frame.label = frame.wlabel = frame.clk = frame.button =
        frame.pbutton = None;

		
    if (Fluxbox::instance()->useIconBar())
        m_iconbar.reset(new IconBar(screen(), frame.window_label.window()));


    XMapSubwindows(display, frame.window.window());
    frame.window.show();

    reconfigure();
	
}


Toolbar::~Toolbar() {
    frame.window.hide();
	 
    if (frame.base) image_ctrl.removeImage(frame.base);
    if (frame.label) image_ctrl.removeImage(frame.label);
    if (frame.wlabel) image_ctrl.removeImage(frame.wlabel);
    if (frame.clk) image_ctrl.removeImage(frame.clk);
    if (frame.button) image_ctrl.removeImage(frame.button);
    if (frame.pbutton) image_ctrl.removeImage(frame.pbutton);

}

bool Toolbar::isVertical() const {
    return (m_place == RIGHTCENTER ||
            m_place == RIGHTTOP ||
            m_place == RIGHTBOTTOM ||
            m_place == LEFTCENTER ||
            m_place == LEFTTOP ||
            m_place == LEFTBOTTOM);
}

void Toolbar::addIcon(FluxboxWindow *w) {
    if (m_iconbar.get() != 0)
        FbTk::EventManager::instance()->add(*this, m_iconbar->addIcon(w));
}

void Toolbar::delIcon(FluxboxWindow *w) {
    if (m_iconbar.get() != 0)
        FbTk::EventManager::instance()->remove(m_iconbar->delIcon(w));
}
		
void Toolbar::reconfigure() {

    frame.bevel_w = screen()->getBevelWidth();

    // recallibrate size
    setPlacement(m_place);

#ifdef HAVE_STRFTIME
    time_t ttmp = time(0);
    struct tm *tt = 0;

    if (ttmp != -1) {
        tt = localtime(&ttmp);
        if (tt) {
            char t[1024], *time_string = (char *) 0;
            int len = strftime(t, 1024, screen()->getStrftimeFormat(), tt);

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
	
    frame.clock_w = m_theme.font().textWidth(
                                             i18n->getMessage(
                                                              ToolbarSet, ToolbarNoStrftimeLength,
                                                              "00:00000"),
                                             strlen(i18n->getMessage(
                                                                     ToolbarSet, ToolbarNoStrftimeLength,
                                                                     "00:00000"))) + (frame.bevel_w * 4);
	
#endif // HAVE_STRFTIME

    unsigned int i;
    unsigned int w = 0;
    frame.workspace_label_w = 0;

    for (i = 0; i < screen()->getCount(); i++) {
        w = m_theme.font().textWidth(
                                     screen()->getWorkspace(i)->name().c_str(),
                                     screen()->getWorkspace(i)->name().size());

        w += (frame.bevel_w * 4);

        if (w > frame.workspace_label_w)
            frame.workspace_label_w = w;
    }
    
    if (frame.workspace_label_w < frame.clock_w)
        frame.workspace_label_w = frame.clock_w;
    else if (frame.workspace_label_w > frame.clock_w)
        frame.clock_w = frame.workspace_label_w;

    frame.window_label_w =
        (frame.width - (frame.clock_w + (frame.button_w * 4) +
                        frame.workspace_label_w + (frame.bevel_w * 8) + 6));
    
    if (hidden)
        frame.window.moveResize(frame.x_hidden, frame.y_hidden,
                                frame.width, frame.height);
    else {
        frame.window.moveResize(frame.x, frame.y,
                                frame.width, frame.height);
    }

    bool vertical = isVertical();
    unsigned int next_x = frame.workspace_label_w;
    unsigned int next_y = frame.window.height();
    
    if (vertical) {
        next_x = frame.window.width();
        next_y = frame.workspace_label_w;
    }
    
    frame.workspace_label.moveResize(frame.bevel_w, frame.bevel_w, next_x, next_y);
    next_x = 0;
    next_y = 0;
    if (vertical) {
        next_y += frame.workspace_label.height() + 1;
    } else {
        next_x += frame.workspace_label.width() + 1;
    }

    frame.psbutton.moveResize(frame.bevel_w * 2 +
                              next_x , next_y,
                              frame.button_w, frame.button_w);
    if (vertical)
        next_y += frame.psbutton.height() + 1;
    else
        next_x += frame.psbutton.width() + 1;

    frame.nsbutton.moveResize(frame.bevel_w * 3 +
                              next_x, next_y,
                              frame.button_w, frame.button_w);
    size_t label_w = frame.window_label_w;
    size_t label_h = frame.height;

    if (vertical) {
        next_y += frame.nsbutton.height() + 1;
        label_w = frame.width;
        label_h = frame.window_label_w - frame.width + frame.height;

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

    frame.clock.moveResize(next_x, next_y,
                           clock_w, clock_h);

    Pixmap tmp = frame.base;
    const FbTk::Texture *texture = &(m_theme.toolbar());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.base = None;
        frame.window.setBackgroundColor(texture->color());
    } else {
        frame.base = image_ctrl.renderImage(frame.window.width(), frame.window.height(), *texture);
        frame.window.setBackgroundPixmap(frame.base);
    }
    if (tmp) image_ctrl.removeImage(tmp);
    
    tmp = frame.label;
    texture = &(m_theme.window());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.label = None;
        frame.window_label.setBackgroundColor(texture->color());
    } else {
        frame.label =
            image_ctrl.renderImage(frame.window_label.width(), frame.window_label.height(), *texture);
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

    tmp = frame.pbutton;
    texture = &(m_theme.pressedButton());
    if (texture->type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        frame.pbutton = None;
    } else
        frame.pbutton =
            image_ctrl.renderImage(frame.button_w, frame.button_w, *texture);
    if (tmp) 
        image_ctrl.removeImage(tmp);

    frame.window.setBorderColor(*screen()->getBorderColor());
    frame.window.setBorderWidth(screen()->getBorderWidth());

    frame.window.clear();
    frame.workspace_label.clear();
    frame.window_label.clear();
    frame.clock.clear();
    frame.psbutton.clear();
    frame.nsbutton.clear();
    frame.pwbutton.clear();
    frame.nwbutton.clear();
	
    redrawWindowLabel();
    redrawWorkspaceLabel();
    redrawPrevWorkspaceButton();
    redrawNextWorkspaceButton();
    redrawPrevWindowButton();
    redrawNextWindowButton();
    checkClock(true);

    m_toolbarmenu.reconfigure();

    if (Fluxbox::instance()->useIconBar()) {
        if (m_iconbar.get() == 0) { // create new iconbar if we don't have one
            m_iconbar.reset(new IconBar(screen(), frame.window_label.window()));
            if (screen()->getIconCount()) {
                BScreen::Icons & l = screen()->getIconList();
                BScreen::Icons::iterator it = l.begin();
                BScreen::Icons::iterator it_end = l.end();
                for(; it != it_end; ++it) {
                    addIcon(*it);
                }
            }
			
        } else 
            m_iconbar->reconfigure();
    } else if (m_iconbar.get() != 0) {
        BScreen::Icons & l = screen()->getIconList();
        BScreen::Icons::iterator it = l.begin();
        BScreen::Icons::iterator it_end = l.end();
        for(; it != it_end; ++it)
            delIcon(*it);

        m_iconbar.reset(0); // destroy iconbar
    }

}



void Toolbar::checkClock(bool redraw, bool date) {
    time_t tmp = 0;
    struct tm *tt = 0;

    if ((tmp = time(NULL)) != -1) {
        if (! (tt = localtime(&tmp))) {
            cerr<<__FILE__<<"("<<__LINE__<<"): ! localtime(&tmp)"<<endl;
            return;
        }
        if (tt->tm_min != frame.minute || tt->tm_hour != frame.hour) {
            frame.hour = tt->tm_hour;
            frame.minute = tt->tm_min;
            frame.clock.clear();
            redraw = true;
        }
    } else
        cerr<<__FILE__<<"("<<__LINE__<<"): time(null)<0"<<endl;
	

    if (redraw) {
        frame.clock.clear();
#ifdef HAVE_STRFTIME
        char t[1024];
        if (! strftime(t, 1024, screen()->getStrftimeFormat(), tt))
            return;
#else // !HAVE_STRFTIME
        char t[9];
        if (date) {
            // format the date... with special consideration for y2k ;)
            if (screen()->getDateFormat() == Blackbox::B_EuropeanDate) {
                sprintf(t,
                        i18n->getMessage(
                            ToolbarSet, ToolbarNoStrftimeDateFormatEu,
                            "%02d.%02d.%02d"),
                        tt->tm_mday, tt->tm_mon + 1,
                        (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
            } else {
                sprintf(t,
                        i18n->getMessage(
                            ToolbarSet, ToolbarNoStrftimeDateFormat,
                            "%02d/%02d/%02d"),
                        tt->tm_mon + 1, tt->tm_mday,
                        (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
            }
        } else {
            if (screen()->isClock24Hour()) {
                sprintf(t,
                        i18n->getMessage(
                            ToolbarSet, ToolbarNoStrftimeTimeFormat24,
                            "	%02d:%02d "),
                        frame.hour, frame.minute);
            } else {
                sprintf(t,
                        i18n->getMessage(
                            ToolbarSet, ToolbarNoStrftimeTimeFormat12,
                            "%02d:%02d %sm"),
                        ((frame.hour > 12) ? frame.hour - 12 :
                         ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
                        ((frame.hour >= 12) ?
                         i18n->getMessage(
                             ToolbarSet, ToolbarNoStrftimeTimeFormatP,
                             "p") :
                         i18n->getMessage(
                             ToolbarSet, ToolbarNoStrftimeTimeFormatA,
                             "a")));
            }
        }
#endif // HAVE_STRFTIME

        size_t newlen = strlen(t);
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
        m_theme.font().drawText(
            frame.clock.window(),
            screen()->getScreenNumber(),
            m_theme.clockTextGC(),
            t, newlen,
            dx, dy);
    }
}


void Toolbar::redrawWindowLabel(bool redraw) {
    if (Fluxbox::instance()->getFocusedWindow()) {
        if (redraw)
            frame.window_label.clear();

        FluxboxWindow *foc = Fluxbox::instance()->getFocusedWindow();
        if (foc->getScreen() != screen() || foc->getTitle().size() == 0)
            return;
		
        size_t newlen = foc->getTitle().size();
        int dx = FbTk::doAlignment(frame.window_label_w, frame.bevel_w*2,
                                       m_theme.justify(),
                                       m_theme.font(),
                                       foc->getTitle().c_str(), foc->getTitle().size(), newlen);
	int dy = 1 + m_theme.font().ascent();

        if (m_theme.font().isRotated()) {
            int tmp = dy;
            dy = frame.window_label.height() - dx;
            dx = tmp;
        }
    
        m_theme.font().drawText(
            frame.window_label.window(),
            screen()->getScreenNumber(),
            m_theme.windowTextGC(),
            foc->getTitle().c_str(), newlen,
            dx, dy);
    } else
        frame.window_label.clear();
}
 
 
void Toolbar::redrawWorkspaceLabel(bool redraw) {
    if (screen()->getCurrentWorkspace()->name().size()==0)
        return;
		
    if (redraw)
        frame.workspace_label.clear();
		
    const char *text = screen()->getCurrentWorkspace()->name().c_str();
    size_t textlen = screen()->getCurrentWorkspace()->name().size();
    size_t newlen = textlen;
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
        screen()->getScreenNumber(),
        m_theme.labelTextGC(),
        text, newlen,
        dx, dy);
}


void Toolbar::redrawPrevWorkspaceButton(bool pressed, bool redraw) {
    if (redraw) {
        drawButtonBase(frame.psbutton, pressed);
    }

    int hh = frame.button_w / 2, hw = frame.button_w / 2;

    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh;
    pts[1].x = 4; pts[1].y = 2;
    pts[2].x = 0; pts[2].y = -4;

    XFillPolygon(display, frame.psbutton.window(), m_theme.buttonPicGC(),
                 pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWorkspaceButton(bool pressed, bool redraw) {
    if (redraw) {
        drawButtonBase(frame.nsbutton, pressed);
    }

    int hh = frame.button_w / 2, hw = frame.button_w / 2;

    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh - 2;
    pts[1].x = 4; pts[1].y =	2;
    pts[2].x = -4; pts[2].y = 2;

    XFillPolygon(display, frame.nsbutton.window(), m_theme.buttonPicGC(),
                 pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawPrevWindowButton(bool pressed, bool redraw) {
    if (redraw) {
        drawButtonBase(frame.pwbutton, pressed);
    }

    int hh = frame.button_w / 2, hw = frame.button_w / 2;

    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh;
    pts[1].x = 4; pts[1].y = 2;
    pts[2].x = 0; pts[2].y = -4;

    XFillPolygon(display, frame.pwbutton.window(), m_theme.buttonPicGC(),
                 pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWindowButton(bool pressed, bool redraw) {
    if (redraw) {
        drawButtonBase(frame.nwbutton, pressed);
    }

    int hh = frame.button_w / 2, hw = frame.button_w / 2;

    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh - 2;
    pts[1].x = 4; pts[1].y =	2;
    pts[2].x = -4; pts[2].y = 2;

    XFillPolygon(display, frame.nwbutton.window(), m_theme.buttonPicGC(),
                 pts, 3, Convex, CoordModePrevious);
}

void Toolbar::drawButtonBase(FbTk::FbWindow &win, bool pressed) {
    if (pressed) {
        if (frame.pbutton)
            win.setBackgroundPixmap(frame.pbutton);
        else
            win.setBackgroundColor(m_theme.pressedButton().color());
    } else {
        if (frame.button)            
            win.setBackgroundPixmap(frame.button);            
        else
            win.setBackgroundColor(m_theme.button().color());
    }
    win.clear();

}

void Toolbar::edit() {
    Window window;
    int foo;

    editing = true;	//mark for editing
	
    //workspace label already has intput focus ?
    if (XGetInputFocus(display, &window, &foo) &&
        window == frame.workspace_label)
        return;

    //set input focus to workspace label
    XSetInputFocus(display, frame.workspace_label.window(),
                   ((screen()->isSloppyFocus() || screen()->isSemiSloppyFocus()) ?
                    RevertToPointerRoot : RevertToParent), CurrentTime);

    frame.workspace_label.clear();
    Fluxbox * const fluxbox = Fluxbox::instance();
    fluxbox->setNoFocus(true);
    if (fluxbox->getFocusedWindow())	//disable focus on current focused window
        fluxbox->getFocusedWindow()->setFocusFlag(false);

    XDrawRectangle(display, frame.workspace_label.window(),
                   screen()->getWindowStyle()->l_text_focus_gc,
                   frame.workspace_label_w / 2, 0, 1,
                   frame.label_h - 1);
}


void Toolbar::buttonPressEvent(XButtonEvent &be) {
    FluxboxWindow *fluxboxwin=0;
    if (be.button == 1) {
        if (be.window == frame.psbutton)
            redrawPrevWorkspaceButton(true, true);
        else if (be.window == frame.nsbutton)
            redrawNextWorkspaceButton(true, true);
        else if (be.window == frame.pwbutton)
            redrawPrevWindowButton(true, true);
        else if (be.window == frame.nwbutton)
            redrawNextWindowButton(true, true);
        else if ( m_iconbar.get() != 0 ) {
            if ( (fluxboxwin = m_iconbar->findWindow(be.window)) )
                fluxboxwin->deiconify();
        }
#ifndef	 HAVE_STRFTIME
        else if (be.window == frame.clock) {
            frame.clock.clear();
            checkClock(true, true);
        }
#endif // HAVE_STRFTIME
        else if (! on_top) {
            Workspace::Stack st;
            st.push_back(frame.window.window());
            screen()->raiseWindows(st);
        }
    } else if (be.button == 2 && (! on_top)) {
        frame.window.lower();
    } else if (be.button == 3) {
        FluxboxWindow *fluxboxwin = 0;
        // if we clicked on a icon then show window menu
        if ( m_iconbar.get() != 0 && (fluxboxwin = m_iconbar->findWindow(be.window)) ) {
            const Windowmenu * const wm = fluxboxwin->getWindowmenu();
            if (wm != 0) {
                int menu_y = be.y_root - wm->height();
                int menu_x = be.x_root;
                // make sure the menu is visible
                if (menu_y < 0) {
                    menu_y = 0;
                }
                if (menu_x < 0) {
                    menu_x = 0;
                } else if (menu_x + wm->width() > screen()->getWidth()) {
                    menu_x = screen()->getWidth() - wm->width();
                }
                fluxboxwin->showMenu(menu_x, menu_y);
            }
        } else if (! m_toolbarmenu.isVisible()) {
            int x, y;

            x = be.x_root - (m_toolbarmenu.width() / 2);
            y = be.y_root - (m_toolbarmenu.height() / 2);

            if (x < 0)
                x = 0;
            else if (x + m_toolbarmenu.width() > screen()->getWidth())
                x = screen()->getWidth() - m_toolbarmenu.width();

            if (y < 0)
                y = 0;
            else if (y + m_toolbarmenu.height() > screen()->getHeight())
                y = screen()->getHeight() - m_toolbarmenu.height();

            m_toolbarmenu.move(x, y);
            m_toolbarmenu.show();
        } else
            m_toolbarmenu.hide();
			
    } 
	
}


void Toolbar::buttonReleaseEvent(XButtonEvent &re) {
    if (re.button == 1) {
        if (re.window == frame.psbutton) {
            redrawPrevWorkspaceButton(false, true);

            if (re.x >= 0 && re.x < (signed) frame.button_w &&
                re.y >= 0 && re.y < (signed) frame.button_w)
                screen()->prevWorkspace(1);
        } else if (re.window == frame.nsbutton) {
            redrawNextWorkspaceButton(false, true);

            if (re.x >= 0 && re.x < (signed) frame.button_w &&
                re.y >= 0 && re.y < (signed) frame.button_w)
                screen()->nextWorkspace(1);
        } else if (re.window == frame.pwbutton) {
            redrawPrevWindowButton(false, true);

            if (re.x >= 0 && re.x < (signed) frame.button_w &&
                re.y >= 0 && re.y < (signed) frame.button_w)
                screen()->prevFocus();
        } else if (re.window == frame.nwbutton) {
            redrawNextWindowButton(false, true);

            if (re.x >= 0 && re.x < (signed) frame.button_w &&
                re.y >= 0 && re.y < (signed) frame.button_w)
                screen()->nextFocus();
        } else if (re.window == frame.workspace_label) {
            Basemenu *menu = screen()->getWorkspacemenu();
            //move the workspace label and make it visible
            menu->move(re.x_root, re.y_root);
            // make sure the entire menu is visible (TODO: this is repeated by other menus, make a function!)
            int newx = menu->x(); // new x position of menu
            int newy = menu->y(); // new y position of menu
            if (menu->x() < 0)
                newx = 0;
            else if (menu->x() + menu->width() > screen()->getWidth())
                newx = screen()->getWidth() - menu->width();
			
            if (menu->y() < 0)
                newy = 0;
            else if (menu->y() + menu->height() > screen()->getHeight())
                newy = screen()->getHeight() - menu->height();
            // move and show menu
            menu->move(newx, newy);
            menu->show();
        } else if (re.window == frame.window_label)
            screen()->raiseFocus();
#ifndef	 HAVE_STRFTIME
        else if (re.window == frame.clock) {
            XClearWindow(display, frame.clock);
            checkClock(true);
        }
#endif // HAVE_STRFTIME
    } else if (re.button == 4) //mousewheel scroll up
        screen()->nextWorkspace(1);
    else if (re.button == 5)	//mousewheel scroll down
        screen()->prevWorkspace(1);
}


void Toolbar::enterNotifyEvent(XCrossingEvent &not_used) {
    if (! do_auto_hide)
        return;

    if (hidden) {
        if (! hide_timer.isTiming())
            hide_timer.start();
    } else {
        if (hide_timer.isTiming())
            hide_timer.stop();
    }
}

void Toolbar::leaveNotifyEvent(XCrossingEvent &not_used) {
    if (! do_auto_hide)
        return;

    if (hidden) {
        if (hide_timer.isTiming()) 
            hide_timer.stop();
    } else if (! m_toolbarmenu.isVisible() && ! hide_timer.isTiming()) 
        hide_timer.start();

}


void Toolbar::exposeEvent(XExposeEvent &ee) {
    if (ee.window == frame.clock) 
        checkClock(true);
    else if (ee.window == frame.workspace_label && (! editing))
        redrawWorkspaceLabel();
    else if (ee.window == frame.window_label) redrawWindowLabel();
    else if (ee.window == frame.psbutton) redrawPrevWorkspaceButton();
    else if (ee.window == frame.nsbutton) redrawNextWorkspaceButton();
    else if (ee.window == frame.pwbutton) redrawPrevWindowButton();
    else if (ee.window == frame.nwbutton) redrawNextWindowButton();
    else if (m_iconbar.get() != 0)
        m_iconbar->exposeEvent(&ee);
}


void Toolbar::keyPressEvent(XKeyEvent &ke) {
    if (ke.window == frame.workspace_label && editing) {
		
        KeySym ks;
        char keychar[1];
        XLookupString(&ke, keychar, 1, &ks, 0);

        if (ks == XK_Return || ks == XK_Escape) {
			

            editing = false;
            Fluxbox * const fluxbox = Fluxbox::instance();			
            fluxbox->setNoFocus(false);
            if (fluxbox->getFocusedWindow()) {
                fluxbox->getFocusedWindow()->setInputFocus();
                fluxbox->getFocusedWindow()->setFocusFlag(true);
            } else
                XSetInputFocus(display, PointerRoot, None, CurrentTime);
			
            if (ks == XK_Return)	//change workspace name if keypress = Return
                screen()->getCurrentWorkspace()->setName(new_workspace_name.c_str());

            new_workspace_name.erase(); //erase temporary workspace name
			
            screen()->getCurrentWorkspace()->menu().hide();
            screen()->getWorkspacemenu()->
                remove(screen()->getCurrentWorkspace()->workspaceID() + 2);
            screen()->getWorkspacemenu()->
                insert(screen()->getCurrentWorkspace()->name().c_str(),
                       &screen()->getCurrentWorkspace()->menu(),
                       screen()->getCurrentWorkspace()->workspaceID() + 2);
            screen()->getWorkspacemenu()->update();

            reconfigure();
            //save workspace names
            Fluxbox::instance()->save_rc();

        } else if (! IsModifierKey(ks) && !IsCursorKey(ks)) {

            if (ks == XK_BackSpace && new_workspace_name.size())
                new_workspace_name.erase(new_workspace_name.size()-1);
            else
                new_workspace_name += keychar[0];


            frame.workspace_label.clear();
            int l = new_workspace_name.size(), tw, x;

            tw = m_theme.font().textWidth(new_workspace_name.c_str(), l);
            x = (frame.workspace_label_w - tw) / 2;

            if (x < (signed) frame.bevel_w)
                x = frame.bevel_w;

            m_theme.font().drawText(
                frame.workspace_label.window(),
                screen()->getScreenNumber(),
                screen()->getWindowStyle()->l_text_focus_gc,
                new_workspace_name.c_str(), l,
                x, 1 + m_theme.font().ascent()); 

            XDrawRectangle(display, frame.workspace_label.window(),
                           screen()->getWindowStyle()->l_text_focus_gc, x + tw, 0, 1,
                           frame.label_h - 1);
        }		
		
    }
}


void Toolbar::timeout() {
    checkClock(true);

    timeval delay;
    delay.tv_sec = 1;
    delay.tv_usec = 0;	
    clock_timer.setTimeout(delay);
}


void Toolbar::setPlacement(Toolbar::Placement where) {
    int head_x = 0,
        head_y = 0,
        head_w,
        head_h;

    m_place = where;

#ifdef XINERAMA
    int head = (screen->hasXinerama())
        ? screen->getToolbarOnHead()
        : -1;

    if (head >= 0) { // toolbar on head nr, if -1 then over ALL heads
        head_x = screen()->getHeadX(head);
        head_y = screen()->getHeadY(head);
        head_w = screen()->getHeadWidth(head);
        head_h = screen()->getHeadHeight(head);
    }	else {
        head_w = screen->getWidth();
        head_h = screen->getHeight();
    }
#else // !XINERAMA
    head_w = screen()->getWidth();
    head_h = screen()->getHeight();
#endif // XINERAMA

    frame.width = head_w * screen()->getToolbarWidthPercent() / 100;
    frame.height = m_theme.font().height();

    frame.height += 2;
    frame.height += (frame.bevel_w * 2);


    // should we flipp sizes?
    if (isVertical()) {
        frame.width = frame.height;
        frame.height = head_h * screen()->getToolbarWidthPercent() / 100;
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

    switch (where) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y +
            screen()->getBevelWidth() - screen()->getBorderWidth() - frame.height;
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + head_h - screen()->getBevelWidth() - 
            screen()->getBorderWidth();
        break;

    case TOPCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y +
            screen()->getBevelWidth() - screen()->getBorderWidth() - frame.height;
        break;
    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y +
            screen()->getBevelWidth() - screen()->getBorderWidth() - frame.height;
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - screen()->getBorderWidth2x();
        frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - screen()->getBevelWidth() - 
            screen()->getBorderWidth();
        break;

    case BOTTOMCENTER: // default is BOTTOMCENTER
    default:
        frame.x = head_x + (head_w - frame.width) / 2;
        frame.y = head_y + head_h - frame.height - screen()->getBorderWidth2x();
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - screen()->getBevelWidth() - 
            screen()->getBorderWidth();
        break;
    case LEFTCENTER:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x - frame.width + screen()->getBevelWidth() + screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case LEFTTOP:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = frame.x - frame.width + screen()->getBevelWidth() + screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case LEFTBOTTOM:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height;
        frame.x_hidden = frame.x - frame.width + screen()->getBevelWidth() + screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case RIGHTCENTER:
        frame.x = head_x + head_w - frame.width;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x + frame.width - screen()->getBevelWidth() - screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case RIGHTTOP:
        frame.x = head_x + head_w - frame.width;
        frame.y = head_y;
        frame.x_hidden = frame.x + frame.width - screen()->getBevelWidth() - screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case RIGHTBOTTOM:
        frame.x = head_x + head_w - frame.width;
        frame.y = head_y + head_h - frame.height;
        frame.x_hidden = frame.x + frame.width - screen()->getBevelWidth() - screen()->getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    }

}

void Toolbar::HideHandler::timeout() {
    if (toolbar->isEditing()) { // don't hide if we're editing workspace label
        toolbar->hide_timer.fireOnce(false);
        toolbar->hide_timer.start(); // restart timer and try next timeout
        return;
    }
    toolbar->hide_timer.fireOnce(true);

    toolbar->hidden = ! toolbar->hidden;
    if (toolbar->hidden) {
        toolbar->frame.window.move(toolbar->frame.x_hidden, toolbar->frame.y_hidden);
    } else {
        toolbar->frame.window.move(toolbar->frame.x, toolbar->frame.y);
    }
}


Toolbarmenu::Toolbarmenu(Toolbar &tb) : Basemenu(tb.screen()), m_toolbar(tb),
                                        m_placementmenu(*this) {

    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    setLabel(i18n->getMessage(
        ToolbarSet, ToolbarToolbarTitle,
        "Toolbar"));
    setInternalMenu();

#ifdef XINERAMA
    if (m_toolbar.screen()->hasXinerama()) { // only create if we need it
        m_headmenu.reset(new Headmenu(this));
    }
#endif // XINERAMA

    insert(i18n->getMessage(
        CommonSet, CommonPlacementTitle,
        "Placement"),
           &m_placementmenu);

    if (m_headmenu.get()) { //TODO: NLS
        insert(i18n->getMessage(0, 0, "Place on Head"), m_headmenu.get());
    }

    insert(i18n->getMessage(
        CommonSet, CommonAlwaysOnTop,
        "Always on top"),
           1);
    insert(i18n->getMessage(
        CommonSet, CommonAutoHide,
        "Auto hide"),
           2);
    insert(i18n->getMessage(
        ToolbarSet, ToolbarEditWkspcName,
        "Edit current workspace name"),
           3);

    update();

    if (m_toolbar.isOnTop())
        setItemSelected(1, true);
    if (m_toolbar.doAutoHide())
        setItemSelected(2, true);
}


Toolbarmenu::~Toolbarmenu() {

}


void Toolbarmenu::itemSelected(int button, unsigned int index) {
    if (button == 1) {
        BasemenuItem *item = find(index);
        if (item == 0)
            return;

        switch (item->function()) {
        case 1:  {// always on top
            bool change = ((m_toolbar.isOnTop()) ? false : true);
            m_toolbar.on_top = change;
            screen()->saveToolbarOnTop(m_toolbar.on_top);
            setItemSelected(1, change);
			
            if (m_toolbar.isOnTop())
                m_toolbar.screen()->raiseWindows(Workspace::Stack());
			
            Fluxbox::instance()->save_rc();
            break;
        }

        case 2: { // auto hide
            bool change = ((m_toolbar.doAutoHide()) ? false : true);
            m_toolbar.do_auto_hide = change;
            screen()->saveToolbarAutoHide(m_toolbar.do_auto_hide);
            setItemSelected(2, change);

#ifdef SLIT
            m_toolbar.screen()->getSlit()->reposition();
#endif // SLIT
            Fluxbox::instance()->save_rc();
            break;
        }

        case 3: // edit current workspace name
            m_toolbar.edit(); //set edit mode
            hide();	//dont show menu while editing name

            break;
        }
    }
}


void Toolbarmenu::internal_hide() {
    Basemenu::internal_hide();
    if (m_toolbar.doAutoHide() && ! m_toolbar.isEditing())
        m_toolbar.hide_handler.timeout();
}


void Toolbarmenu::reconfigure() {
    m_placementmenu.reconfigure();

    if (m_headmenu.get()) {
        m_headmenu->reconfigure();
    }

    Basemenu::reconfigure();
}

//TODO: fix placement
Toolbarmenu::Placementmenu::Placementmenu(Toolbarmenu &tm)
    : Basemenu(tm.m_toolbar.screen()), m_toolbarmenu(tm) {

    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    setLabel(i18n->getMessage(
        ToolbarSet, ToolbarToolbarPlacement,
        "Toolbar Placement"));
    setInternalMenu();
    setMinimumSublevels(5);

    insert("Left Top", Toolbar::LEFTTOP);
    insert("Left Center", Toolbar::LEFTCENTER);
    insert("Left Bottom", Toolbar::LEFTBOTTOM);
    
    insert(i18n->getMessage(
        CommonSet, CommonPlacementTopLeft,
        "Top Left"),
           Toolbar::TOPLEFT);
    insert("");

    insert(i18n->getMessage(
        CommonSet, CommonPlacementBottomLeft,
        "Bottom Left"),
           Toolbar::BOTTOMLEFT);
    insert(i18n->getMessage(
        CommonSet, CommonPlacementTopCenter,
        "Top Center"),
           Toolbar::TOPCENTER);
    insert("");
    insert(i18n->getMessage(
        CommonSet, CommonPlacementBottomCenter,
        "Bottom Center"),
           Toolbar::BOTTOMCENTER);
    insert(i18n->getMessage(
        CommonSet, CommonPlacementTopRight,
        "Top Right"),
           Toolbar::TOPRIGHT);
    insert("");
    insert(i18n->getMessage(
        CommonSet, CommonPlacementBottomRight,
        "Bottom Right"),
           Toolbar::BOTTOMRIGHT);

    insert("Right Top", Toolbar::RIGHTTOP);
    insert("Right Center", Toolbar::RIGHTCENTER);
    insert("Right Bottom", Toolbar::RIGHTBOTTOM);
    update();
}


void Toolbarmenu::Placementmenu::itemSelected(int button, unsigned int index) {
    if (button == 1) {
        BasemenuItem *item = find(index);
        if (! item)
            return;

        m_toolbarmenu.m_toolbar.screen()->saveToolbarPlacement(
            static_cast<Toolbar::Placement>(item->function()));
        m_toolbarmenu.m_toolbar.setPlacement(static_cast<Toolbar::Placement>(item->function()));
        hide();
        m_toolbarmenu.m_toolbar.reconfigure();

#ifdef SLIT
        // reposition the slit as well to make sure it doesn't intersect the
        // toolbar
        m_toolbarmenu.m_toolbar.screen()->getSlit()->reposition();
#endif // SLIT
        Fluxbox::instance()->save_rc();
    }
}

#ifdef XINERAMA

Toolbarmenu::Headmenu::Headmenu(Toolbarmenu &tm)
    : Basemenu(tm.m_toolbar.screen()), m_toolbarmenu(tm) {

    I18n *i18n = I18n::instance();

    setLabel(i18n->getMessage(0, 0, "Place on Head")); //TODO: NLS
    setInternalMenu();

    int numHeads = toolbarmenu->toolbar->screen->getNumHeads();
    // fill menu with head entries
    for (int i = 0; i < numHeads; i++) {
        char headName[32];
        sprintf(headName, "Head %i", i+1); //TODO: NLS
        insert(i18n->getMessage(0, 0, headName), i);
    }

    insert(i18n->getMessage(0, 0, "All Heads"), -1); //TODO: NLS

    update();
}


void Toolbarmenu::Headmenu::itemSelected(int button, unsigned int index) {
    if (button == 1) {
        BasemenuItem *item = find(index);
        if (! item)
            return;

        screen()->saveToolbarOnHead(
            static_cast<int>(item->function()));
        
        hide();
        m_toolbarmenu.m_toolbar.reconfigure();


#ifdef SLIT
        // reposition the slit as well to make sure it doesn't intersect the
        // toolbar
        m_toolbarmenu.m_toolbar.screen()->getSlit()->reposition();
#endif // SLIT
        Fluxbox::instance()->save_rc();
    }
}

#endif // XINERAMA
