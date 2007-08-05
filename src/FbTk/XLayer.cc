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

// $Id$

#include "XLayer.hh"
#include "XLayerItem.hh"
#include "App.hh"

#include <iostream>

using std::find;
using namespace FbTk;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

XLayer::XLayer(MultLayers &manager, int layernum):
    m_manager(manager), m_layernum(layernum) {
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
void XLayer::stackBelowItem(XLayerItem *item, XLayerItem *above) {
    if (!m_manager.isUpdatable())
        return;

    // if there are no windows provided for above us,
    // then we must restack the entire layer
    // we can't do XRaiseWindow because a restack then causes OverrideRedirect
    // windows to get pushed to the bottom
    if (!above) { // must need to go right to top
        restack();
        return;
    }

    Window *winlist;
    size_t winnum = 1, size = item->numWindows()+1;

    // We do have a window to stack below
    // so we put it on top, and fill the rest of the array with the ones to go below it.
    winlist = new Window[size];
    // assume that above's window exists
    winlist[0] = above->getWindows().back()->window();

    // fill the rest of the array
    XLayerItem::Windows::iterator it = item->getWindows().begin();
    XLayerItem::Windows::iterator it_end = item->getWindows().end();
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
        stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
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
        stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
    else
        stackBelowItem(&item, *it);

}

XLayer::iterator XLayer::insert(XLayerItem &item, unsigned int pos) {
#ifdef DEBUG
    // at this point we don't support insertions into a layer other than at the top
    if (pos != 0)
        cerr<<__FILE__<<"("<<__LINE__<<"): Insert using non-zero position not valid in XLayer"<<endl;
#endif // DEBUG

    itemList().push_front(&item);
    // restack below next window up
    stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
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

#ifdef NOT_USED
void XLayer::cycleUp() {
    // need to find highest visible window, and move it to bottom
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    while (it != it_end && !(*it)->visible())
        ++it;

    // if there is something to do
    if (it != it_end)
        lower(**it);

}

void XLayer::cycleDown() {
    // need to find lowest visible window, and move it to top
    // so use a reverse iterator, and the same logic as cycleUp()
    reverse_iterator it = itemList().rbegin();
    reverse_iterator it_end = itemList().rend();
    while (it != it_end && !(*it)->visible())
        ++it;

    // if there is something to do
    if (it != it_end)
        raise(**it);

}

void XLayer::stepUp(XLayerItem &item) {
    // need to find next visible window upwards, and put it above that

    if (&item == itemList().front())
        return; // nothing to do

    // TODO: is there a better way of doing this?

    // get our item
    iterator myit = find(itemList().begin(), itemList().end(), &item);
    iterator it = myit;

    // go to the one above it in our layer (top is front, so we decrement)
    --it;

    // keep going until we find one that is currently visible to the user
    while (it != itemList().begin() && !(*it)->visible())
        --it;

    if (it == itemList().begin() && !(*it)->visible()) {
        // reached front item, but it wasn't visible, therefore it was already raised
    } else {
        // it is the next visible item down, we need to be above it.
        // remove that item from the list and add it back to before it (the one we want to go above)

        itemList().erase(myit);
        itemList().insert(it, &item);

        // if we've reached the top of the layer, we need to stack below the next one up
        if (it == itemList().begin()) {
            stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));
        } else {
            // otherwise go up one in this layer (i.e. above the one we want to go above)
            --it;
            // and stack below that.
            stackBelowItem(&item, *it);
        }
    }
}

void XLayer::stepDown(XLayerItem &item) {
    // need to find next visible window down, and put it below that

    // if we're already the bottom of the layer
    if (&item == itemList().back())
        return; // nothing to do

    // get our position
    iterator it = find(itemList().begin(), itemList().end(), &item);

    // go one below it (top is front, so we must increment)
    it++;
    iterator it_end = itemList().end();

    // keep going down until we find a visible one
    while (it != it_end && !(*it)->visible())
        it++;

    // if we didn't reach the end, then stack below the
    // item that we found.
    if (it != it_end)
        stackBelowItem(&item, *it);
    // if we did reach the end, then there are no visible windows, so we don't do anything
}
#endif // NOT_USED

void XLayer::raise(XLayerItem &item) {
    // assume it is already in this layer

    if (&item == itemList().front())
        return; // nothing to do

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
    stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));

}

void XLayer::tempRaise(XLayerItem &item) {
    // assume it is already in this layer

    if (&item == itemList().front())
        return; // nothing to do

    iterator it = find(itemList().begin(), itemList().end(), &item);
    if (it == itemList().end()) {
#ifdef DEBUG
        cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: raise on item not in layer["<<m_layernum<<"]"<<endl;
#endif // DEBUG
        return;
    }

    // don't add it back to the top
    stackBelowItem(&item, m_manager.getLowestItemAboveLayer(m_layernum));

}

void XLayer::lower(XLayerItem &item) {
    // assume already in this layer

    // is it already the lowest?
    if (&item == itemList().back())
        return; // nothing to do

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
    stackBelowItem(&item, *it);
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

XLayerItem *XLayer::getItemBelow(XLayerItem &item) {
    // get our iterator
    iterator it = find(itemList().begin(), itemList().end(), &item);

    // go one lower
    it++;

    // if one lower is the end, there is no item below, otherwise we've got it
    if (it == itemList().end())
        return 0;
    else
        return *it;
}

XLayerItem *XLayer::getItemAbove(XLayerItem &item) {
    // get our iterator
    iterator it = find(itemList().begin(), itemList().end(), &item);

    // if this is the beginning (top-most item), do nothing, otherwise give the next one up
    // the list (which must be there since we aren't the beginning)
    if (it == itemList().begin())
        return 0;
    else
        return *(--it);
}
