// WindowMenuAccessor.hh
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef WINDOW_MENU_ACCESSOR_HH
#define WINDOW_MENU_ACCESSOR_HH

#include "FbTk/Accessor.hh"

#include "FbMenu.hh"

/// accesses values in current window
template <typename Ret, typename Def=Ret>
class WindowMenuAccessor: public FbTk::Accessor<Ret> {
public:
    typedef Ret (FluxboxWindow:: *Getter)() const;
    typedef void (FluxboxWindow:: *Setter)(Ret);
    WindowMenuAccessor(Getter g, Setter s, Def def):
            m_getter(g), m_setter(s), m_def(def) { }

    operator Ret() const {
        FluxboxWindow *fbwin = FbMenu::window();
        if (fbwin)
            return (Ret)(fbwin->*m_getter)();
        return m_def;
    }
    FbTk::Accessor<Ret> &operator =(const Ret &val) {
        FluxboxWindow *fbwin = FbMenu::window();
        if (fbwin)
            (fbwin->*m_setter)(val);
        return *this;
    }

private:
    Getter m_getter;
    Setter m_setter;
    Def m_def;
};

/// same as above but only reads
template <typename Ret, typename Def=Ret>
class ConstWindowMenuAccessor: public FbTk::Accessor<Ret> {
public:
    typedef Ret (FluxboxWindow:: *Getter)() const;
    ConstWindowMenuAccessor(Getter g, Def def):
            m_getter(g), m_def(def) { }

    operator Ret() const {
        FluxboxWindow *fbwin = FbMenu::window();
        return fbwin ? (fbwin->*m_getter)() : m_def;
    }
    FbTk::Accessor<Ret> &operator =(const Ret &val) { return *this; }

private:
    Getter m_getter;
    Def m_def;
};

#endif // WINDOW_MENU_ACCESSOR_HH
