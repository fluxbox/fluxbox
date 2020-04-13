// Container.hh
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

#ifndef FBTK_CONTAINER_HH
#define FBTK_CONTAINER_HH

#include "FbWindow.hh"
#include "EventHandler.hh"
#include "NotCopyable.hh"
#include "Orientation.hh"

#include <list>
#include <functional>

namespace FbTk {

class Button;

class Container: public FbWindow, public EventHandler, private NotCopyable {
public:
    // LEFT, RIGHT => fixed total width, fixed icon size
    // RELATIVE => fixed total width, relative/variable icon size
    enum Alignment { LEFT, CENTER, RIGHT, RELATIVE, RELATIVE_SMART };
    typedef Button * Item;
    typedef const Button * ConstItem;
    typedef std::list<Item> ItemList;

    explicit Container(const FbWindow &parent, bool auto_resize = true);
    virtual ~Container();

    // manipulators

    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void insertItem(Item item, int pos = -1);
    bool removeItem(int item); // return true if something was removed
    bool removeItem(Item item); // return true if something was removed
    void removeAll();
    void moveItem(Item item, int movement); // wraps around
    bool moveItemTo(Item item, int x, int y);
    int find(ConstItem item);
    void setMaxSizePerClient(unsigned int size);
    void setMaxTotalSize(unsigned int size);
    void setAlignment(Alignment a);
    void setOrientation(Orientation orient);

    Item back() { return m_item_list.back(); }

    /// force update
    void update() { repositionItems(); }
    /// so we can add items without having an graphic update for each item
    void setUpdateLock(bool value) { m_update_lock = value; }

    /// event handler
    void exposeEvent(XExposeEvent &event);
    // for use when embedded in something that may passthrough
    bool tryExposeEvent(XExposeEvent &event);
    bool tryButtonPressEvent(XButtonEvent &event);
    bool tryButtonReleaseEvent(XButtonEvent &event);

    void parentMoved();
    void invalidateBackground();

    /// accessors
    Alignment alignment() const { return m_align; }
    Orientation orientation() const { return m_orientation; }
    int size() const { return m_item_list.size(); }
    bool empty() const { return m_item_list.empty(); }
    unsigned int maxWidthPerClient() const;
    bool updateLock() const { return m_update_lock; }

    void for_each(std::mem_fun_t<void, FbWindow> function);
    void setAlpha(int alpha); // set alpha on all windows

    ItemList::iterator begin() { return m_item_list.begin(); }
    ItemList::iterator end() { return m_item_list.end(); }

    void clear(); // clear all windows

    void repositionItems();
private:

    Orientation m_orientation;

    Alignment m_align;
    unsigned int m_max_size_per_client;
    unsigned int m_max_total_size;
    ItemList m_item_list;
    bool m_update_lock, m_auto_resize;
};

} // end namespace FbTk

#endif // FBTK_CONTAINER_HH
