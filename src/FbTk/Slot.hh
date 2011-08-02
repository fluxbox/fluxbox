// Slot.hh for FbTk, Fluxbox Toolkit
// Copyright (c) 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_SLOT_HH
#define FBTK_SLOT_HH

#include "NotCopyable.hh"

namespace FbTk {

/// \namespace Implementation details for signals, do not use anything in this namespace
namespace SigImpl {

struct EmptyArg {};

/** A base class for all slots. It's purpose is to provide a virtual destructor and to enable the
 * Signal class to hold a pointer to a generic slot.
 */
class SlotBase: private FbTk::NotCopyable {
public:
    virtual ~SlotBase() {}
};

} // namespace SigImpl

/** Declares a pure virtual function call operator with a specific number of arguments (depending
 * on the template specialization). This allows us to "call" any functor in an opaque way.
 */
template<typename ReturnType, typename Arg1 = SigImpl::EmptyArg,
         typename Arg2 = SigImpl::EmptyArg, typename Arg3 = SigImpl::EmptyArg>
class Slot: public SigImpl::SlotBase {
public:
    virtual ReturnType operator()(Arg1, Arg2, Arg3) = 0;
};

/// Specialization for two arguments
template<typename ReturnType, typename Arg1, typename Arg2>
class Slot<ReturnType, Arg1, Arg2, SigImpl::EmptyArg>: public SigImpl::SlotBase {
public:
    virtual ReturnType operator()(Arg1, Arg2) = 0;
};

/// Specialization for one argument
template<typename ReturnType, typename Arg1>
class Slot<ReturnType, Arg1, SigImpl::EmptyArg, SigImpl::EmptyArg>: public SigImpl::SlotBase {
public:
    virtual ReturnType operator()(Arg1) = 0;
};

/// Specialization for no arguments
template<typename ReturnType>
class Slot<ReturnType, SigImpl::EmptyArg, SigImpl::EmptyArg, SigImpl::EmptyArg>: public SigImpl::SlotBase {
public:
    virtual ReturnType operator()() = 0;
};

/** A class which knows how to call a specific functor. It inherits from Slot and implemetents
 * the function call operator
 */
template<typename Functor, typename ReturnType, typename Arg1 = SigImpl::EmptyArg,
         typename Arg2 = SigImpl::EmptyArg, typename Arg3 = SigImpl::EmptyArg>
class SlotImpl: public Slot<ReturnType, Arg1, Arg2, Arg3> {
public:
    virtual ReturnType operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3)
    { return static_cast<ReturnType>(m_functor(arg1, arg2, arg3)); }

    SlotImpl(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

/// Specialization for two arguments
template<typename Functor, typename ReturnType, typename Arg1, typename Arg2>
class SlotImpl<Functor, ReturnType, Arg1, Arg2, SigImpl::EmptyArg>: public Slot<ReturnType, Arg1, Arg2> {
public:
    virtual ReturnType operator()(Arg1 arg1, Arg2 arg2)
    { return static_cast<ReturnType>(m_functor(arg1, arg2)); }

    SlotImpl(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

/// Specialization for one argument
template<typename Functor, typename ReturnType, typename Arg1>
class SlotImpl<Functor, ReturnType, Arg1, SigImpl::EmptyArg, SigImpl::EmptyArg>: public Slot<ReturnType, Arg1> {
public:
    virtual ReturnType operator()(Arg1 arg1) { return static_cast<ReturnType>(m_functor(arg1)); }

    SlotImpl(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

/// Specialization for no arguments
template<typename Functor, typename ReturnType>
class SlotImpl<Functor, ReturnType, SigImpl::EmptyArg, SigImpl::EmptyArg, SigImpl::EmptyArg>: public Slot<ReturnType> {
public:
    virtual ReturnType operator()() { return static_cast<ReturnType>(m_functor()); }

    SlotImpl(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

} // namespace FbTk

#endif // FBTK_SLOT_H
