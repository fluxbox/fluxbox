// XLayerItem.hh for FbTk - fluxbox toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: XLayerItem.hh,v 1.4 2003/02/03 13:42:47 fluxgen Exp $

#ifndef FBTK_XLAYERITEM_HH
#define FBTK_XLAYERITEM_HH

#include "LayerItem.hh"
#include "XLayer.hh"
#include "NotCopyable.hh"

#include <X11/Xlib.h>


namespace FbTk {

class XLayerItem : public LayerItem, private NotCopyable {
public:
    typedef std::list<Window> Windows;

    XLayerItem(Window win, XLayer &layer);
    ~XLayerItem();

    void setLayer(XLayer &layer);

    void raise();
    void lower();
    void stepUp();
    void stepDown();
    //!! we don't need this?
    bool visible() const { return true; } 

    const XLayer &getLayer() const { return *m_layer; }
    XLayer &getLayer() { return *m_layer; }
    Window window() const { return m_window; }

private:
    XLayer *m_layer;
    XLayer::iterator m_layeriterator;
    Window m_window;
};

};

#endif // FBTK_XLAYERITEM_HH
