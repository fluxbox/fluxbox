// Accessor.hh
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef FBTK_ACCESSOR_HH
#define FBTK_ACCESSOR_HH

namespace FbTk {

// base class for objects that act like data type T
template <typename T>
class Accessor {
public:
    virtual Accessor<T> &operator =(const T &val) = 0;
    virtual operator T() const = 0;
    virtual ~Accessor() {}
};

// essentially just a reference
template <typename T>
class SimpleAccessor: public Accessor<T> {
public:
    SimpleAccessor(T &val): m_val(val) { }
    Accessor<T> &operator =(const T &val) { m_val = val; return *this; }
    operator T() const { return m_val; }

private:
    T &m_val;
};

// use object methods as an accessor
template <typename T, typename Receiver>
class ObjectAccessor: public Accessor<T> {
public:
    typedef T (Receiver:: *Getter)() const;
    typedef void (Receiver:: *Setter)(T &);
    ObjectAccessor(Receiver &r, Getter g, Setter s):
        m_receiver(r), m_getter(g), m_setter(s) { }

    operator T() const { return (m_receiver.*m_getter)(); }
    Accessor<T> &operator =(const T &val) {
        (m_receiver.*m_setter)(val); return *this;
    }

private:
    Receiver &m_receiver;
    Getter m_getter;
    Setter m_setter;
};

// same as above but with no set method
template <typename T, typename Receiver>
class ConstObjectAccessor: public Accessor<T> {
public:
    typedef T (Receiver:: *Getter)() const;
    ConstObjectAccessor(const Receiver &r, Getter g):
        m_receiver(r), m_getter(g) { }

    operator T() const { return (m_receiver.*m_getter)(); }
    Accessor<T> &operator =(const T &val) { return *this; }

private:
    const Receiver &m_receiver;
    Getter m_getter;
};

} // end namespace FbTk

#endif // FBTK_ACCESSOR_HH
