// Container.hh
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

// $Id: Container.hh,v 1.4 2003/12/12 14:35:34 fluxgen Exp $

#ifndef CONTAINER_HH
#define CONTAINER_HH

#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/NotCopyable.hh"

#include <list>

class Container:public FbTk::FbWindow, public FbTk::EventHandler, private FbTk::NotCopyable {
public:
    enum Alignment { LEFT, RELATIVE, RIGHT };
    typedef FbTk::FbWindow * Item;
    typedef std::list<Item> ItemList;

    explicit Container(const FbTk::FbWindow &parent);
    virtual ~Container();
    
    // manipulators

    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void insertItems(ItemList &list, int position=-1);
    void insertItem(Item item, int pos = -1);
    void removeItem(int item);
    void removeAll();
    int find(Item item);
    void setSelected(int index);
    void setMaxSizePerClient(unsigned int size);
    void setAlignment(Alignment a);

    /// force update
    inline void update() { repositionItems(); }
    /// so we can add items without having an graphic update for each item
    inline void setUpdateLock(bool value) { m_update_lock = value; }

    /// event handler
    void exposeEvent(XExposeEvent &event);

    /// accessors
    inline Alignment alignment() const { return m_align; }
    inline int size() const { return m_item_list.size(); }
    inline const Item selected() const { return m_selected; }
    inline Item selected() { return m_selected; }
    unsigned int maxWidthPerClient() const;
    inline unsigned int maxHeightPerClient() const { return (size() == 0 ? height() : height()/size()); }    
    inline bool updateLock() const { return m_update_lock; }

private:
    void repositionItems();

    Alignment m_align;
    unsigned int m_max_size_per_client;
    ItemList m_item_list;
    Item m_selected;
    bool m_update_lock;
};

#endif // CONTAINER_HH

