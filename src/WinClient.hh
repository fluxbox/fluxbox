// WinClient.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: WinClient.hh,v 1.12 2003/07/28 15:46:00 rathnor Exp $

#ifndef WINCLIENT_HH
#define WINCLIENT_HH

#include "Window.hh"
#include "Subject.hh"
#include "FbWindow.hh"

#include <X11/Xutil.h>
#include <string>

class BScreen;
class Strut;

/// Holds client window info 
class WinClient:public FbTk::FbWindow {
public:
    typedef std::list<WinClient *> TransientList;

    WinClient(Window win, BScreen &screen, FluxboxWindow *fbwin = 0);

    ~WinClient();
    void updateRect(int x, int y, unsigned int width, unsigned int height);
    bool sendFocus(); // returns whether we sent a message or not 
                      // i.e. whether we assume the focus will get taken
    void sendClose(bool forceful = false);
    inline bool isClosable() const { return closable; }
    void reparent(Window win, int x, int y);
    bool getAttrib(XWindowAttributes &attr) const;
    bool getWMName(XTextProperty &textprop) const;
    bool getWMIconName(XTextProperty &textprop) const;
    /// @return name member of class structure
    const std::string &getWMClassName() const; 
    /// @return class member of class structure
    const std::string &getWMClassClass() const;
    /// updates from wm class hints
    void updateWMClassHint();
    void updateWMProtocols();

    inline const std::string &getTitle() const { return m_title; }
    void updateTitle();
    void updateIconTitle();
    BScreen &screen() { return m_screen; }
    const BScreen &screen() const { return m_screen; }
    /// notifies when this client dies
    FbTk::Subject &dieSig() { return m_diesig; }

    /// updates transient window information
    void updateTransientInfo();
    WinClient *transientFor() { return transient_for; }
    const WinClient *transientFor() const { return transient_for; }
    TransientList &transientList() { return transients; }
    const TransientList &transientList() const { return transients; }
    bool isTransient() const { return transient_for != 0; }

    bool isModal() const { return m_modal > 0; }
    void addModal(); // some transient of ours (or us) is modal
    void removeModal(); // some transient (or us) is no longer modal

    bool operator == (const FluxboxWindow &win) const {
        return (m_win == &win);
    }

    void setStrut(Strut *strut);
    void clearStrut();

    bool focus(); // calls Window->setCurrentClient to give focus to this client

    const std::string &title() const { return m_title; }
    const std::string &iconTitle() const { return m_icon_title; }
    const FluxboxWindow *fbwindow() const { return m_win; }
    FluxboxWindow *fbwindow() { return m_win; }

    static const int PropBlackboxHintsElements = 5;
    static const int PropMwmHintsElements = 3;

    void updateBlackboxHints();
    void updateMWMHints();
    void updateWMHints();
    void updateWMNormalHints();

    // grouping is tracked by remembering the window to the left in the group
    Window getGroupLeftWindow() const;
    void setGroupLeftWindow(Window win);
    bool hasGroupLeftWindow() const;

    // does this client have a pending unmap or destroy event?
    bool validateClient() const;

    /**
       !! TODO !!
       remove or move these to private
     */

    WinClient *transient_for; // which window are we a transient for?
    std::list<WinClient *> transients;  // which windows are our transients?
    Window window_group;

 
    int x, y, old_bw;
    unsigned int
        min_width, min_height, max_width, max_height, width_inc, height_inc,
        min_aspect_x, min_aspect_y, max_aspect_x, max_aspect_y,
        base_width, base_height, win_gravity;
    unsigned long initial_state, normal_hint_flags, wm_hint_flags;

    // this structure only contains 3 elements... the Motif 2.0 structure contains
    // 5... we only need the first 3... so that is all we will define
    typedef struct MwmHints {
        unsigned long flags;       // Motif wm flags
        unsigned long functions;   // Motif wm functions
        unsigned long decorations; // Motif wm decorations
    } MwmHints;

    FluxboxWindow *m_win;
    class WinClientSubj: public FbTk::Subject {
    public:
        explicit WinClientSubj(WinClient &client):m_winclient(client) { }
        WinClient &winClient() { return m_winclient; }
    private:
        WinClient &m_winclient;
    };

    inline int getFocusMode() const { return m_focus_mode; }
    inline const FluxboxWindow::BlackboxHints *getBlackboxHint() const {
        return m_blackbox_hint; }
    void saveBlackboxAttribs(FluxboxWindow::BlackboxAttributes &blackbox_attribs);
    inline const MwmHints *getMwmHint() const { return m_mwm_hint; }

    enum { F_NOINPUT = 0, F_PASSIVE, F_LOCALLYACTIVE, F_GLOBALLYACTIVE };

private:
    // number of transients which we are modal for
    // or indicates that we are modal if don't have any transients
    int m_modal;
    bool send_focus_message, closable;

    std::string m_title, m_icon_title;
    std::string m_class_name, m_instance_name;

    FluxboxWindow::BlackboxHints *m_blackbox_hint;
    MwmHints *m_mwm_hint;

    int m_focus_mode;

    WinClientSubj m_diesig;
    BScreen &m_screen;

    Strut *m_strut;

};

#endif // WINCLIENT_HH
