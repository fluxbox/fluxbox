// FbWindow.cc for FbTk - fluxbox toolkit
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbWindow.cc,v 1.30 2004/01/08 22:04:39 fluxgen Exp $

#include "FbWindow.hh"

#include "EventManager.hh"
#include "Color.hh"
#include "App.hh"
#include "Transparent.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xatom.h>

#include <cassert>

namespace FbTk {

namespace {
Pixmap getRootPixmap(int screen_num) {
    Pixmap root_pm = 0;
    // get root pixmap for transparency
    Display *disp = FbTk::App::instance()->display();
    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    unsigned int *data;
    if (XGetWindowProperty(disp, RootWindow(disp, screen_num), 
                           XInternAtom(disp, "_XROOTPMAP_ID", false),
                           0L, 1L, 
                           false, XA_PIXMAP, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) { 
        root_pm = (Pixmap) (*data);                  
        XFree(data);
    }

    return root_pm; 
}

}; // end anonymous namespace

Display *FbWindow::s_display = 0;

FbWindow::FbWindow():m_parent(0), m_screen_num(0), m_window(0), m_x(0), m_y(0), 
                     m_width(0), m_height(0), m_border_width(0), m_depth(0), m_destroy(true),
                     m_buffer_pm(0) {

    if (s_display == 0)
        s_display = App::instance()->display();
}

FbWindow::FbWindow(const FbWindow& the_copy):m_parent(the_copy.parent()), 
                                             m_screen_num(the_copy.screenNumber()), m_window(the_copy.window()), 
                                             m_x(the_copy.x()), m_y(the_copy.y()), 
                                             m_width(the_copy.width()), m_height(the_copy.height()),
                                             m_border_width(the_copy.borderWidth()), 
                                             m_depth(the_copy.depth()), m_destroy(true),
                                             m_buffer_pm(0) {
    if (s_display == 0)
        s_display = App::instance()->display();

    the_copy.m_window = 0;
}

FbWindow::FbWindow(int screen_num,
                   int x, int y, 
                   unsigned int width, unsigned int height, 
                   long eventmask, 
                   bool override_redirect,
                   int depth,
                   int class_type):
    m_parent(0),
    m_screen_num(screen_num),
    m_destroy(true),
    m_buffer_pm(0) {
	
    create(RootWindow(FbTk::App::instance()->display(), screen_num), 
           x, y, width, height, eventmask,
           override_redirect, depth, class_type);
};

FbWindow::FbWindow(const FbWindow &parent,
                   int x, int y, unsigned int width, unsigned int height, 
                   long eventmask,
                   bool override_redirect, 
                   int depth, int class_type):
    m_parent(&parent),
    m_screen_num(parent.screenNumber()), 
    m_destroy(true),
    m_buffer_pm(0) { 

    create(parent.window(), x, y, width, height, eventmask, 
           override_redirect, depth, class_type);
	
	
};

FbWindow::FbWindow(Window client):m_parent(0), 
                                  m_screen_num(0),
                                  m_window(0),
                                  m_x(0), m_y(0),
                                  m_width(1), m_height(1),
                                  m_border_width(0),
                                  m_depth(0),
                                  m_destroy(false),  // don't destroy this window
                                  m_buffer_pm(0) {

    if (s_display == 0)
        s_display = App::instance()->display();

    setNew(client);
}

FbWindow::~FbWindow() {

    if (m_window != 0) {
        // so we don't get any dangling eventhandler for this window
        FbTk::EventManager::instance()->remove(m_window); 
        if (m_destroy)
            XDestroyWindow(s_display, m_window);     
    }
}


void FbWindow::setBackgroundColor(const FbTk::Color &bg_color) {
    XSetWindowBackground(s_display, m_window, bg_color.pixel());
}

void FbWindow::setBackgroundPixmap(Pixmap bg_pixmap) {
    XSetWindowBackgroundPixmap(s_display, m_window, bg_pixmap);
}

void FbWindow::setBorderColor(const FbTk::Color &border_color) {
    XSetWindowBorder(s_display, m_window, border_color.pixel());
}

void FbWindow::setBorderWidth(unsigned int size) {	
    XSetWindowBorderWidth(s_display, m_window, size);
    m_border_width = size;
}

void FbWindow::setName(const char *name) {
    XStoreName(s_display, m_window, name);
}

void FbWindow::setEventMask(long mask) {
    XSelectInput(s_display, m_window, mask);
}

void FbWindow::clear() {
    XClearWindow(s_display, m_window);
}

void FbWindow::clearArea(int x, int y, 
                         unsigned int width, unsigned int height, 
                         bool exposures) {
    XClearArea(s_display, window(), x, y, width, height, exposures);
}

void FbWindow::updateTransparent(int the_x, int the_y, unsigned int the_width, unsigned int the_height) {
#ifdef HAVE_XRENDER
    if (width() == 0 || height() == 0)
        return;

    if (the_width == 0 || the_height == 0) {
        the_width = width();
        the_height = height();
    }

    if (the_x < 0 || the_y < 0) {
        the_x = 0;
        the_y = 0;
    }

    if (!m_transparent.get())
        return;

    // update source and destination if needed
    Pixmap root = getRootPixmap(screenNumber());
    if (m_transparent->source() != root)
        m_transparent->setSource(root, screenNumber());

    if (m_buffer_pm) {
        if (m_transparent->dest() != m_buffer_pm) {
            m_transparent->setDest(m_buffer_pm, screenNumber());
        }
    } else if (m_transparent->dest() != window())
        m_transparent->setDest(window(), screenNumber());


    // get root position

    const FbWindow *root_parent = parent();
    // our position in parent ("root")
    int root_x = x() + borderWidth(), root_y = y() + borderWidth();
    if (root_parent != 0) {
        root_x += root_parent->x() + root_parent->borderWidth();
        root_y += root_parent->y() + root_parent->borderWidth();
        while (root_parent->parent() != 0) {
            root_parent = root_parent->parent();
            root_x += root_parent->x() + root_parent->borderWidth();
            root_y += root_parent->y() + root_parent->borderWidth();
        }

    } // else toplevel window so we already have x, y set

    // render background image from root pos to our window
    m_transparent->render(root_x + the_x, root_y + the_y,
                          the_x, the_y,
                          the_width, the_height);
#endif // HAVE_XRENDER
}

void FbWindow::setAlpha(unsigned char alpha) {
#ifdef HAVE_XRENDER
    if (m_transparent.get() == 0 && alpha != 0) {
        m_transparent.reset(new Transparent(getRootPixmap(screenNumber()), window(), alpha, screenNumber()));
    } else if (alpha != 0 && alpha != m_transparent->alpha())
        m_transparent->setAlpha(alpha);
    else if (alpha == 0)
        m_transparent.reset(0); // destroy transparent object
#endif // HAVE_XRENDER
}


FbWindow &FbWindow::operator = (const FbWindow &win) {
    m_parent = win.parent();
    m_screen_num = win.screenNumber();
    m_window = win.window();
    m_x = win.x();
    m_y = win.y();
    m_width = win.width();
    m_height = win.height();
    m_border_width = win.borderWidth();
    m_depth = win.depth();
    // take over this window
    win.m_window = 0;
    return *this;
}

FbWindow &FbWindow::operator = (Window win) {
    setNew(win);	
    return *this;
}

void FbWindow::setNew(Window win) {
    if (s_display == 0)
        s_display = App::instance()->display();

    if (m_window != 0 && m_destroy)
        XDestroyWindow(s_display, m_window);

    m_window = win;

    if (m_window != 0) {
        updateGeometry();
        XWindowAttributes attr;
        attr.screen = 0;
        //get screen number
        if (XGetWindowAttributes(s_display,
                                 m_window,
                                 &attr) != 0 && attr.screen != 0) {
            m_screen_num = XScreenNumberOfScreen(attr.screen);
            m_width = attr.width;
            m_height = attr.height ;
            m_x = attr.x;
            m_y = attr.y;
            m_depth = attr.depth;
            m_border_width = attr.border_width;
        }
        
    }
}

void FbWindow::show() {
    XMapWindow(s_display, m_window);
}

void FbWindow::showSubwindows() {
    XMapSubwindows(s_display, m_window);
}

void FbWindow::hide() {
    XUnmapWindow(s_display, m_window);
}

void FbWindow::lower() {
    XLowerWindow(s_display, window());
}

void FbWindow::raise() {
    XRaiseWindow(s_display, window());
}

void FbWindow::setInputFocus(int revert_to, int time) {
    XSetInputFocus(s_display, window(), revert_to, time);
}

void FbWindow::setCursor(Cursor cur) {
    XDefineCursor(s_display, window(), cur); 
}

void FbWindow::unsetCursor() {
    XUndefineCursor(s_display, window());
}

void FbWindow::reparent(const FbWindow &parent, int x, int y) {
    XReparentWindow(s_display, window(), parent.window(), x, y);
    m_parent = &parent;
    updateGeometry();
}

bool FbWindow::property(Atom property,
                        long long_offset, long long_length,
                        bool do_delete,
                        Atom req_type,
                        Atom *actual_type_return,
                        int *actual_format_return,
                        unsigned long *nitems_return,
                        unsigned long *bytes_after_return,
                        unsigned char **prop_return) const {
    if (XGetWindowProperty(s_display, window(), 
                           property, long_offset, long_length, do_delete, 
                           req_type, actual_type_return,
                           actual_format_return, nitems_return,
                           bytes_after_return, prop_return) == Success)
        return true;

    return false;
}

void FbWindow::changeProperty(Atom property, Atom type,
                              int format,
                              int mode,
                              unsigned char *data,
                              int nelements) {
    
    XChangeProperty(s_display, m_window, property, type,
                    format, mode, 
                    data, nelements);
}

int FbWindow::screenNumber() const {
    return m_screen_num;
}

long FbWindow::eventMask() const {
    XWindowAttributes attrib;
    if (XGetWindowAttributes(s_display, window(), 
                         &attrib) == Success) {
        return attrib.your_event_mask;
    }
    return 0;
}

void FbWindow::setBufferPixmap(Pixmap pm) {
    m_buffer_pm = pm;
}

void FbWindow::updateGeometry() {
    if (m_window == 0)
        return;

    Window root;
    unsigned int border_width, depth;
    XGetGeometry(s_display, m_window, &root, &m_x, &m_y,
                 (unsigned int *)&m_width, (unsigned int *)&m_height, 
                 &border_width, &depth);
    m_depth = depth;
}

void FbWindow::create(Window parent, int x, int y,
                      unsigned int width, unsigned int height, 
                      long eventmask, bool override_redirect,
                      int depth, int class_type) {
                     

    if (s_display == 0)
        s_display = FbTk::App::instance()->display();

    m_border_width = 0;

    long valmask = CWEventMask;
    XSetWindowAttributes values;
    values.event_mask = eventmask;

    if (override_redirect) {        
        valmask |= CWOverrideRedirect;
        values.override_redirect = True;
    }

    m_window = XCreateWindow(s_display, parent, x, y, width, height,
                             0, // border width  
                             depth, // depth
                             class_type, // class
                             CopyFromParent, // visual
                             valmask, // create mask
                             &values); // create atrribs
    
    assert(m_window);

    updateGeometry();
    FbWindow::setBackgroundColor(Color("gray", screenNumber()));
}

bool operator == (Window win, const FbWindow &fbwin) {
    return win == fbwin.window();
}

};
