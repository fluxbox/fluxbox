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

// $Id: Container.hh,v 1.1 2003/08/11 15:28:33 fluxgen Exp $

#ifndef CONTAINER_HH
#define CONTAINER_HH

#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/NotCopyable.hh"

#include <list>

class ContainerTheme;

namespace FbTk {
class Button;
};

class Container:public FbTk::FbWindow, public FbTk::EventHandler, private FbTk::NotCopyable {
public:
    typedef FbTk::Button * Item;
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

    // event handler
    void exposeEvent(XExposeEvent &event);

    // accessors
    int size() const { return m_item_list.size(); }
    const Item selected() const { return m_selected; }
    Item selected() { return m_selected; }
    unsigned int maxWidthPerClient() const { return (size() == 0 ? width() : width()/size()); }
    unsigned int maxHeightPerClient() const { return (size() == 0 ? height() : height()/size()); }
private:
    void repositionItems();

    ItemList m_item_list;
    Item m_selected;
};

#endif // CONTAINER_HH

