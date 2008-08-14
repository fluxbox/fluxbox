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

#include "Accessor.hh"

namespace FbTk {

// class for overriding default values and restoring them later

// Ret = type of value that gets returned
// Def = type of default value -- may be Accessor<Ret> &, for example
template <typename Ret, typename Def=Ret &>
class DefaultValue: public Accessor<Ret> {
public:
    DefaultValue(const Def def):
        m_default(def), m_actual(def), m_use_default(true) { }

    void restoreDefault() { m_use_default = true; }
    bool isDefault() const { return m_use_default; }

    DefaultValue<Ret, Def> &operator =(const Ret &val) {
        m_use_default = false; m_actual = val; return *this;
    }

    operator Ret() const { return m_use_default ? m_default : m_actual; }

private:
    const Def m_default;
    Ret m_actual;
    bool m_use_default;
};

} // end namespace FbTk

#endif // FBTK_DEFAULTVALUE_HH
