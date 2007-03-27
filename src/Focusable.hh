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

// base class for any object that might be "focused"
class Focusable: public FbTk::ITypeAheadable {
public:
    Focusable(BScreen &scr, FluxboxWindow *fbwin = 0):
        m_screen(scr), m_fbwin(fbwin),
        m_focused(false), m_titlesig(*this) { }
    virtual ~Focusable() { }

    virtual bool focus() { return false; }

    virtual bool isFocused() const { return m_focused; }
    virtual bool acceptsFocus() const { return true; }

    inline BScreen &screen() { return m_screen; }
    inline const BScreen &screen() const { return m_screen; }

    // for accessing window properties, like shaded, minimized, etc.
    inline const FluxboxWindow *fbwindow() const { return m_fbwin; }
    inline FluxboxWindow *fbwindow() { return m_fbwin; }

    // so we can make nice buttons, menu entries, etc.
    virtual const FbTk::PixmapWithMask &icon() const { return m_icon; }
    virtual const std::string &title() const { return m_title; }
    const std::string &iTypeString() const { return title(); }

    class FocusSubject: public FbTk::Subject {
    public:
        FocusSubject(Focusable &w):m_win(w) { }
        Focusable &win() { return m_win; }
        const Focusable &win() const { return m_win; }
    private:
        Focusable &m_win;
    };

    // used for both title and icon changes
    FbTk::Subject &titleSig() { return m_titlesig; }
    const FbTk::Subject &titleSig() const { return m_titlesig; }

protected:
    BScreen &m_screen;
    FluxboxWindow *m_fbwin;

    bool m_focused;
    std::string m_title;
    FbTk::PixmapWithMask m_icon;

    FocusSubject m_titlesig;
};

#endif // FOCUSABLE_HH
