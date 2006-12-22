// Layer.hh for FbTk - fluxbox toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_LAYERTEMPLATE_HH
#define FBTK_LAYERTEMPLATE_HH

#include <vector>
#include <algorithm>

namespace FbTk {

template <typename ItemType, typename Container = std::vector<ItemType *> >
class Layer {
public:
    typedef Container ListType;
    typedef typename Container::iterator iterator;
    typedef typename Container::reverse_iterator reverse_iterator;
    virtual ~Layer() { }
    /// insert in top by default
    virtual iterator insert(ItemType &item, unsigned int pos=0);
    /// remove item from list
    virtual void remove(ItemType &item);
#ifdef NOT_USED
    /// cycle all item upwards
    virtual void cycleUp();
    /// cycle all items downwards
    virtual void cycleDown();
#endif // NOT_USED
    /// move item to top
    virtual void raise(ItemType &item);
    /// move item to bottom
    virtual void lower(ItemType &item);
#ifdef NOT_USED
    /// raise a specific item one step
    virtual void stepUp(ItemType &item);
    /// lower a specific item one step
    virtual void stepDown(ItemType &item);
#endif // NOT_USED
    virtual void restack();
    /// @return layer item on specific position, on failure 0
    ItemType *getItem(unsigned int position);
    /// @return number of elements in layer
    size_t size() const { return m_list.size(); }
    /// @return layer list
    const ListType &itemList() const { return m_list; }
    /// @return layer list
    ListType &itemList() { return m_list; }
private:
    ListType m_list;
};

template <typename ItemType, typename Container>
typename Container::iterator Layer<ItemType, Container>::insert(ItemType &item, unsigned int position) {
    // make sure we don't alreay have it in the list
    if (std::find(itemList().begin(), itemList().end(), &item) != itemList().end())
        return m_list.end();

    if (position > size())
        position = size();

    iterator it = m_list.begin();

    for (unsigned int i=0; i<position; ++it, ++i)
        continue;

    m_list.insert(it, &item);
    restack();
    return it++;
}


template <typename ItemType, typename Container>
void Layer<ItemType, Container>::remove(ItemType &item) {
    iterator it = std::find(itemList().begin(), itemList().end(), &item);
    if (it != itemList().end())
        m_list.erase(it);
}

#ifdef NOT_USED
template <typename ItemType, typename Container>
void Layer<ItemType, Container>::cycleUp() {
    if (size() == 0)
        return;
    iterator it = itemList().begin();
    it++;
    rotate(itemList().begin(), it, itemList().end());
    restack();
}


template <typename ItemType, typename Container>
void Layer<ItemType, Container>::cycleDown() {
    if (size() == 0)
        return;
    // save last item and remove it from list
    ItemType *last_item = itemList().back();
    itemList().pop_back();
    // add last item to front
    itemList().insert(itemList().begin(), last_item);
    restack();
}

template <typename ItemType, typename Container>
void Layer<ItemType, Container>::stepUp(ItemType &item) {
    iterator it = 
        find(itemList().begin(), itemList().end(), &item);
             
    if (it == itemList().end())
        return;

    if (it == itemList().begin()) // we can't raise it more
        return;

    iterator new_pos = it;
    new_pos--;
    ItemType *textitem = *it;
    // remove item from list    
    itemList().erase(it);
    // insert above last pos
    itemList().insert(new_pos, textitem);
    restack();
}

template <typename ItemType, typename Container>
void Layer<ItemType, Container>::stepDown(ItemType &item) {
    iterator it = 
        find(itemList().begin(), itemList().end(), &item);
             
    if (it == itemList().end()) // we didn't find the item in our list
        return;

    if (*it == itemList().back()) // it's already at the bottom
        return;

    iterator new_pos = it;
    new_pos++;
    ItemType *textitem = *it;
    // remove from list
    itemList().erase(it); 
    // insert on the new place
    itemList().insert(new_pos, textitem);
    restack();
}
#endif // NOT_USED

template <typename ItemType, typename Container>
void Layer<ItemType, Container>::raise(ItemType &item) {
    if (&item == itemList().front()) // already at the bottom
        return;
    remove(item);
    insert(item, 0);
    restack();
}

template <typename ItemType, typename Container>
void Layer<ItemType, Container>::lower(ItemType &item) {
    if (&item == itemList().back()) // already at the bottom
        return;
    remove(item);
    insert(item, size());
    restack();
}

template <typename ItemType, typename Container>
ItemType *Layer<ItemType, Container>::getItem(unsigned int position) {
    if (position >= m_list.size())
        return 0;
    iterator it = m_list.begin();
    iterator it_end = m_list.end();
    for (unsigned int i=0; i < position && it != it_end; i++);
        
    if (it == it_end) return 0;
    else 
        return *it;
}

template <typename ItemType, typename Container>
void Layer<ItemType, Container>::restack() {
}


} // end namespace FbTk


#endif // FBTK_LAYERTEMPLATE_HH
