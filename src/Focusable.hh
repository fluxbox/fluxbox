// Focusable.hh
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef FOCUSABLE_HH
#define FOCUSABLE_HH

#include "FbTk/PixmapWithMask.hh"
#include "FbTk/ITypeAheadable.hh"
#include "FbTk/Subject.hh"

#include <string>

class BScreen;
class FluxboxWindow;

/** 
 * A Base class for any object that might be "focused".
 * Such as FluxboxWindow, Menu etc
 */
class Focusable: public FbTk::ITypeAheadable {
public:
    Focusable(BScreen &scr, FluxboxWindow *fbwin = 0):
        m_screen(scr), m_fbwin(fbwin),
        m_instance_name("fluxbox"), m_class_name("fluxbox"),
        m_focused(false), m_attention_state(false),
        m_titlesig(*this), m_focussig(*this), m_diesig(*this),
        m_attentionsig(*this) { }
    virtual ~Focusable() { }
    /**
     * Take focus.
     * @return true if the focuable took focus
     */
    virtual bool focus() { return false; }

    /// @return true if the focusable has input focus
    virtual bool isFocused() const { return m_focused; }
    /// @return return true if it can be focused
    virtual bool acceptsFocus() const { return true; }

    /// @return true if icon button should appear focused
    inline bool getAttentionState() const { return m_attention_state; }
    /// @set the attention state
    virtual void setAttentionState(bool value) {
        m_attention_state = value; attentionSig().notify();
    }

    /// @return the screen in which this object resides
    inline BScreen &screen() { return m_screen; }
    /// @return the screen in which this object resides
    inline const BScreen &screen() const { return m_screen; }

    /**
     *  For accessing window properties, like shaded, minimized, etc.
     *  @return window context
     */
    inline const FluxboxWindow *fbwindow() const { return m_fbwin; }
    /**
     *  For accessing window properties, like shaded, minimized, etc.
     *  @return window context
     */
    inline FluxboxWindow *fbwindow() { return m_fbwin; }

    /// @return WM_CLASS class string (for pattern matching)
    virtual const std::string &getWMClassClass() const { return m_class_name; }
    /// @return WM_CLASS name string (for pattern matching)
    virtual const std::string &getWMClassName() const { return m_instance_name; }
    /// @return wm role string (for pattern matching)
    virtual std::string getWMRole() const { return "Focusable"; }

    /// @return whether this window is a transient (for pattern matching)
    virtual bool isTransient() const { return false; }

    // so we can make nice buttons, menu entries, etc.
    /// @return icon pixmap of the focusable
    virtual const FbTk::PixmapWithMask &icon() const { return m_icon; }
    /// @return title string
    virtual const std::string &title() const { return m_title; }
    /// @return type ahead string
    const std::string &iTypeString() const { return title(); }
    /**
     * Signaling object to attatch observers to.
     */
    class FocusSubject: public FbTk::Subject {
    public:
        explicit FocusSubject(Focusable &w):m_win(w) { }
        /// @return context focusable for this signal
        Focusable &win() { return m_win; }
        /// @return context focusable for this signal
        const Focusable &win() const { return m_win; }
    private:
        Focusable &m_win; //< the context
    };

    /**
       @name signals
       @{
    */
    // Used for both title and icon changes.
    FbTk::Subject &titleSig() { return m_titlesig; }
    // Used for both title and icon changes.
    const FbTk::Subject &titleSig() const { return m_titlesig; }
    FbTk::Subject &focusSig() { return m_focussig; }
    const FbTk::Subject &focusSig() const { return m_focussig; }
    FbTk::Subject &dieSig() { return m_diesig; }
    const FbTk::Subject &dieSig() const { return m_diesig; }
    FbTk::Subject &attentionSig() { return m_attentionsig; }
    const FbTk::Subject &attentionSig() const { return m_attentionsig; }
    /** @} */ // end group signals

protected:
    BScreen &m_screen; //< the screen in which it works
    FluxboxWindow *m_fbwin; //< the working fluxbox window

    std::string m_title, m_instance_name, m_class_name;
    bool m_focused; //< whether or not it has focus
    bool m_attention_state; //< state of icon button while demanding attention
    FbTk::PixmapWithMask m_icon; //< icon pixmap with mask

    // state and hint signals
    FocusSubject m_titlesig, m_focussig, m_diesig, m_attentionsig;
};

#endif // FOCUSABLE_HH
