// XLayer.cc for FbTk - fluxbox toolkit
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

// $Id: XLayer.cc,v 1.3 2003/02/02 16:32:41 rathnor Exp $

#include "XLayer.hh"
#include "XLayerItem.hh"
#include "App.hh"

#include <iostream>
using namespace std;
using namespace FbTk;

XLayer::XLayer(MultLayers &manager, int layernum):
    m_manager(manager), m_layernum(layernum) {
}

XLayer::~XLayer() {
}

void XLayer::restack() {
    int numWindows = countWindows();

    // each LayerItem can contain several windows
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    it = itemList().begin();
    it_end = itemList().end();
    Window *winlist = new Window[numWindows];
    size_t j=0;
    for (size_t i=0; it != it_end; ++it, i++) {
        XLayerItem::Windows::const_iterator wit = (*it)->getWindows().begin();
        XLayerItem::Windows::const_iterator wit_end = (*it)->getWindows().end();
        for (; wit != wit_end; ++wit, j++) {
            winlist[j] = (*wit);
        }
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, numWindows);

    delete [] winlist;
}

int XLayer::countWindows() {
    int numWindows = 0;
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    for (size_t i=0; it != it_end; ++it, i++) {
        numWindows += (*it)->numWindows();
    }
    return numWindows;
}


void XLayer::stackBelowItem(XLayerItem *item, XLayerItem *above) {
    // little optimisation
    Window *winlist;
    size_t i, size, num = item->numWindows();

    if (!above) { // must need to go right to top
        XRaiseWindow(FbTk::App::instance()->display(), item->getWindows().front());
        if (num > 1) {
            i = 0;
            // stack relative to top one (just raised)
            size = num;
            winlist = new Window[size];
        } else {
            return;
        }
    } else {

        i=1;
        // stack relative to one above

        size = num+1;
        winlist = new Window[size];
        winlist[0] = above->getWindows().front();
    }

    XLayerItem::Windows::iterator it = item->getWindows().begin();
    XLayerItem::Windows::iterator it_end = item->getWindows().end();
    for (; it != it_end; ++it, i++) {
        winlist[i] = (*it);
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, size);

    delete [] winlist;
}

XLayer::iterator XLayer::insert(XLayerItem &item, unsigned int pos) {
#ifdef DEBUG
    if (pos != 0)
        cerr<<__FILE__<<"("<<__LINE__<<"): Insert using non-zero position not valid in XLayer"<<endl;
#endif // DEBUG
    
    itemList().push_front(&item);
    item.setLayer(this);
    // restack below next window up
    item.setLayerIterator(itemList().begin());
    stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
    return itemList().begin();
}

void XLayer::remove(XLayerItem &item) {
    itemList().erase(item.getLayerIterator());
    item.setLayer(0);
}

void XLayer::cycleUp() {
    // need to find highest visible window, and move it to bottom
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    while (it != it_end && !(*it)->visible()) ++it;
    
    // if there is something to do
    if (it != it_end) {
        lower(**it);
    }
}

void XLayer::cycleDown() {
    // need to find highest visible window, and move it to bottom
    reverse_iterator it = itemList().rbegin();
    reverse_iterator it_end = itemList().rend();
    while (it != it_end && !(*it)->visible()) ++it;
    
    // if there is something to do
    if (it != it_end) {
        raise(**it);
    }
    
}

void XLayer::stepUp(XLayerItem &item) {
    // need to find next visible window upwards, and put it above that

    if (&item == itemList().front()) return; // nothing to do

    // TODO: is there a better way of doing this?
    iterator it = item.getLayerIterator();
    it--;
    while ((*it) != itemList().front() && !(*it)->visible()) --it;
    
    if (*it == itemList().front() && !(*it)->visible()) {
        // reached front item, but it wasn't visible, therefore it was already raised
        //moveToBottom(item);
    } else {
        // it is the next visible item down, we need to be above it.
        itemList().erase(item.getLayerIterator());
        //itemList().insert(it, item);
        item.setLayerIterator(it = itemList().insert(it, &item));
        if (*it == itemList().front()) {
            stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
        } else {
            it--;
            stackBelowItem(&item, *it);
        }
    }
}

void XLayer::stepDown(XLayerItem &item) {
    // need to find next visible window down, and put it below that
    if (&item == itemList().back()) return; // nothing to do


    iterator it = item.getLayerIterator();
    it++;
    iterator it_end = itemList().end();
    while (it != it_end && !(*it)->visible()) ++it;

    if (it != it_end) {
        stackBelowItem(&item, *it);
    }
}

void XLayer::raise(XLayerItem &item) {
    // assume it is already in this layer

    if (&item == itemList().front()) {
        return; // nothing to do
    }

    itemList().erase(item.getLayerIterator());
    itemList().push_front(&item);
    item.setLayerIterator(itemList().begin());
    stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
    
}

void XLayer::lower(XLayerItem &item) {
    // assume already in this layer
    if (&item == itemList().back()) 
        return; // nothing to do

    itemList().erase(item.getLayerIterator());
    itemList().push_back(&item);
    iterator it = itemList().end();
    it--;
    item.setLayerIterator(it);
    it--;
    stackBelowItem(&item, *it); // must exist, otherwise item must == itemList().back()
}

XLayerItem *XLayer::getLowestItem() {
    if (itemList().empty()) return 0;
    else return itemList().back();
}

XLayerItem *XLayer::getItemBelow(XLayerItem &item) {
    iterator it = item.getLayerIterator();
    if (it == itemList().begin()) {
        return 0;
    } else {
        return *(--it);
    }
}

XLayerItem *XLayer::getItemAbove(XLayerItem &item) {
    iterator it = item.getLayerIterator();
    it++;
    if (it == itemList().end()) {
        return 0;
    } else {
        return *it;
    }
}
