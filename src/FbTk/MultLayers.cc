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

// $Id: MultLayers.cc,v 1.3 2003/02/02 16:32:41 rathnor Exp $

#include "MultLayers.hh"
#include "XLayer.hh"
#include "XLayerItem.hh"
#include "App.hh"

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

XLayerItem *MultLayers::getItemBelow(XLayerItem &item) {
    XLayer *curr_layer = item.getLayer();
    cerr<<"getItemBelow xlayer = "<<hex<<curr_layer<<endl;
    // assume that the LayerItem does exist in a layer.
    XLayerItem *ret = curr_layer->getItemBelow(item);

    if (!ret) {
        int num = curr_layer->getLayerNum()-1;
        while (num >= 0 && !ret) {
            ret = m_layers[num]->getItemBelow(item);
            num--;
        }
    }

    return ret;
}    

XLayerItem *MultLayers::getItemAbove(XLayerItem &item) {
    XLayer *curr_layer = item.getLayer();
    
    // assume that the LayerItem does exist in a layer.
    XLayerItem *ret = curr_layer->getItemAbove(item);

    if (!ret) {
        ret = getLowestItemAboveLayer(curr_layer->getLayerNum());
    }

    return ret;
}    

void MultLayers::addToTop(XLayerItem &item, int layernum) {
    if (layernum < 0) 
        layernum = 0; 
    else if (layernum >= m_numlayers) 
        layernum = m_numlayers-1;

    m_layers[layernum]->insert(item);
    restack();
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
    moveToLayer(item, curr_layer->getLayerNum()-1);
}

/* lower the item one level */
void MultLayers::lower(XLayerItem &item) {
    // get the layer it is in
    XLayer *curr_layer = item.getLayer();
    moveToLayer(item, curr_layer->getLayerNum()+1);
}

void MultLayers::moveToLayer(XLayerItem &item, int layernum) {
    // get the layer it is in
    XLayer *curr_layer = item.getLayer();
    if (!curr_layer) {
        addToTop(item, layernum);
        return;
    }
    if (curr_layer->getLayerNum() == layernum )
        // do nothing
        return;
    
    if (layernum < 0) 
        layernum = 0; 
    else if (layernum >= m_numlayers) 
        layernum = m_numlayers-1;
    
    curr_layer->remove(item);
    m_layers[layernum]->insert(item);
}

void MultLayers::restack() {
    int i=0, j=0, size=0;
    for (; i < m_numlayers; i++) {
        size += m_layers[i]->countWindows();
    }

    Window *winlist = new Window[size];
    for (i=0; i < m_numlayers; i++) {

        XLayer::ItemList::iterator it = m_layers[i]->getItemList().begin();
        XLayer::ItemList::iterator it_end = m_layers[i]->getItemList().end();

        for (; it != it_end; ++it) {
            XLayerItem::Windows::const_iterator wit = (*it)->getWindows().begin();
            XLayerItem::Windows::const_iterator wit_end = (*it)->getWindows().end();
            for (; wit != wit_end; ++wit, j++) {
                winlist[j] = (*wit);
            }
        }
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, size);
}

int MultLayers::size() {
    int i = 0, num = 0;
    for (; i < m_numlayers; i++) {
        num += m_layers[i]->countWindows();
    }
    return num;
}
