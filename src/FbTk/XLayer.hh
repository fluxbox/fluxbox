// XLayer.hh for FbTk - fluxbox toolkit
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

// $Id: XLayer.hh,v 1.1 2003/01/16 12:41:27 rathnor Exp $


#ifndef FBTK_XLAYER_HH
#define FBTK_XLAYER_HH

#include <list>
#include "Layer.hh"
#include "MultLayers.hh"

namespace FbTk {

class XLayerItem;

class XLayer : public FbTk::Layer<XLayerItem, std::list<XLayerItem *> > {
public:
    XLayer(MultLayers &manager, int layernum);
    ~XLayer();

    //typedef std::list<XLayerItem *>::iterator iterator;
    //typedef std::list<XLayerItem *>::reverse_iterator reverse_iterator;

    void setLayerNum(int layernum) { m_layernum = layernum; };
    int  getLayerNum() { return m_layernum; };
    void restack();
    void stackBelowItem(XLayerItem *item, XLayerItem *above);
    XLayerItem *getLowestItem();
    
    // we redefine these as XLayer has special optimisations, and X restacking needs
    iterator insert(XLayerItem &item, unsigned int pos=0);
    void remove(XLayerItem &item);

    void cycleUp();
    void cycleDown();
    void raise(XLayerItem &item);
    void lower(XLayerItem &item);
    void stepUp(XLayerItem &item);
    void stepDown(XLayerItem &item);

private:
    MultLayers &m_manager;
    int m_layernum;

};

};

#endif // FBTK_XLAYER_HH
