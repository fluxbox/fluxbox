// XLayer.cc for FbTk - fluxbox toolkit
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

#include "XLayer.hh"
#include "XLayerItem.hh"
#include "App.hh"
#include "FbWindow.hh"
#include "MultLayers.hh"

#include <iostream>

using std::find;
using namespace FbTk;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

XLayer::XLayer(MultLayers &manager, int layernum):
    m_manager(manager), m_layernum(layernum), m_needs_restack(false) {
}

XLayer::~XLayer() {

}

void XLayer::restack() {
    if (!m_manager.isUpdatable())
        return;

    int num_windows = countWindows();

    // each LayerItem can contain several windows
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    Window *winlist = new Window[num_windows];
    size_t j=0;

    // add all the windows from each item
    for (; it != it_end; ++it) {
        XLayerItem::Windows::const_iterator wit = (*it)->getWindows().begin();
        XLayerItem::Windows::const_iterator wit_end = (*it)->getWindows().end();
        for (; wit != wit_end; ++wit) {
            if ((*wit)->window())
                winlist[j++] = (*wit)->window();
        }
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, j);

    delete [] winlist;

    m_needs_restack = false;
}

void XLayer::restackAndTempRaise(XLayerItem &item) {
    int num_windows = countWindows();

    // each LayerItem can contain several windows
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    Window *winlist = new Window[num_windows];
    size_t j=0;

    // add windows that go on top
    XLayerItem::Windows::const_iterator wit = item.getWindows().begin();
    XLayerItem::Windows::const_iterator wit_end = item.getWindows().end();
    for (; wit != wit_end; ++wit) {
        if ((*wit)->window())
            winlist[j++] = (*wit)->window();
    }

    // add all the windows from each other item
    for (; it != it_end; ++it) {
        if (*it == &item)
            continue;
        wit = (*it)->getWindows().begin();
        wit_end = (*it)->getWindows().end();
        for (; wit != wit_end; ++wit) {
            if ((*wit)->window())
                winlist[j++] = (*wit)->window();
        }
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, j);

    delete [] winlist;
}

int XLayer::countWindows() {
    int num_windows = 0;
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    for (; it != it_end; ++it) {
        num_windows += (*it)->numWindows();
    }
    return num_windows;
}


// Stack all windows associated with 'item' below the 'above' item
void XLayer::stackBelowItem(XLayerItem &item, XLayerItem *above) {
    if (!m_manager.isUpdatable())
        return;

    // if there are no windows provided for above us,
    // then we must restack the entire layer
    // we can't do XRaiseWindow because a restack then causes OverrideRedirect
    // windows to get pushed to the bottom
    if (!above || m_needs_restack) { // must need to go right to top
        restack();
        return;
    }

    Window *winlist;
    size_t winnum = 1, size = item.numWindows()+1;

    // We do have a window to stack below
    // so we put it on top, and fill the rest of the array with the ones to go below it.
    winlist = new Window[size];
    // assume that above's window exists
    winlist[0] = above->getWindows().back()->window();

    // fill the rest of the array
    XLayerItem::Windows::iterator it = item.getWindows().begin();
    XLayerItem::Windows::iterator it_end = item.getWindows().end();
    for (; it != it_end; ++it) {
        if ((*it)->window())
            winlist[winnum++] = (*it)->window();
    }

    // stack the windows
    XRestackWindows(FbTk::App::instance()->display(), winlist, winnum);

    delete [] winlist;

}

// We can't just use Restack here, because it won't do anything if they're
// already in the same relative order excluding other windows
void XLayer::alignItem(XLayerItem &item) {
    if (itemList().front() == &item) {
        stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));
        return;
    }

    // Note: some other things effectively assume that the window list is
    // sorted from highest to lowest
    // get our item
    iterator myit = find(itemList().begin(), itemList().end(), &item);
    iterator it = myit;

    // go to the one above it in our layer (top is front, so we decrement)
    --it;

    // keep going until we find one that is currently visible to the user
    while (it != itemList().begin() && !(*it)->visible())
        --it;

    if (it == itemList().begin() && !(*it)->visible())
        // reached front item, but it wasn't visible, therefore it was already raised
        stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));
    else
        stackBelowItem(item, *it);

}

XLayer::iterator XLayer::insert(XLayerItem &item, unsigned int pos) {
#ifdef DEBUG
    // at this point we don't support insertions into a layer other than at the top
    if (pos != 0)
        cerr<<__FILE__<<"("<<__LINE__<<"): Insert using non-zero position not valid in XLayer"<<endl;
#endif // DEBUG

    itemList().push_front(&item);
    // restack below next window up
    stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));
    return itemList().begin();
}

void XLayer::remove(XLayerItem &item) {
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    for (; it != it_end; ++it) {
        if (*it == &item) {
            itemList().erase(it);
            break;
        }
    }
}

void XLayer::raise(XLayerItem &item) {
    // assume it is already in this layer

    if (&item == itemList().front()) {
        if (m_needs_restack)
            restack();
        return; // nothing to do
    }

    iterator it = find(itemList().begin(), itemList().end(), &item);
    if (it != itemList().end())
        itemList().erase(it);
    else {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: raise on item not in layer["<<m_layernum<<"]"<<endl;
#endif // DEBUG
        return;
    }

    itemList().push_front(&item);
    stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));

}

void XLayer::tempRaise(XLayerItem &item) {
    // assume it is already in this layer

    if (!m_needs_restack && &item == itemList().front())
        return; // nothing to do

    iterator it = find(itemList().begin(), itemList().end(), &item);
    if (it == itemList().end()) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: raise on item not in layer["<<m_layernum<<"]"<<endl;
#endif // DEBUG
        return;
    }

    if (m_needs_restack)
        restackAndTempRaise(item);
    else
        stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));

    m_needs_restack = true;
}

void XLayer::lower(XLayerItem &item) {
    // assume already in this layer

    // is it already the lowest?
    if (&item == itemList().back()) {
        if (m_needs_restack)
            restack();
        return; // nothing to do
    }

    iterator it = find(itemList().begin(), itemList().end(), &item);
    if (it != itemList().end())
        // remove this item
        itemList().erase(it);
#ifdef DEBUG
    else {
        cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: lower on item not in layer"<<endl;
        return;
    }
#endif // DEBUG

    // add it to the bottom
    itemList().push_back(&item);

    // find the item we need to stack below
    // start at the end
    it = itemList().end();

    // go up one so we have an object (which must exist, since at least this item is in the layer)
    it--;

    // go down another one
    // must exist, otherwise our item must == itemList().back()
    it--;

    // and restack our window below that one.
    stackBelowItem(item, *it);
}

void XLayer::raiseLayer(XLayerItem &item) {
    m_manager.raiseLayer(item);
}

void XLayer::lowerLayer(XLayerItem &item) {
    m_manager.lowerLayer(item);
}

void XLayer::moveToLayer(XLayerItem &item, int layernum) {
    m_manager.moveToLayer(item, layernum);
}


XLayerItem *XLayer::getLowestItem() {
    if (itemList().empty())
        return 0;
    else
        return itemList().back();
}

