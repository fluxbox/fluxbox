// SystemTray.hh
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

// $Id: SystemTray.hh,v 1.4 2004/04/19 22:48:19 fluxgen Exp $

#ifndef SYSTEMTRAY_HH
#define SYSTEMTRAY_HH


#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"

#include "ToolbarItem.hh"

#include <X11/Xlib.h>

#include <list>

class AtomHandler;

class SystemTray: public ToolbarItem, public FbTk::EventHandler {
public:

    explicit SystemTray(const FbTk::FbWindow &parent);
    virtual ~SystemTray();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void show();
    void hide();

    bool active() { return !m_clients.empty(); }

    bool clientMessage(const XClientMessageEvent &event);
    void exposeEvent(XExposeEvent &event);
    void handleEvent(XEvent &event);

    void addClient(Window win);
    void removeClient(Window win);

    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    int numClients() const { return m_clients.size(); }
    const FbTk::FbWindow &window() const { return m_window; }

private:
    typedef std::list<FbTk::FbWindow *> ClientList;
    ClientList::iterator findClient(Window win);

    void renderTheme();
    void rearrangeClients();
    void removeAllClients();

    FbTk::FbWindow m_window;

    std::auto_ptr<AtomHandler> m_handler;

    ClientList m_clients;
};

#endif // SYSTEMTRAY_HH
