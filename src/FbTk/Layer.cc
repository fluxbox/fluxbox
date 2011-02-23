// Layer.cc for FbTk - fluxbox toolkit
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

#include "Layer.hh"
#include "LayerItem.hh"
#include "App.hh"
#include "FbWindow.hh"
#include "MultLayers.hh"

#include <iostream>
#include <algorithm>
#include <numeric>

using namespace FbTk;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

namespace {

int sum_windows(int nr, LayerItem* item) {
    return nr + item->numWindows();
}

int count_windows(const FbTk::Layer::ItemList& items) {
    return std::accumulate(items.begin(), items.end(), 0, sum_windows);
}


void extract_windows_to_stack(const LayerItem::Windows& windows, std::vector<Window>& stack) {
    LayerItem::Windows::const_iterator i = windows.begin();
    LayerItem::Windows::const_iterator end = windows.end();
    for (; i != end; ++i) {
        Window w = (*i)->window();
        if (w)
            stack.push_back(w);
    }
}

void extract_windows_to_stack(const FbTk::Layer::ItemList& items, LayerItem* temp_raised, std::vector<Window>& stack) {

    if (temp_raised) { // add windows that go on top
        extract_windows_to_stack(temp_raised->getWindows(), stack);
    }

    FbTk::Layer::ItemList::const_iterator it = items.begin();
    FbTk::Layer::ItemList::const_iterator it_end = items.end();
    for (; it != it_end; ++it) { // add all the windows from each other item
        if (*it == temp_raised) {
            continue;
        }
        extract_windows_to_stack((*it)->getWindows(), stack);
    }
}

void restack(const FbTk::Layer::ItemList& items, LayerItem* temp_raised) {

    std::vector<Window> stack;
    extract_windows_to_stack(items, temp_raised, stack);

    if (!stack.empty())
        XRestackWindows(FbTk::App::instance()->display(), &stack[0], stack.size());
}

} // end of anonymous namespace


void Layer::restack(const std::vector<Layer*>& layers) {

    std::vector<Window> stack;
    std::vector<Layer*>::const_iterator l;
    for (l = layers.begin(); l != layers.end(); ++l) {
        extract_windows_to_stack((*l)->itemList(), 0, stack);
    }

    if (!stack.empty())
        XRestackWindows(FbTk::App::instance()->display(), &stack[0], stack.size());
}

Layer::Layer(MultLayers &manager, int layernum):
    m_manager(manager), m_layernum(layernum), m_needs_restack(false) {
}

Layer::~Layer() {

}

void Layer::restack() {
    if (m_manager.isUpdatable()) {
        ::restack(itemList(), 0);
        m_needs_restack = false;
    }
}

void Layer::restackAndTempRaise(LayerItem &item) {
    ::restack(itemList(), &item);
}

int Layer::countWindows() {
    return ::count_windows(itemList());
}


// Stack all windows associated with 'item' below the 'above' item
void Layer::stackBelowItem(LayerItem &item, LayerItem *above) {
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

    std::vector<Window> stack;

    // We do have a window to stack below
    // so we put it on top, and fill the rest of the array with the ones to go below it.
    // assume that above's window exists
    stack.push_back(above->getWindows().back()->window());

    // fill the rest of the array
    extract_windows_to_stack(item.getWindows(), stack);

    XRestackWindows(FbTk::App::instance()->display(), &stack[0], stack.size());
}

// We can't just use Restack here, because it won't do anything if they're
// already in the same relative order excluding other windows
void Layer::alignItem(LayerItem &item) {
    if (itemList().front() == &item) {
        stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));
        return;
    }

    // Note: some other things effectively assume that the window list is
    // sorted from highest to lowest
    // get our item
    iterator myit = std::find(itemList().begin(), itemList().end(), &item);
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

Layer::iterator Layer::insert(LayerItem &item, unsigned int pos) {
#ifdef DEBUG
    // at this point we don't support insertions into a layer other than at the top
    if (pos != 0)
        cerr<<__FILE__<<"("<<__LINE__<<"): Insert using non-zero position not valid in Layer"<<endl;
#endif // DEBUG

    itemList().push_front(&item);
    // restack below next window up
    stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum));
    return itemList().begin();
}

void Layer::remove(LayerItem &item) {
    iterator it = itemList().begin();
    iterator it_end = itemList().end();
    for (; it != it_end; ++it) {
        if (*it == &item) {
            itemList().erase(it);
            break;
        }
    }
}

void Layer::raise(LayerItem &item) {
    // assume it is already in this layer

    if (&item == itemList().front()) {
        if (m_needs_restack)
            restack();
        return; // nothing to do
    }


    iterator it = std::find(itemList().begin(), itemList().end(), &item);
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

void Layer::tempRaise(LayerItem &item) {
    // assume it is already in this layer

    if (!m_needs_restack && &item == itemList().front())
        return; // nothing to do

    iterator it = std::find(itemList().begin(), itemList().end(), &item);
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

void Layer::lower(LayerItem &item) {
    // assume already in this layer

    // is it already the lowest?
    if (&item == itemList().back()) {
        if (m_needs_restack)
            restack();
        return; // nothing to do
    }

    iterator it = std::find(itemList().begin(), itemList().end(), &item);
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

void Layer::raiseLayer(LayerItem &item) {
    m_manager.raiseLayer(item);
}

void Layer::lowerLayer(LayerItem &item) {
    m_manager.lowerLayer(item);
}

void Layer::moveToLayer(LayerItem &item, int layernum) {
    m_manager.moveToLayer(item, layernum);
}


LayerItem *Layer::getLowestItem() {
    if (itemList().empty())
        return 0;
    else
        return itemList().back();
}

