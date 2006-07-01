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

// $Id$

#ifndef CONTAINER_HH
#define CONTAINER_HH

#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/NotCopyable.hh"
#include "FbTk/Text.hh" // for Orientation

namespace FbTk {
    class Button;
}

#include <list>
#include <functional>

class Container:public FbTk::FbWindow, public FbTk::EventHandler, private FbTk::NotCopyable {
public:
    // LEFT, RIGHT => fixed total width, fixed icon size
    // RELATIVE => fixed total width, relative/variable icon size
    enum Alignment { LEFT, RELATIVE, RIGHT };
    typedef FbTk::Button * Item;
    typedef const FbTk::Button * ConstItem;
    typedef std::list<Item> ItemList;

    explicit Container(const FbTk::FbWindow &parent);
    virtual ~Container();
    
    // manipulators

    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

#ifdef NOT_USED
    void insertItems(ItemList &list, int position=-1);
#endif
    void insertItem(Item item, int pos = -1);
    bool removeItem(int item); // return true if something was removed
    bool removeItem(Item item); // return true if something was removed
    void removeAll();
    void moveItem(Item item, int movement); // wraps around
    bool moveItemTo(Item item, int x, int y);
    int find(ConstItem item);
    void setSelected(int index);
    void setMaxSizePerClient(unsigned int size);
    void setMaxTotalSize(unsigned int size);
    void setAlignment(Alignment a);
    void setOrientation(FbTk::Orientation orient);

    Item back() { return m_item_list.back(); }

    /// force update
    inline void update() { repositionItems(); }
    /// so we can add items without having an graphic update for each item
    inline void setUpdateLock(bool value) { m_update_lock = value; }

    /// event handler
    void exposeEvent(XExposeEvent &event);
    // for use when embedded in something that may passthrough
    bool tryExposeEvent(XExposeEvent &event);
    bool tryButtonPressEvent(XButtonEvent &event);
    bool tryButtonReleaseEvent(XButtonEvent &event);

    void parentMoved();
    void invalidateBackground();

    /// accessors
    inline Alignment alignment() const { return m_align; }
    inline FbTk::Orientation orientation() const { return m_orientation; }
    inline int size() const { return m_item_list.size(); }
    inline bool empty() const { return m_item_list.empty(); }
    inline const Item& selected() const { return m_selected; }
    inline Item selected() { return m_selected; }
    unsigned int maxWidthPerClient() const;
    inline bool updateLock() const { return m_update_lock; }

    void for_each(std::mem_fun_t<void, FbTk::FbWindow> function);
    void setAlpha(unsigned char alpha); // set alpha on all windows

    ItemList::iterator begin() { return m_item_list.begin(); }
    ItemList::iterator end() { return m_item_list.end(); }

    void clear(); // clear all windows

private:
    void repositionItems();

    FbTk::Orientation m_orientation;

    Alignment m_align;
    unsigned int m_max_size_per_client;
    unsigned int m_max_total_size;
    ItemList m_item_list;
    Item m_selected;
    bool m_update_lock;
};

#endif // CONTAINER_HH

