// RefCount.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef FBTK_REFCOUNT_HH
#define FBTK_REFCOUNT_HH

namespace FbTk {

/// holds a pointer with reference counting, similar to std:auto_ptr
template <typename Pointer>
class RefCount {
public:
    RefCount();
    explicit RefCount(Pointer *p);
    explicit RefCount(RefCount<Pointer> &copy);
    RefCount(const RefCount<Pointer> &copy);
    ~RefCount();
    RefCount<Pointer> &operator = (const RefCount<Pointer> &copy);
    RefCount<Pointer> &operator = (Pointer *p);
    Pointer *operator * () const { return get(); } 
    Pointer *operator -> () const { return get(); }
    Pointer *get() const { return m_data; }
#ifdef NOT_USED
    /// @return number of references
    unsigned int usedBy() const { return (m_refcount != 0 ? *m_refcount : 0); }
#endif
private:
    /// increase reference count
    void incRefCount();
    /// decrease reference count
    void decRefCount();
    Pointer *m_data; ///< data holder
    mutable unsigned int *m_refcount; ///< holds reference counting
};

// implementation

template <typename Pointer>
RefCount<Pointer>::RefCount():m_data(0), m_refcount(new unsigned int(0)) {
    incRefCount(); // it really counts how many things are storing m_refcount
}

template <typename Pointer>
RefCount<Pointer>::RefCount(RefCount<Pointer> &copy):
    m_data(copy.m_data),
    m_refcount(copy.m_refcount) {
    incRefCount();
}

template <typename Pointer>
RefCount<Pointer>::RefCount(Pointer *p):m_data(p), m_refcount(new unsigned int(0)) {
    incRefCount();
}

template <typename Pointer>
RefCount<Pointer>::RefCount(const RefCount<Pointer> &copy):
    m_data(copy.m_data),
    m_refcount(copy.m_refcount) {
    incRefCount();
}

template <typename Pointer>
RefCount<Pointer>::~RefCount() {
    decRefCount();
}

template <typename Pointer>
RefCount<Pointer> &RefCount<Pointer>::operator = (const RefCount<Pointer> &copy) {
    decRefCount(); // dec current ref count
    m_refcount = copy.m_refcount; // set new ref count
    m_data = copy.m_data; // set new data pointer
    incRefCount(); // inc new ref count 
    return *this;
}

template <typename Pointer>
RefCount<Pointer> &RefCount<Pointer>::operator = (Pointer *p) {
    decRefCount();
    m_data = p; // set data pointer
    m_refcount = new unsigned int(0); // create new counter
    incRefCount();
    return *this;
}

template <typename Pointer>
void RefCount<Pointer>::decRefCount() {
    if (m_refcount == 0)
        return;
    if (*m_refcount == 0) { // already zero, then delete refcount
        delete m_refcount;
        m_refcount = 0;
        return;
    }
    (*m_refcount)--;
    if (*m_refcount == 0) { // destroy m_data and m_refcount if nobody else is using this
        if (m_data != 0)
            delete m_data;
        m_data = 0;
        delete m_refcount;
        m_refcount = 0;
    }
}

template <typename Pointer>
void RefCount<Pointer>::incRefCount() {
    if (m_refcount == 0)
        return;
    (*m_refcount)++;
}

} // end namespace FbTk

#endif // FBTK_REFCOUNT_HH
