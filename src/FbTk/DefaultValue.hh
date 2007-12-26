// DefaultValue.hh
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

#ifndef FBTK_DEFAULTVALUE_HH
#define FBTK_DEFAULTVALUE_HH

namespace FbTk {

// classes for overriding default values without having to listen for changes
template <typename T>
class DefaultValue {
public:
    DefaultValue(const T &def):
        m_default(def), m_actual(def), m_use_default(true) { }

    inline const T &get() const { return m_use_default ? m_default : m_actual; }
    inline void set(const T &val) { m_use_default = false; m_actual = val; }
    inline void restoreDefault() { m_use_default = true; }
    inline void isDefault() const { return m_use_default; }

    inline DefaultValue<T> &operator =(const T &val) {
        set(val); return *this;
    }

    inline operator T() const { return get(); }

private:
    const T &m_default;
    T m_actual;
    bool m_use_default;
};

// designed for use with built-in types T, thus no need to return references
template <typename T, typename Receiver>
class DefaultAccessor {
public:
    typedef T (Receiver:: *Accessor)() const;
    DefaultAccessor(const Receiver &r, Accessor a):
        m_receiver(r), m_accessor(a), m_actual((r.*a)()),
        m_use_default(true) { }

    inline const T get() const {
        return m_use_default ? (m_receiver.*m_accessor)() : m_actual;
    }
    inline void set(const T &val) { m_use_default = false; m_actual = val; }
    inline void restoreDefault() { m_use_default = true; }
    inline void isDefault() const { return m_use_default; }

    inline DefaultAccessor<T, Receiver> &operator =(const T &val) {
        set(val); return *this;
    }

    inline operator T() const { return get(); }

private:
    const Receiver &m_receiver;
    Accessor &m_accessor;
    T m_actual;
    bool m_use_default;
};

}; // end namespace FbTk

#endif // FBTK_DEFAULTVALUE_HH
