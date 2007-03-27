// WinClient.hh for Fluxbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef WINCLIENT_HH
#define WINCLIENT_HH

#include "Focusable.hh"
#include "Window.hh"
#include "Subject.hh"
#include "FbWindow.hh"
#include "FbTk/FbString.hh"

#include <X11/Xutil.h>

class BScreen;
class Strut;

/// Holds client window info 
class WinClient: public Focusable, public FbTk::FbWindow {
public:
    typedef std::list<WinClient *> TransientList;
    // this structure only contains 3 elements... the Motif 2.0 structure contains
    // 5... we only need the first 3... so that is all we will define
    typedef struct MwmHints {
        unsigned long flags;       // Motif wm flags
        unsigned long functions;   // Motif wm functions
        unsigned long decorations; // Motif wm decorations
    } MwmHints;

    WinClient(Window win, BScreen &screen, FluxboxWindow *fbwin = 0);

    ~WinClient();

    bool sendFocus(); // returns whether we sent a message or not 
                      // i.e. whether we assume the focus will get taken
    bool acceptsFocus() const; // will this window accept focus (according to hints)
    void sendClose(bool forceful = false);
    // not aware of anything that makes this false at present
    inline bool isClosable() const { return true; }

    void addModal(); // some transient of ours (or us) is modal
    void removeModal(); // some transient (or us) is no longer modal

    /// updates from wm class hints
    void updateWMClassHint();
    void updateWMProtocols();

    // override the title with this
    void setTitle(FbTk::FbString &title);
    void setIconTitle(FbTk::FbString &icon_title);
    void updateTitle();
    void updateIconTitle();
    /// updates transient window information
    void updateTransientInfo();

    void updateBlackboxHints();
    void updateMWMHints();
    void updateWMHints();
    void updateWMNormalHints();

    void setStrut(Strut *strut);
    void clearStrut();

    bool focus(); // calls Window->setCurrentClient to give focus to this client
    const std::string &title() const;

    /**
     * Changes width and height to the nearest (lower) value
     * that conforms to it's size hints.
     *
     * display_* give the values that would be displayed
     * to the user when resizing.
     * We use pointers for display_* since they are optional.
     */
    void applySizeHints(int &width, int &height, int *display_width = 0,
            int *display_height = 0, bool maximizing = false);


    void setGroupLeftWindow(Window win);

    void saveBlackboxAttribs(FluxboxWindow::BlackboxAttributes &blackbox_attribs);
    void setFluxboxWindow(FluxboxWindow *win);

    // does this client have a pending unmap or destroy event?
    bool validateClient() const;

    //
    // accessors 
    //

    bool getAttrib(XWindowAttributes &attr) const;
    bool getWMName(XTextProperty &textprop) const;
    bool getWMIconName(XTextProperty &textprop) const;
    /// @return name member of class structure
    const std::string &getWMClassName() const; 
    /// @return class member of class structure
    const std::string &getWMClassClass() const;

    /// notifies when this client dies
    FbTk::Subject &dieSig() { return m_diesig; }
    /// notifies when this client becomes focused
    FbTk::Subject &focusSig() { return m_focussig; }

    inline WinClient *transientFor() { return transient_for; }
    inline const WinClient *transientFor() const { return transient_for; }
    inline TransientList &transientList() { return transients; }
    inline const TransientList &transientList() const { return transients; }
    inline bool isTransient() const { return transient_for != 0; }

    inline bool isModal() const { return m_modal > 0; }

    inline int gravity() const { return m_win_gravity; }

    bool hasGroupLeftWindow() const;
    // grouping is tracked by remembering the window to the left in the group
    Window getGroupLeftWindow() const;

    inline int getFocusMode() const { return m_focus_mode; }
    inline const FluxboxWindow::BlackboxHints *getBlackboxHint() const { return m_blackbox_hint; }
    inline const MwmHints *getMwmHint() const { return m_mwm_hint; }

    inline unsigned int maxWidth() const { return max_width; }
    inline unsigned int maxHeight() const { return max_height; }

    static const int PropBlackboxHintsElements = 5;
    static const int PropMwmHintsElements = 3;

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
        base_width, base_height;
    unsigned long initial_state, normal_hint_flags, wm_hint_flags;



    class WinClientSubj: public FbTk::Subject {
    public:
        explicit WinClientSubj(WinClient &client):m_winclient(client) { }
        WinClient &winClient() { return m_winclient; }
    private:
        WinClient &m_winclient;
    };

    enum FocusMode { F_NOINPUT = 0, F_PASSIVE, F_LOCALLYACTIVE, F_GLOBALLYACTIVE };

private:
    /// removes client from any waiting list and clears empty waiting lists
    void removeTransientFromWaitingList();

    // number of transients which we are modal for
    // or indicates that we are modal if don't have any transients
    int m_modal;
    bool send_focus_message, send_close_message;

    int m_win_gravity;

    std::string m_class_name, m_instance_name, m_icon_title;
    bool m_title_override, m_icon_title_override;

    FluxboxWindow::BlackboxHints *m_blackbox_hint;
    MwmHints *m_mwm_hint;

    int m_focus_mode;

    WinClientSubj m_diesig;
    WinClientSubj m_focussig;

    Strut *m_strut;
    // map transient_for X window to winclient transient 
    // (used if transient_for FbWindow was created after transient)    
    // Since a lot of transients can be created before transient_for 
    // we need to map transient_for window to a list of transients
    //
    // Stuff to worry about:
    // 1) If transients die before the transient_for is created
    // 2) If transients changes to a new transient_for before old transient_for is created
    // ( 3) Transient_for is never created 
    //      This is not a big deal since the key value will be cleared 
    //      once the list is empty )
    typedef std::map<Window, TransientList> TransientWaitMap;
    static TransientWaitMap s_transient_wait;

};

#endif // WINCLIENT_HH
