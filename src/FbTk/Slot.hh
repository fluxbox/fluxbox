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

#include "RefCount.hh"

namespace FbTk {

/// \namespace Implementation details for signals, do not use anything in this namespace
namespace SigImpl {

struct EmptyArg {};

class SlotBase {
public:
    virtual ~SlotBase() {}
};

template<typename Arg1, typename Arg2, typename Arg3>
class SlotTemplate: public SlotBase {
public:
    virtual void operator()(Arg1, Arg2, Arg3) = 0;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Functor>
class Slot: public SlotTemplate<Arg1, Arg2, Arg3> {
public:
    virtual void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3)
    { m_functor(arg1, arg2, arg3); }

    Slot(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

// specialization for two arguments
template<typename Arg1, typename Arg2, typename Functor>
class Slot<Arg1, Arg2, EmptyArg, Functor>: public SlotTemplate<Arg1, Arg2, EmptyArg> {
public:
    virtual void operator()(Arg1 arg1, Arg2 arg2, EmptyArg)
    { m_functor(arg1, arg2); }

    Slot(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

// specialization for one argument
template<typename Arg1, typename Functor>
class Slot<Arg1, EmptyArg, EmptyArg, Functor>: public SlotTemplate<Arg1, EmptyArg, EmptyArg> {
public:
    virtual void operator()(Arg1 arg1, EmptyArg, EmptyArg)
    { m_functor(arg1); }

    Slot(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

// specialization for no arguments
template<typename Functor>
class Slot<EmptyArg, EmptyArg, EmptyArg, Functor>: public SlotTemplate<EmptyArg, EmptyArg, EmptyArg> {
public:
    virtual void operator()(EmptyArg, EmptyArg, EmptyArg)
    { m_functor(); }

    Slot(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

} // namespace SigImpl

} // namespace FbTk


#endif // FBTK_SLOT_H
