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

#ifndef FOCUSABLE_HH
#define FOCUSABLE_HH

#include "FbTk/PixmapWithMask.hh"
#include "FbTk/ITypeAheadable.hh"
#include "FbTk/Signal.hh"
#include "FbTk/FbString.hh"

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
        m_attentionsig(),
        m_focussig(),
        m_diesig(),
        m_titlesig() { }
    virtual ~Focusable() { }

    /**
     * Take focus.
     * @return true if the focuable took focus
     */
    virtual bool focus() { return false; }

    /// @return true if the focusable has input focus
    virtual bool isFocused() const { return m_focused; }
    /// @return true if it can be focused
    virtual bool acceptsFocus() const { return true; }
    /// @return true if temporarily prevented from being focused
    virtual bool isModal() const { return false; }

    /// @return true if icon button should appear focused
    bool getAttentionState() const { return m_attention_state; }
    /// @set the attention state
    virtual void setAttentionState(bool value) {
        m_attention_state = value; attentionSig().emit(*this);
    }

    /// @return the screen in which this object resides
    BScreen &screen() { return m_screen; }
    /// @return the screen in which this object resides
    const BScreen &screen() const { return m_screen; }

    /**
     *  For accessing window properties, like shaded, minimized, etc.
     *  @return window context
     */
    const FluxboxWindow *fbwindow() const { return m_fbwin; }
    /**
     *  For accessing window properties, like shaded, minimized, etc.
     *  @return window context
     */
    FluxboxWindow *fbwindow() { return m_fbwin; }

    /// @return WM_CLASS class string (for pattern matching)
    virtual const FbTk::FbString &getWMClassClass() const { return m_class_name; }
    /// @return WM_CLASS name string (for pattern matching)
    virtual const FbTk::FbString &getWMClassName() const { return m_instance_name; }
    /// @return wm role string (for pattern matching)
    virtual std::string getWMRole() const { return "Focusable"; }

    virtual FbTk::FbString getTextProperty(Atom prop,bool*exists=NULL) const { return ""; }
    virtual long getCardinalProperty(Atom prop,bool*exists=NULL) const { return 0; }

    /// @return whether this window is a transient (for pattern matching)
    virtual bool isTransient() const { return false; }

    // so we can make nice buttons, menu entries, etc.
    /// @return icon pixmap of the focusable
    virtual const FbTk::PixmapWithMask &icon() const { return m_icon; }
    /// @return title string
    virtual const FbTk::BiDiString &title() const { return m_title; }
    /// @return type ahead string
    const std::string &iTypeString() const { return title().logical(); }

    /**
       @name signals
       @{
    */
    typedef FbTk::Signal<const std::string&, Focusable&> TitleSignal;
    /// Used for both title and icon changes.
    TitleSignal &titleSig() { return m_titlesig; }
    /// Used for both title and icon changes.
    const TitleSignal &titleSig() const { return m_titlesig; }
    FbTk::Signal<Focusable&> &focusSig() { return m_focussig; }
    FbTk::Signal<Focusable&> &dieSig() { return m_diesig; }
    FbTk::Signal<Focusable&> &attentionSig() { return m_attentionsig; }
    /** @} */ // end group signals

    /// Notify any listeners that the focus changed for this window.
    void notifyFocusChanged() {
        m_focussig.emit(*this);
    }

protected:
    BScreen &m_screen; //< the screen in which it works
    FluxboxWindow *m_fbwin; //< the working fluxbox window

    FbTk::BiDiString m_title;
    FbTk::FbString m_instance_name;
    FbTk::FbString m_class_name;

    bool m_focused; //< whether or not it has focus
    bool m_attention_state; //< state of icon button while demanding attention
    FbTk::PixmapWithMask m_icon; //< icon pixmap with mask


private:
    FbTk::Signal<Focusable&> m_attentionsig;
    FbTk::Signal<Focusable&> m_focussig;
    FbTk::Signal<Focusable&> m_diesig;
    TitleSignal m_titlesig;
};

#endif // FOCUSABLE_HH
