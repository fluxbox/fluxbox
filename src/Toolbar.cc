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

// $Id: Toolbar.cc,v 1.70 2003/04/15 12:15:44 fluxgen Exp $

#include "Toolbar.hh"

#include "i18n.hh"
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

namespace {
class SetToolbarPlacementCmd: public FbTk::Command {
public:
    explicit SetToolbarPlacementCmd(Toolbar &tbar, Toolbar::Placement place):m_tbar(tbar), m_place(place) { }
    void execute() {
        m_tbar.setPlacement(m_place);
        m_tbar.reconfigure();
        m_tbar.screen().saveToolbarPlacement(m_place);
        Fluxbox::instance()->save_rc();
    }
private:
    Toolbar &m_tbar;
    Toolbar::Placement m_place;
};


void setupMenus(Toolbar &tbar) {
    I18n *i18n = I18n::instance();
    using namespace FBNLS;
    using namespace FbTk;

    FbTk::Menu &menu = tbar.menu();
    
    RefCount<Command> start_edit(new SimpleCommand<Toolbar>(tbar, &Toolbar::edit));
    menu.insert(i18n->getMessage(
                                 FBNLS::ToolbarSet, FBNLS::ToolbarEditWkspcName,
                                 "Edit current workspace name"),
                start_edit);

    menu.setLabel(i18n->getMessage(
                                   FBNLS::ToolbarSet, FBNLS::ToolbarToolbarTitle,
                                   "Toolbar"));
    menu.setInternalMenu();

    menu.insert("Layer...", &tbar.layermenu());

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
        {0, 0, "Top Left", Toolbar::TOPLEFT},
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
    minute(-1)

{
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // add windows to eventmanager
    evm.add(evh, window);
    evm.add(evh, workspace_label);
    evm.add(evh, window_label);
    evm.add(evh, clock);
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
    editing(false),
    hidden(scrn.doToolbarAutoHide()), 
    do_auto_hide(scrn.doToolbarAutoHide()),
    frame(*this, scrn.getScreenNumber()),
    m_screen(scrn),
    image_ctrl(*scrn.getImageControl()),
    clock_timer(this), 	// get the clock updating every minute
    hide_timer(&hide_handler),
    m_toolbarmenu(menu),
    m_placementmenu(*scrn.menuTheme(),
                    scrn.getScreenNumber(), *scrn.getImageControl()),
    m_layermenu(*scrn.menuTheme(), 
                scrn.getScreenNumber(), 
                *scrn.getImageControl(),
                *scrn.layerManager().getLayer(Fluxbox::instance()->getMenuLayer()), 
                this),
    m_theme(scrn.getScreenNumber()),
    m_place(BOTTOMCENTER),
    m_themelistener(*this),
    m_layeritem(frame.window, layer) {

       
    // we need to get notified when the theme is reloaded
    m_theme.addListener(m_themelistener);

    setupMenus(*this);

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

    //DEL/fix -> remove useIconBar resource
//    if (Fluxbox::instance()->useIconBar())
    m_iconbar.reset(new IconBar(screen(), frame.window_label.window(), m_theme.font()));


    XMapSubwindows(display, frame.window.window());
    frame.window.show();

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


    reconfigure();
	
}


Toolbar::~Toolbar() {

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

void Toolbar::delAllIcons() {
    if (m_iconbar.get() == 0)
        return;

    IconBar::WindowList *deleted = m_iconbar->delAllIcons();
    IconBar::WindowList::iterator it = deleted->begin();
    IconBar::WindowList::iterator it_end = deleted->end();
    for (; it != it_end; ++it) {
        FbTk::EventManager::instance()->remove(*it);
    }
    delete deleted;
}
    
bool Toolbar::containsIcon(FluxboxWindow &win) {
    return (m_iconbar->findIcon(&win) != 0);
}

void Toolbar::enableIconBar() {
    // already on
    if (m_iconbar.get() != 0) 
        return;
    m_iconbar.reset(new IconBar(screen(), frame.window_label.window(), m_theme.font()));
}

void Toolbar::disableIconBar() {
    // already off
    if (m_iconbar.get() == 0) 
        return;
    
    delAllIcons();

    m_iconbar.reset(0); // destroy iconbar

}


void Toolbar::reconfigure() {

    if (do_auto_hide == false && 
        do_auto_hide != screen().doToolbarAutoHide()) {
        hide_timer.start();
    }

    do_auto_hide = screen().doToolbarAutoHide();

    bool vertical = isVertical();

    if (m_iconbar.get())
        m_iconbar->setVertical(vertical);

    frame.bevel_w = screen().getBevelWidth();

    // recallibrate size
    setPlacement(m_place);

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
                                             i18n->getMessage(
                                                              FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeLength,
                                                              "00:00000"),
                                             strlen(i18n->getMessage(
                                                                     FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeLength,
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

    frame.window.setBorderColor(*screen().getBorderColor());
    frame.window.setBorderWidth(screen().getBorderWidth());

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
    checkClock(true);

    m_toolbarmenu.reconfigure();

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
                    i18n->getMessage(
                                     FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeDateFormatEu,
                                     "%02d.%02d.%02d"),
                    tt->tm_mday, tt->tm_mon + 1,
                    (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
        } else {
            sprintf(t,
                    i18n->getMessage(
                                     FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeDateFormat,
                                     "%02d/%02d/%02d"),
                    tt->tm_mon + 1, tt->tm_mday,
                    (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
        }
    } else {
        if (screen().isClock24Hour()) {
            sprintf(t,
                    i18n->getMessage(
                                     FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormat24,
                                     "	%02d:%02d "),
                    frame.hour, frame.minute);
        } else {
            sprintf(t,
                    i18n->getMessage(
                                     FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormat12,
                                     "%02d:%02d %sm"),
                    ((frame.hour > 12) ? frame.hour - 12 :
                     ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
                    ((frame.hour >= 12) ?
                     i18n->getMessage(
                                      FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormatP,
                                      "p") :
                     i18n->getMessage(
                                      FBNLS::ToolbarSet, FBNLS::ToolbarNoStrftimeTimeFormatA,
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
    m_theme.font().drawText(
                            frame.clock.window(),
                            screen().getScreenNumber(),
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
        if (&foc->getScreen() != &screen() || foc->getTitle().size() == 0)
            return;
		
        unsigned int newlen = foc->getTitle().size();
        int dx = FbTk::doAlignment(frame.window_label_w, frame.bevel_w*2,
                                   m_theme.justify(),
                                   m_theme.font(),
                                   foc->getTitle().c_str(), 
                                   foc->getTitle().size(), newlen);
	int dy = 1 + m_theme.font().ascent();

        if (m_theme.font().isRotated()) {
            int tmp = dy;
            dy = frame.window_label.height() - dx;
            dx = tmp;
        }
    
        m_theme.font().drawText(
                                frame.window_label.window(),
                                screen().getScreenNumber(),
                                m_theme.windowTextGC(),
                                foc->getTitle().c_str(), newlen,
                                dx, dy);
    } else
        frame.window_label.clear();
}
 
 
void Toolbar::redrawWorkspaceLabel(bool redraw) {
    if (screen().getCurrentWorkspace()->name().size()==0)
        return;
		
    if (redraw)
        frame.workspace_label.clear();
		
    const char *text = screen().getCurrentWorkspace()->name().c_str();
    size_t textlen = screen().getCurrentWorkspace()->name().size();
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
        screen().getScreenNumber(),
        m_theme.labelTextGC(),
        text, newlen,
        dx, dy);
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
                   ((screen().isSloppyFocus() || screen().isSemiSloppyFocus()) ?
                    RevertToPointerRoot : RevertToParent), CurrentTime);

    frame.workspace_label.clear();
    Fluxbox * const fluxbox = Fluxbox::instance();
    fluxbox->setNoFocus(true);
    if (fluxbox->getFocusedWindow())	//disable focus on current focused window
        fluxbox->getFocusedWindow()->setFocusFlag(false);

    XDrawRectangle(display, frame.workspace_label.window(),
                   screen().getWindowStyle()->l_text_focus_gc,
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
            const FbTk::Menu &wm = fluxboxwin->getWindowmenu();

            int menu_y = be.y_root - wm.height();
            int menu_x = be.x_root;
            // make sure the menu is visible
            if (menu_y < 0) {
                menu_y = 0;
            }
            if (menu_x < 0) {
                menu_x = 0;
            } else if (menu_x + wm.width() > screen().getWidth()) {
                menu_x = screen().getWidth() - wm.width();
            }
            fluxboxwin->showMenu(menu_x, menu_y);

        } else if (! m_toolbarmenu.isVisible()) {
            int x, y;

            x = be.x_root - (m_toolbarmenu.width() / 2);
            y = be.y_root - (m_toolbarmenu.height() / 2);

            if (x < 0)
                x = 0;
            else if (x + m_toolbarmenu.width() > screen().getWidth())
                x = screen().getWidth() - m_toolbarmenu.width();

            if (y < 0)
                y = 0;
            else if (y + m_toolbarmenu.height() > screen().getHeight())
                y = screen().getHeight() - m_toolbarmenu.height();

            m_toolbarmenu.move(x, y);
            m_toolbarmenu.show();
        } else
            m_toolbarmenu.hide();
			
    } 
	
}


void Toolbar::buttonReleaseEvent(XButtonEvent &re) {
    if (re.button == 1) {
        if (re.window == frame.workspace_label) {
            FbTk::Menu *menu = screen().getWorkspacemenu();
            //move the workspace label and make it visible
            menu->move(re.x_root, re.y_root);
            // make sure the entire menu is visible (TODO: this is repeated by other menus, make a function!)
            int newx = menu->x(); // new x position of menu
            int newy = menu->y(); // new y position of menu
            if (menu->x() < 0)
                newx = 0;
            else if (menu->x() + menu->width() > screen().getWidth())
                newx = screen().getWidth() - menu->width();
			
            if (menu->y() < 0)
                newy = 0;
            else if (menu->y() + menu->height() > screen().getHeight())
                newy = screen().getHeight() - menu->height();
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
                screen().getCurrentWorkspace()->setName(new_workspace_name.c_str());

            new_workspace_name.erase(); //erase temporary workspace name
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
            int dy = 1 + m_theme.font().ascent();
            if (m_theme.font().isRotated()) {
                int tmp = dy;
                dy = frame.workspace_label_w - x;
                x = tmp;
            }

            m_theme.font().drawText(
                frame.workspace_label.window(),
                screen().getScreenNumber(),
                screen().getWindowStyle()->l_text_focus_gc,
                new_workspace_name.c_str(), l,
                x, dy);

            XDrawRectangle(display, frame.workspace_label.window(),
                           screen().getWindowStyle()->l_text_focus_gc, x + tw, 0, 1,
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

    head_w = screen().getWidth();
    head_h = screen().getHeight();

    frame.width = head_w * screen().getToolbarWidthPercent() / 100;
    frame.height = m_theme.font().height();

    frame.height += 2;
    frame.height += (frame.bevel_w * 2);


    // should we flipp sizes?
    if (isVertical()) {
        frame.width = frame.height;
        frame.height = head_h * screen().getToolbarWidthPercent() / 100;
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
            screen().getBevelWidth() - screen().getBorderWidth() - frame.height;
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - screen().getBorderWidth2x();
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + head_h - screen().getBevelWidth() - 
            screen().getBorderWidth();
        break;

    case TOPCENTER:
        frame.x = head_x + ((head_w - frame.width) / 2);
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y +
            screen().getBevelWidth() - screen().getBorderWidth() - frame.height;
        break;
    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - screen().getBorderWidth2x();
        frame.y = head_y;
        frame.x_hidden = frame.x;
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - screen().getBorderWidth2x();
        frame.y = head_y + head_h - frame.height - screen().getBorderWidth2x();
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - screen().getBevelWidth() - 
            screen().getBorderWidth();
        break;

    case BOTTOMCENTER: // default is BOTTOMCENTER
    default:
        frame.x = head_x + (head_w - frame.width) / 2;
        frame.y = head_y + head_h - frame.height - screen().getBorderWidth2x();
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - screen().getBevelWidth() - 
            screen().getBorderWidth();
        break;
    case LEFTCENTER:
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x - frame.width + 
            screen().getBevelWidth() + screen().getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case LEFTTOP:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = frame.x - frame.width + 
            screen().getBevelWidth() + screen().getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case LEFTBOTTOM:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height;
        frame.x_hidden = frame.x - frame.width + 
            screen().getBevelWidth() + screen().getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case RIGHTCENTER:
        frame.x = head_x + head_w - frame.width;
        frame.y = head_y + (head_h - frame.height)/2;
        frame.x_hidden = frame.x + frame.width - 
            screen().getBevelWidth() - screen().getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case RIGHTTOP:
        frame.x = head_x + head_w - frame.width;
        frame.y = head_y;
        frame.x_hidden = frame.x + frame.width - 
            screen().getBevelWidth() - screen().getBorderWidth();
        frame.y_hidden = frame.y;
        break;
    case RIGHTBOTTOM:
        frame.x = head_x + head_w - frame.width;
        frame.y = head_y + head_h - frame.height;
        frame.x_hidden = frame.x + frame.width - 
            screen().getBevelWidth() - screen().getBorderWidth();
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
