// Container.cc
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

// $Id: Container.cc,v 1.4 2003/09/15 20:13:24 fluxgen Exp $

#include "FbTk/Button.hh"
#include "Container.hh"

#include "FbTk/EventManager.hh"

Container::Container(const FbTk::FbWindow &parent):
    FbTk::FbWindow(parent, 0, 0, 1, 1, ExposureMask), m_selected(0),
    m_update_lock(false) {

    FbTk::EventManager::instance()->add(*this, *this);
}

Container::~Container() {

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
    clear();

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
        if (m_selected)
            m_selected->clear();
    }
        
}

void Container::exposeEvent(XExposeEvent &event) {
    clearArea(event.x, event.y, event.width, event.height);
}

void Container::repositionItems() {
    if (size() == 0 || m_update_lock)
        return;

    //!! TODO vertical position

    const int max_width_per_client = maxWidthPerClient();

    ItemList::iterator it = m_item_list.begin();
    const ItemList::iterator it_end = m_item_list.end();
    int next_x = 0;
    for (; it != it_end; ++it, next_x += max_width_per_client) {
        // resize each clients including border in size
        (*it)->moveResize(next_x - (*it)->borderWidth(),
                          -(*it)->borderWidth(), 
                          max_width_per_client - (*it)->borderWidth(),
                          height() + (*it)->borderWidth());
        (*it)->clear();
    }
}


unsigned int Container::maxWidthPerClient() const {
    return (size() == 0 ? width() : (width() + size()*m_item_list.front()->borderWidth())/size()); 
}
