// MultLayers.cc for FbTk - fluxbox toolkit
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

// $Id: MultLayers.cc,v 1.2 2003/01/29 21:42:52 rathnor Exp $

#include "MultLayers.hh"
#include "XLayer.hh"
#include "XLayerItem.hh"

using namespace FbTk;

MultLayers::MultLayers(int numlayers) :
    m_numlayers(numlayers), m_layers(numlayers) {
    for (int i=0; i < numlayers; i++) {
        m_layers[i] = new XLayer(*this, i);
    }
}

MultLayers::~MultLayers() {
    // TODO delete all the layers
}


XLayerItem *MultLayers::getLowestItemAboveLayer(int layernum) {
    if (layernum >= (m_numlayers) || layernum <= 0) 
        return 0;

    layernum--; // next one up
    XLayerItem *item = 0;
    while (layernum >= 0 && (item = m_layers[layernum]->getLowestItem()) == 0) layernum--;
    return item;

}    


void MultLayers::addToTop(XLayerItem &item, int layernum) {
    if (layernum < 0 || layernum >= m_numlayers) return;
    m_layers[layernum]->insert(item);
}

void MultLayers::remove(XLayerItem &item) {
    XLayer *curr_layer = item.getLayer();
    if (!curr_layer || curr_layer->getLayerNum() < 0 || curr_layer->getLayerNum() >= m_numlayers) {
        // do nothing
        return;
    }
    curr_layer->remove(item);
}

/* raise the item one level */
void MultLayers::raise(XLayerItem &item) {
    // get the layer it is in
    XLayer *curr_layer = item.getLayer();
    if (!curr_layer || curr_layer->getLayerNum() == 0 || m_numlayers == 1) {
        // do nothing
        return;
    }
    
    curr_layer->remove(item);
    m_layers[curr_layer->getLayerNum()-1]->insert(item);
}

/* lower the item one level */
void MultLayers::lower(XLayerItem &item) {
    // get the layer it is in
    XLayer *curr_layer = item.getLayer();
    if (!curr_layer || curr_layer->getLayerNum() >= (m_numlayers-1) || m_numlayers == 1) {
        // do nothing
        return;
    }
    
    curr_layer->remove(item);
    m_layers[curr_layer->getLayerNum()+1]->insert(item);
}
