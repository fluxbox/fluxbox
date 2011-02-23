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

#include "MultLayers.hh"
#include "Layer.hh"
#include "LayerItem.hh"
#include "App.hh"
#include "FbWindow.hh"

#include "Util.hh"

using namespace FbTk;

MultLayers::MultLayers(int numlayers) :
    m_lock(0)
{
    for (int i=0; i < numlayers; ++i)
        m_layers.push_back(new Layer(*this, i));
}

MultLayers::~MultLayers() {
    while (!m_layers.empty()) {
        delete m_layers.back();
        m_layers.pop_back();
    }
}


LayerItem *MultLayers::getLowestItemAboveLayer(int layernum) {
    if (layernum >= static_cast<signed>(m_layers.size()) || layernum <= 0)
        return 0;

    layernum--; // next one up
    LayerItem *item = 0;
    while (layernum >= 0 && (item = m_layers[layernum]->getLowestItem()) == 0)
        layernum--;
    return item;

}

void MultLayers::addToTop(LayerItem &item, int layernum) {
    layernum = FbTk::Util::clamp(layernum, 0, static_cast<signed>(m_layers.size()) - 1);
    m_layers[layernum]->insert(item);
    restack();
}


// raise the whole layer
void MultLayers::raise(Layer &layer) {
    int layernum = layer.getLayerNum();
    if (layernum >= static_cast<signed>(m_layers.size() - 1))
        // already on top
        return;

    // not yet implemented
}

// lower the whole layer
void MultLayers::lower(Layer &layer) {
    int layernum = layer.getLayerNum();
    if (layernum == 0)
        // already on bottom
        return;

    // not yet implemented
}

/* raise the item one level */
void MultLayers::raiseLayer(LayerItem &item) {
    // get the layer it is in
    Layer &curr_layer = item.getLayer();
    moveToLayer(item, curr_layer.getLayerNum()-1);
}

/* raise the item one level */
void MultLayers::lowerLayer(LayerItem &item) {
    // get the layer it is in
    Layer &curr_layer = item.getLayer();
    moveToLayer(item, curr_layer.getLayerNum()+1);
}

void MultLayers::moveToLayer(LayerItem &item, int layernum) {
    // get the layer it is in
    Layer &curr_layer = item.getLayer();

    // do nothing if the item already is in the requested layer
    if (curr_layer.getLayerNum() == layernum)
        return;

    layernum = FbTk::Util::clamp(layernum, 0, static_cast<signed>(m_layers.size()) - 1);
    item.setLayer(*m_layers[layernum]);
}

void MultLayers::restack() {
    if (!isUpdatable())
        return;

    Layer::restack(m_layers);
}

int MultLayers::size() {
    int num = 0;
    for (size_t i = 0; i < m_layers.size(); i++) {
        num += m_layers[i]->countWindows();
    }
    return num;
}

Layer *MultLayers::getLayer(size_t num) {
    if (num >= m_layers.size())
        return 0;
    return m_layers[num];
}

const Layer *MultLayers::getLayer(size_t num) const {
    if (num >= m_layers.size())
        return 0;
    return m_layers[num];
}

