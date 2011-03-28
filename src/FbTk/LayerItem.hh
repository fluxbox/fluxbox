// LayerItem.hh for FbTk - fluxbox toolkit
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

#ifndef FBTK_LAYERITEM_HH
#define FBTK_LAYERITEM_HH

#include "Layer.hh"
#include "NotCopyable.hh"
#include <vector>
#include <cstdlib> // size_t

namespace FbTk {

class FbWindow;

class LayerItem : private NotCopyable {
public:
    typedef std::vector<FbWindow *> Windows;

    LayerItem(FbWindow &win, Layer &layer);
    ~LayerItem();

    void setLayer(Layer &layer);

    void raise();
    void lower();
    void tempRaise(); // this raise gets reverted by a restack()

    // send to next layer up
    void raiseLayer();
    void lowerLayer();
    void moveToLayer(int layernum);

    // this is needed for step and cycle functions
    // (you need to know the next one visible, otherwise nothing may appear to happen)
    // not yet implemented
    bool visible() const { return true; } 

    const Layer &getLayer() const { return *m_layer; }
    Layer &getLayer() { return *m_layer; }
    int getLayerNum() { return m_layer->getLayerNum(); }

    // an LayerItem holds several windows that are equivalent in a layer 
    // (i.e. if one is raised, then they should all be).
    void addWindow(FbWindow &win);
    void removeWindow(FbWindow &win);

    // using this you can bring one window to the top of this item (equivalent to add then remove)
    void bringToTop(FbWindow &win);

    Windows &getWindows() { return m_windows; }
    size_t numWindows() const { return m_windows.size(); }

private:
    Layer *m_layer;
    Windows m_windows;
};

}

#endif // FBTK_LAYERITEM_HH
