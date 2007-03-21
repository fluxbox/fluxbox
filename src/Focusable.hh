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

#include "FbTk/FbPixmap.hh"

#include <string>

class FluxboxWindow;

// base class for any object that might be "focused"
class Focusable {
public:
    Focusable(FluxboxWindow *fbwin = 0): m_fbwin(fbwin) { }
    virtual ~Focusable() { }

    virtual bool focus() { return false; }

    // so we can make nice buttons, menu entries, etc.
    virtual const FbTk::FbPixmap &iconPixmap() const { return m_icon_pixmap; }
    virtual bool usePixmap() const { return iconPixmap().drawable() != None; }

    virtual const FbTk::FbPixmap &iconMask() const { return m_icon_mask; }
    virtual bool useMask() const { return iconMask().drawable() != None; }

    virtual const std::string &title() const { return m_title; }
    virtual const std::string &iconTitle() const { return m_icon_title; }

    // for accessing window properties, like shaded, minimized, etc.
    inline const FluxboxWindow *fbwindow() const { return m_fbwin; }
    inline FluxboxWindow *fbwindow() { return m_fbwin; }

protected:
    std::string m_title, m_icon_title;
    FbTk::FbPixmap m_icon_pixmap, m_icon_mask;
    FluxboxWindow *m_fbwin;
};

#endif // FOCUSABLE_HH
