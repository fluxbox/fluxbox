// CompareEqual.hh
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_COMPAREEQUAL_HH
#define FBTK_COMPAREEQUAL_HH

#include <functional>
namespace FbTk {

/// @brief compares one class function with a value type
template <typename ClassType, typename ValueType>
class CompareEqual_base: public std::unary_function<ClassType, bool> {
public:
    typedef ValueType (ClassType::* Action)() const;
    typedef ValueType Value;
    CompareEqual_base(Action a, ValueType v):m_action(a), m_value(v) { }
    bool operator ()(const ClassType *instance) const {
        return (instance->*m_action)() == m_value;
    }
private:
    Action m_action;
    Value m_value;
};

// creates an CompareEqual_base object
template <typename A, typename B>
inline CompareEqual_base<A, B> 
CompareEqual(typename CompareEqual_base<A, B>::Action action, B b) {
    return CompareEqual_base<A, B>(action, b);
}
} // end namespace FbTk

#endif // FBTK_COMPAREEQUAL_HH
