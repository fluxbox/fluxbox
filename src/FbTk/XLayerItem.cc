// XLayerItem.cc for FbTk - fluxbox toolkit
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

// $Id$

#include "XLayerItem.hh"
#include "XLayer.hh"

using namespace FbTk;

XLayerItem::XLayerItem(FbWindow &win, XLayer &layer) :
    m_layer(&layer) {
    m_windows.push_front(&win);
    m_layer->insert(*this);
}


XLayerItem::~XLayerItem() {
    m_layer->remove(*this);
}

void XLayerItem::setLayer(XLayer &layer) {
    // make sure we don't try to set the same layer
    if (m_layer == &layer)
        return;

    m_layer->remove(*this);
    m_layer = &layer;
    m_layer->insert(*this);
}

void XLayerItem::raise() {
    m_layer->raise(*this); 
}

void XLayerItem::lower() {
    m_layer->lower(*this);
}

void XLayerItem::tempRaise() {
    m_layer->tempRaise(*this); 
}

#ifdef NOT_USED
void XLayerItem::stepUp() {
    m_layer->stepUp(*this);
}

void XLayerItem::stepDown() {
    m_layer->stepDown(*this);
}
#endif // NOT_USED

void XLayerItem::raiseLayer() {
    m_layer->raiseLayer(*this); 
}

void XLayerItem::lowerLayer() {
    m_layer->lowerLayer(*this);
}

void XLayerItem::moveToLayer(int layernum) {
    m_layer->moveToLayer(*this, layernum);
}

void XLayerItem::addWindow(FbWindow &win) {
    // I'd like to think we can trust ourselves that it won't be added twice...
    // Otherwise we're always scanning through the list.
    m_windows.push_back(&win);
    m_layer->alignItem(*this);
}

void XLayerItem::removeWindow(FbWindow &win) {
  // I'd like to think we can trust ourselves that it won't be added twice...
  // Otherwise we're always scanning through the list.

  XLayerItem::Windows::iterator it = std::find(m_windows.begin(), m_windows.end(), &win);
  if (it != m_windows.end())
      m_windows.erase(it);
}

void XLayerItem::bringToTop(FbWindow &win) {
  removeWindow(win);
  addWindow(win);
}
