// MultLayers.hh for FbTk - fluxbox toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_MULTLAYERS_HH
#define FBTK_MULTLAYERS_HH

#include <vector>
#include <cstdlib> // size_t

namespace FbTk {

class LayerItem;
class Layer;

class MultLayers {
public:
    explicit MultLayers(int numlayers);
    ~MultLayers();
    LayerItem *getLowestItemAboveLayer(int layernum);

    /// if there are none below, it will return null
    LayerItem *getItemBelow(LayerItem &item);
    LayerItem *getItemAbove(LayerItem &item);
    void addToTop(LayerItem &item, int layernum);
    void remove(LayerItem &item);

    // raise/lower the whole layer
    void raise(Layer &layer);
    void lower(Layer &layer);

    // raise/lower the item a whole layer, not just to top of current layer
    void raiseLayer(LayerItem &item);
    void lowerLayer(LayerItem &item);

    void moveToLayer(LayerItem &item, int layernum);
    int  size();

    Layer *getLayer(size_t num);
    const Layer *getLayer(size_t num) const;

    bool isUpdatable() const { return m_lock == 0; }
    void lock() { ++m_lock; }
    void unlock() { if (--m_lock == 0) restack(); }

private:
    void restack();

    std::vector<Layer *> m_layers;
    int m_lock;
};

}
#endif // FBTK_MULTLAYERS_HH
