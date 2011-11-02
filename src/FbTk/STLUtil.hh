// STLUtil.cc for FbTk
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef FBTK_STLUTIL_HH
#define FBTK_STLUTIL_HH

/// contains useful utilities for STL 
namespace FbTk {
namespace STLUtil {

template<bool C, typename Ta, typename Tb>
struct IfThenElse;

template<typename Ta, typename Tb>
struct IfThenElse<true, Ta, Tb> {
    Ta& operator ()(Ta& ta, Tb& tb) const {
        return ta;
    }
    typedef Ta ResultType;
};

template<typename Ta, typename Tb>
struct IfThenElse<false, Ta, Tb> {
    Tb& operator ()(Ta& ta, Tb& tb) const {
        return tb;
    }
    typedef Tb ResultType;
};

/// calls delete on each item in the container and then clears the container
template <typename A>
void destroyAndClear(A &a) {
    typedef typename A::iterator iterator;
    iterator it = a.begin();
    iterator it_end = a.end();
    for (; it != it_end; ++it)
        delete (*it);

    a.clear();
}

/// calls delete on each item value in the map and then clears the map
template <typename A>
void destroyAndClearSecond(A &a) {
    typedef typename A::iterator iterator;
    iterator it = a.begin();
    iterator it_end = a.end();
    for (; it != it_end; ++it)
        delete it->second;
    a.clear();
}


template <typename C, typename F>
F forAll(C& c, F f) {
    typedef typename C::iterator iterator;
    iterator i = c.begin();
    iterator e = c.end();
    for (; i != e; i++) {
        f(*i);
    }
    return f;
}

template <typename C, typename I, typename F>
F forAllIf(C& c, I i, F f) {
    typedef typename C::iterator iterator;
    iterator it = c.begin();
    iterator end = c.end();
    for (; it != end; ++it) {
        if (i(*it))
            f(*it);
    }
    return f;
}




} // end namespace STLUtil
} // end namespace FbTk

#endif // STLUTIL_Hh
