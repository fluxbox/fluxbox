// MultLayers.cc for FbTk - fluxbox toolkit
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

#include "MultLayers.hh"
#include "XLayer.hh"
#include "XLayerItem.hh"
#include "App.hh"

using namespace FbTk;

MultLayers::MultLayers(int numlayers) :
    m_lock(0)
{
    for (int i=0; i < numlayers; ++i)
        m_layers.push_back(new XLayer(*this, i));
}

MultLayers::~MultLayers() {
    while (!m_layers.empty()) {
        delete m_layers.back();
        m_layers.pop_back();
    }
}


XLayerItem *MultLayers::getLowestItemAboveLayer(int layernum) {
    if (layernum >= static_cast<signed>(m_layers.size()) || layernum <= 0)
        return 0;

    layernum--; // next one up
    XLayerItem *item = 0;
    while (layernum >= 0 && (item = m_layers[layernum]->getLowestItem()) == 0)
        layernum--;
    return item;

}

XLayerItem *MultLayers::getItemBelow(XLayerItem &item) {
    XLayer &curr_layer = item.getLayer();

    // assume that the LayerItem does exist in a layer.
    XLayerItem *ret = curr_layer.getItemBelow(item);

    if (ret == 0) {
        int num = curr_layer.getLayerNum()-1;
        while (num >= 0 && !ret) {
            ret = m_layers[num]->getItemBelow(item);
            num--;
        }
    }

    return ret;
}

XLayerItem *MultLayers::getItemAbove(XLayerItem &item) {
    XLayer &curr_layer = item.getLayer();

    // assume that the LayerItem does exist in a layer.
    XLayerItem *ret = curr_layer.getItemAbove(item);

    if (!ret) {
        ret = getLowestItemAboveLayer(curr_layer.getLayerNum());
    }

    return ret;
}

void MultLayers::addToTop(XLayerItem &item, int layernum) {
    if (layernum < 0)
        layernum = 0;
    else if (layernum >= static_cast<signed>(m_layers.size()))
        layernum = m_layers.size()-1;

    m_layers[layernum]->insert(item);
    restack();
}


// raise the whole layer
void MultLayers::raise(XLayer &layer) {
    int layernum = layer.getLayerNum();
    if (layernum >= static_cast<signed>(m_layers.size() - 1))
        // already on top
        return;

    // not yet implemented
}

// lower the whole layer
void MultLayers::lower(XLayer &layer) {
    int layernum = layer.getLayerNum();
    if (layernum == 0)
        // already on bottom
        return;

    // not yet implemented
}

/* raise the item one level */
void MultLayers::raiseLayer(XLayerItem &item) {
    // get the layer it is in
    XLayer &curr_layer = item.getLayer();
    moveToLayer(item, curr_layer.getLayerNum()-1);
}

/* raise the item one level */
void MultLayers::lowerLayer(XLayerItem &item) {
    // get the layer it is in
    XLayer &curr_layer = item.getLayer();
    moveToLayer(item, curr_layer.getLayerNum()+1);
}

void MultLayers::moveToLayer(XLayerItem &item, int layernum) {
    // get the layer it is in
    XLayer &curr_layer = item.getLayer();

    // do nothing if the item already is in the requested layer
    if (curr_layer.getLayerNum() == layernum)
        return;

    // clamp layer number
    if (layernum < 0)
        layernum = 0;
    else if (layernum >= static_cast<signed>(m_layers.size()))
        layernum = m_layers.size()-1;
    // remove item from old layer and insert it into the
    item.setLayer(*m_layers[layernum]);
}

void MultLayers::restack() {
    if (!isUpdatable())
        return;

    int layernum=0, winnum=0, size = this->size();

    Window *winlist = new Window[size];
    for (layernum=0; layernum < static_cast<signed>(m_layers.size()); layernum++) {

        XLayer::ItemList::iterator it = m_layers[layernum]->getItemList().begin();
        XLayer::ItemList::iterator it_end = m_layers[layernum]->getItemList().end();

        // add all windows from each layeritem in each layer
        for (; it != it_end; ++it) {
            XLayerItem::Windows::const_iterator wit = (*it)->getWindows().begin();
            XLayerItem::Windows::const_iterator wit_end = (*it)->getWindows().end();
            for (; wit != wit_end; ++wit) {
                if ((*wit)->window())
                    winlist[winnum++] = (*wit)->window();
            }
        }
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, winnum);

    delete [] winlist;
}

int MultLayers::size() {
    int num = 0;
    for (size_t i = 0; i < m_layers.size(); i++) {
        num += m_layers[i]->countWindows();
    }
    return num;
}

XLayer *MultLayers::getLayer(size_t num) {
    if (num >= m_layers.size())
        return 0;
    return m_layers[num];
}

const XLayer *MultLayers::getLayer(size_t num) const {
    if (num >= m_layers.size())
        return 0;
    return m_layers[num];
}

