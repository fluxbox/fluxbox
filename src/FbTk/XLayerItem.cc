// XLayerItem.cc for FbTk - fluxbox toolkit
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

// $Id: XLayerItem.cc,v 1.3 2003/02/02 16:32:41 rathnor Exp $

#include "XLayerItem.hh"
#include "XLayer.hh"

using namespace FbTk;

XLayerItem::XLayerItem(Window win) :
    m_layer(0), m_layeriterator(0) {
    m_windows.push_front(win);
}

/*
XLayerItem::XLayerItem(XLayer &layer): 
    m_layer(&layer) {
    m_layeriterator = layer.insert(*this);
    }*/

XLayerItem::~XLayerItem() {
    if (m_layer) 
        m_layer->remove(*this);
}

void XLayerItem::setLayer(XLayer *layer) {
    // make sure we don't try to set the same layer
    m_layer = layer;
}

void XLayerItem::raise() {
    m_layer->raise(*this); 
}

void XLayerItem::lower() {
    m_layer->lower(*this);
}

void XLayerItem::stepUp() {
    m_layer->stepUp(*this);
}

void XLayerItem::stepDown() {
    m_layer->stepDown(*this);
}

void XLayerItem::addWindow(Window win) {
  // I'd like to think we can trust ourselves that it won't be added twice...
  // Otherwise we're always scanning through the list.
  m_windows.push_back(win);
}

void XLayerItem::removeWindow(Window win) {
  // I'd like to think we can trust ourselves that it won't be added twice...
  // Otherwise we're always scanning through the list.

  XLayerItem::Windows::iterator it = std::find(m_windows.begin(), m_windows.end(), win);
  m_windows.erase(it);
}

void XLayerItem::bringToTop(Window win) {
  removeWindow(win);
  addWindow(win);
}
