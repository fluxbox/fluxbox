// Composer.hh
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_COMPOSE_HH
#define FBTK_COMPOSE_HH

#include <functional>

namespace FbTk {

/// Composes two functions into one.
/// Uses Arg for type B and then calls type A
template <class A, class B>
class Compose_base: public std::unary_function<typename B::argument_type, typename A::result_type> {
public:
    typedef typename A::result_type ResultTypeA;
    typedef typename A::argument_type ArgumentTypeA;
    typedef typename B::result_type ResultTypeB;
    typedef typename B::argument_type ArgumentTypeB;

    Compose_base(const A &a, const B &b):m_a(a), m_b(b) { }
    ResultTypeA operator () (const ArgumentTypeB &arg) const {
        return m_a(m_b(arg));
    }

private:
    A m_a;
    B m_b;
};

// helper that creates a Compose_base
template <typename A, typename B>
inline Compose_base<A, B> 
Compose(const A& a, const B& b) {
    return Compose_base<A, B>(a, b);
}

} // namespace FbTk

#endif // FBTK_COMPOSE_HH
