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

// $Id$

#ifndef FBTK_MULTLAYERS_HH
#define FBTK_MULTLAYERS_HH

#include <vector>

namespace FbTk {

class XLayerItem;
class XLayer;

class MultLayers {
public:
    explicit MultLayers(int numlayers);
    virtual ~MultLayers();
    XLayerItem *getLowestItemAboveLayer(int layernum);

    /// if there are none below, it will return null
    XLayerItem *getItemBelow(XLayerItem &item);
    XLayerItem *getItemAbove(XLayerItem &item);
    void addToTop(XLayerItem &item, int layernum);
    void remove(XLayerItem &item);

    // raise/lower the whole layer
    void raise(XLayer &layer);
    void lower(XLayer &layer);

    // raise/lower the item a whole layer, not just to top of current layer
    void raiseLayer(XLayerItem &item);
    void lowerLayer(XLayerItem &item);

    void moveToLayer(XLayerItem &item, int layernum);
    int  size();
    void restack();

    XLayer *getLayer(size_t num);
    const XLayer *getLayer(size_t num) const;

    inline bool isUpdatable() const { return m_lock == 0; }
    inline void lock() { ++m_lock; }
    inline void unlock() { if (--m_lock == 0) restack(); }

private:
    std::vector<XLayer *> m_layers;

    int m_lock;

};

};
#endif // FBTK_MULTLAYERS_HH
