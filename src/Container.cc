// Container.cc
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Container.hh"

#include "FbTk/Button.hh"
#include "FbTk/EventManager.hh"

Container::Container(const FbTk::FbWindow &parent):
    FbTk::FbWindow(parent, 0, 0, 1, 1, ExposureMask), 
    m_align(RELATIVE),
    m_max_size_per_client(60),
    m_selected(0),
    m_update_lock(false) {
    FbTk::EventManager::instance()->add(*this, *this);
}

Container::~Container() {
    // ~FbWindow cleans event manager
}

void Container::resize(unsigned int width, unsigned int height) {
    // do we need to resize?
    if (FbTk::FbWindow::width() == width &&
        FbTk::FbWindow::height() == height)
        return;

    FbTk::FbWindow::resize(width, height);
    repositionItems();
}

void Container::moveResize(int x, int y,
                           unsigned int width, unsigned int height) {
    FbTk::FbWindow::moveResize(x, y, width, height);
    repositionItems();
}

void Container::insertItems(ItemList &item_list, int pos) {

    // make sure all items have parent == this
    ItemList::iterator it = m_item_list.begin();
    ItemList::iterator it_end = m_item_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->parent() != this)
            return;
    }

    if (pos > size() || pos < 0) {
        // insert last
        m_item_list.splice(m_item_list.end(), item_list);
    } else if (pos == 0) {
        // insert first
        m_item_list.splice(m_item_list.begin(), item_list);
    } else {
        // find insert point
        for (it = m_item_list.begin(); pos != 0; ++it, --pos)
            continue;
        m_item_list.splice(it, item_list);
    }

    m_item_list.unique();

    // update position
    repositionItems();
}

void Container::insertItem(Item item, int pos) {
    if (find(item) != -1)
        return;

    // it must be a child of this window
    if (item->parent() != this)
        return;

    if (pos >= size() || pos < 0) {
        m_item_list.push_back(item);
    } else if (pos == 0) {
        m_item_list.push_front(item);
    } else {
        ItemList::iterator it = m_item_list.begin();
        for (; pos != 0; ++it, --pos)
            continue;

        m_item_list.insert(it, item);
    }

    // make sure we dont have duplicate items
    m_item_list.unique();

    repositionItems();
}

void Container::removeItem(int index) {    
    if (index < 0 || index > size())
        return;

    ItemList::iterator it = m_item_list.begin();
    for (; index != 0; ++it, --index)
        continue;

    if (*it == selected())
        m_selected = 0;

    m_item_list.erase(it);

    repositionItems();
}

void Container::removeAll() {
    m_selected = 0;
    m_item_list.clear();
    if (!m_update_lock) {
        clear();
        updateTransparent();
    }

}

int Container::find(Item item) {
    ItemList::iterator it = m_item_list.begin();
    ItemList::iterator it_end = m_item_list.end();
    int index = 0;
    for (; it != it_end; ++it, ++index) {
        if ((*it) == item)
            break;
    }

    if (it == it_end)
        return -1;

    return index;
}

void Container::setSelected(int pos) {
    if (pos < 0 || pos >= size())
        m_selected = 0;
    else {
        ItemList::iterator it = m_item_list.begin();
        for (; pos != 0; --pos, ++it)
            continue;
        m_selected = *it;
        // caller does any graphics stuff if appropriate
    }
        
}

void Container::setMaxSizePerClient(unsigned int size) {
    m_max_size_per_client = size;
}

void Container::setAlignment(Container::Alignment a) {
    m_align = a;
}

void Container::exposeEvent(XExposeEvent &event) {
    if (!m_update_lock) {
        clearArea(event.x, event.y, event.width, event.height);
        updateTransparent(event.x, event.y, event.width, event.height);
    }
}

void Container::repositionItems() {
    if (empty() || m_update_lock)
        return;

    //!! TODO vertical position

    const int max_width_per_client = maxWidthPerClient();

    ItemList::iterator it = m_item_list.begin();
    const ItemList::iterator it_end = m_item_list.end();
    int borderW = m_item_list.front()->borderWidth();

    int rounding_error = width() - ((maxWidthPerClient() + borderW)* m_item_list.size() - borderW);

    int next_x = -borderW; // zero so the border of the first shows
    int extra = 0;
    int direction = 1;
    if (alignment() == RIGHT) {
        direction = -1;
        next_x = width() - max_width_per_client + borderW;
    }

    for (; it != it_end; ++it, next_x += direction*(max_width_per_client + borderW + extra)) {
        // we only need to do error stuff with alignment RELATIVE
        if (rounding_error != 0 && alignment() == RELATIVE) {
            --rounding_error;
            extra = 1;
        } else {
            extra = 0;
        }
        // resize each clients including border in size
        (*it)->moveResize(next_x,
                          -borderW,
                          max_width_per_client + extra,
                          height());
        // moveresize does a clear
    }

}


unsigned int Container::maxWidthPerClient() const {
    switch (alignment()) {
    case RIGHT:
    case LEFT:
        return m_max_size_per_client;
        break;
    case RELATIVE:
        int count = size();
        if (count == 0)
            return width();
        else {
            int borderW = m_item_list.front()->borderWidth();
            // there're count-1 borders to fit in with the windows
            // -> 1 per window plus end
            return (width() - (count - 1) * borderW) / count;
        }
        break;
    }

    // this will never happen anyway
    return 1;
}
