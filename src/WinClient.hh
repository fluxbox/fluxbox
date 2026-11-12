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

#ifndef WINCLIENT_HH
#define WINCLIENT_HH

#include "Window.hh"
#include "WindowState.hh"

#include "FbTk/FbWindow.hh"
#include "FbTk/FbString.hh"

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
    bool isClosable() const { return true; }

    /// updates from wm class hints
    void updateWMClassHint();
    void updateWMProtocols();

    // override the title with this
    void setTitle(const FbTk::FbString &title);
    void updateTitle();
    /// updates transient window information
    void updateTransientInfo();

    // override the icon with this
    void setIcon(const FbTk::PixmapWithMask& pm);

    // update some thints
    void updateMWMHints();
    void updateWMHints();
    void updateWMNormalHints();

    void setStrut(Strut *strut);
    void clearStrut();

    bool focus(); // calls Window->setCurrentClient to give focus to this client
    bool isFocused() const;
    void setAttentionState(bool value);

    void setGroupLeftWindow(Window win);

    void setFluxboxWindow(FluxboxWindow *win);

    // does this client have a pending unmap or destroy event?
    bool validateClient() const;

    //
    // accessors 
    //

    bool getAttrib(XWindowAttributes &attr) const;
    bool getWMName(XTextProperty &textprop) const;
    bool getWMIconName(XTextProperty &textprop) const;
    std::string getWMRole() const;
    WindowState::WindowType getWindowType() const { return m_window_type; }
    void setWindowType(WindowState::WindowType type) { m_window_type = type; }
    long getCardinalProperty(Atom prop,bool*exists=NULL) const { return FbTk::FbWindow::cardinalProperty(prop,exists); }
    FbTk::FbString getTextProperty(Atom prop,bool*exists=NULL) const { return FbTk::FbWindow::textProperty(prop,exists); }

    WinClient *transientFor() { return transient_for; }
    const WinClient *transientFor() const { return transient_for; }
    TransientList &transientList() { return transients; }
    const TransientList &transientList() const { return transients; }
    bool isTransient() const { return transient_for != 0; }

    bool isModal() const { return m_modal_count > 0; }
    bool isStateModal() const { return m_modal; }
    void setStateModal(bool state);

    int gravity() const { return m_size_hints.win_gravity; }

    bool hasGroupLeftWindow() const;
    // grouping is tracked by remembering the window to the left in the group
    Window getGroupLeftWindow() const;

    const MwmHints *getMwmHint() const { return m_mwm_hint; }
    const SizeHints &sizeHints() const { return m_size_hints; }

    unsigned int minWidth() const { return m_size_hints.min_width; }
    unsigned int minHeight() const { return m_size_hints.min_height; }
    unsigned int maxWidth() const { return m_size_hints.max_width; }
    unsigned int maxHeight() const { return m_size_hints.max_height; }
    unsigned int widthInc() const { return m_size_hints.width_inc; }
    unsigned int heightInc() const { return m_size_hints.height_inc; }

    static const int PropMwmHintsElements = 3;

    /**
       !! TODO !!
       remove or move these to private
     */

    WinClient *transient_for; // which window are we a transient for?
    std::list<WinClient *> transients;  // which windows are our transients?
    Window window_group;

 
    int old_bw;
    unsigned long initial_state, normal_hint_flags, wm_hint_flags;

private:
    /// removes client from any waiting list and clears empty waiting lists
    void removeTransientFromWaitingList();

    // some transient of ours (or us) is modal
    void addModal() { ++m_modal_count; }
    // some transient (or us) is no longer modal
    void removeModal() { --m_modal_count; }

    FbTk::Timer m_title_update_timer;
    void emitTitleSig();

    // number of transients which we are modal for
    int m_modal_count;
    bool m_modal;
    bool accepts_input, send_focus_message, send_close_message;

    bool m_title_override;
    bool m_icon_override;

    WindowState::WindowType m_window_type;
    MwmHints *m_mwm_hint;
    SizeHints m_size_hints;

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
