// LayerItem.cc for FbTk - fluxbox toolkit
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "LayerItem.hh"
#include "Layer.hh"

#include <algorithm>

using namespace FbTk;

LayerItem::LayerItem(FbWindow &win, Layer &layer) :
    m_layer(&layer) {
    m_windows.push_back(&win);
    m_layer->insert(*this);
}


LayerItem::~LayerItem() {
    m_layer->remove(*this);
}

void LayerItem::setLayer(Layer &layer) {
    // make sure we don't try to set the same layer
    if (m_layer == &layer)
        return;

    m_layer->remove(*this);
    m_layer = &layer;
    m_layer->insert(*this);
}

void LayerItem::raise() {
    m_layer->raise(*this); 
}

void LayerItem::lower() {
    m_layer->lower(*this);
}

void LayerItem::tempRaise() {
    m_layer->tempRaise(*this); 
}

void LayerItem::raiseLayer() {
    m_layer->raiseLayer(*this); 
}

void LayerItem::lowerLayer() {
    m_layer->lowerLayer(*this);
}

void LayerItem::moveToLayer(int layernum) {
    m_layer->moveToLayer(*this, layernum);
}

void LayerItem::addWindow(FbWindow &win) {
    // I'd like to think we can trust ourselves that it won't be added twice...
    // Otherwise we're always scanning through the list.
    m_windows.push_back(&win);
    m_layer->alignItem(*this);
}

void LayerItem::removeWindow(FbWindow &win) {
    // I'd like to think we can trust ourselves that it won't be added twice...
    // Otherwise we're always scanning through the list.

    LayerItem::Windows::iterator it = std::find(m_windows.begin(), m_windows.end(), &win);
    if (it != m_windows.end())
        m_windows.erase(it);
}

void LayerItem::bringToTop(FbWindow &win) {
    removeWindow(win);
    addWindow(win);
}
