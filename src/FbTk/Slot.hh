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

namespace FbTk {

/// \namespace Implementation details for signals, do not use anything in this namespace
namespace SigImpl {

class SlotBase {
public:
    virtual ~SlotBase() {}
};

template<typename ReturnType>
class SlotBase0: public SlotBase {
public:
    virtual ReturnType operator()() = 0;
};

template<typename ReturnType, typename Functor>
class Slot0: public SlotBase0<ReturnType> {
public:
    virtual ReturnType operator()() { return m_functor(); }

    Slot0(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

template<typename ReturnType, typename Arg1>
class SlotBase1: public SlotBase {
public:
    virtual ReturnType operator()(Arg1) = 0;
};

template<typename ReturnType, typename Arg1, typename Functor>
class Slot1: public SlotBase1<ReturnType, Arg1> {
public:
    virtual ReturnType operator()(Arg1 arg1) { return m_functor(arg1); }

    Slot1(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

template<typename ReturnType, typename Arg1, typename Arg2>
class SlotBase2: public SlotBase {
public:
    virtual ReturnType operator()(Arg1, Arg2) = 0;
};

template<typename ReturnType, typename Arg1, typename Arg2, typename Functor>
class Slot2: public SlotBase2<ReturnType, Arg1, Arg2> {
public:
    virtual ReturnType operator()(Arg1 arg1, Arg2 arg2) { return m_functor(arg1, arg2); }

    Slot2(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

template<typename ReturnType, typename Arg1, typename Arg2, typename Arg3>
class SlotBase3: public SlotBase {
public:
    virtual ReturnType operator()(Arg1, Arg2, Arg3) = 0;
    virtual ~SlotBase3() {}
};

template<typename ReturnType, typename Arg1, typename Arg2, typename Arg3, typename Functor>
class Slot3: public SlotBase3<ReturnType, Arg1, Arg2, Arg3> {
public:
    virtual ReturnType operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3)
    { return m_functor(arg1, arg2, arg3); }

    Slot3(Functor functor) : m_functor(functor) {}

private:
    Functor m_functor;
};

} // namespace SigImpl

} // namespace FbTk

#endif // FBTK_SLOT_H
