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

class CallbackHolder;

/// Placeholder type for typed callbacks
typedef void* (*CallbackFunc)(void *);
/// Clone function callback type for cloning typed callback holders
typedef CallbackHolder* (*CloneFunc)(CallbackHolder*);
/// Kill function callback type for destroying type specific information in
/// FunctorHolder
typedef void (*KillFunc)(CallbackHolder*);

/// Holds clone, functor callback, and the kill function for FunctorHolder.
class CallbackHolder {
public:
    /**
     * @param callback The callback to call when a slot receives a signal.
     * @param clone The callback to use for cloning a type specific instance of
     *              this classinstance.
     * @param kill The callback that knows how to free the memory in type
     *             specific instance of this class.
     */
    CallbackHolder(CallbackFunc callback, 
                   CloneFunc clone,
                   KillFunc kill):
        m_callback(callback),
        m_kill(kill),
        m_clone(clone) { }

    ~CallbackHolder() {
        (*m_kill)(this);
    }

    /// @return a clone of this instance
    CallbackHolder* clone() {
        return (*m_clone)(this);
    }

    /// \c Callback to \c Functor specific callback
    CallbackFunc m_callback;
    
protected:

    CallbackHolder& operator = (const CallbackHolder& other) {
        if ( this == &other ) {
            return *this;
        }
        m_callback = other.m_callback;
        m_clone = other.m_clone;
        m_kill = other.m_kill;

        return *this;
    }

    CallbackHolder(const CallbackHolder& other) {
        *this = other;
    }

private:
    /// This function is called to kill this instance
    KillFunc m_kill;
    /// Functions that knows how to clone a specific \c Functor type
    CloneFunc m_clone;
};


/// Holds the functor and creates a clone callback for \c Functor specific type
template <typename Functor>
class FunctorHolder: public CallbackHolder {
public:
    /// This type.
    typedef FunctorHolder<Functor> Self;
    /**
     * @param functor The functor to be used when a signal is emitted.
     * @param callback The callback to call when a signal is emitted.
     */
    FunctorHolder(const Functor& functor, CallbackFunc callback):
        CallbackHolder(callback, &clone, &kill),
        m_functor(functor) {
    }

    /// Specific clone for this Functor type
    static CallbackHolder* clone(CallbackHolder* self) {
        return new Self( static_cast<Self&>(*self));
    }

    static void kill(CallbackHolder* self) {
        // Destroy functor
        static_cast<Self*>( self )->m_functor.~Functor();
    }

    Functor m_functor; ///< the functor to use when a signal is emitted.
};



/// Callback with no arguments.
template <typename Functor, typename ReturnType >
struct Callback0 {
    static ReturnType callback(CallbackHolder* base) {
        static_cast< FunctorHolder<Functor>* >( base )->m_functor();
        return ReturnType();
    }

    static CallbackFunc functionAddress() { 
        return reinterpret_cast<CallbackFunc>(&callback);
    }
};

/// Callback with one argument
template <typename Functor, typename ReturnType, typename Arg1>
struct Callback1 {
    typedef ReturnType (Functor::* CallbackType)(CallbackHolder*, Arg1);

    static ReturnType callback(CallbackHolder* base, Arg1 arg1) {
        static_cast< FunctorHolder<Functor>* >( base )->m_functor(arg1);
        return ReturnType();
    }

    static CallbackFunc functionAddress() { 
        return reinterpret_cast<CallbackFunc>(&callback);
    }
};

/// Callback with two arguments
template <typename Functor, typename ReturnType,
          typename Arg1, typename Arg2>
struct Callback2 {
    typedef ReturnType (Functor::* CallbackType)(CallbackHolder*, Arg1, Arg2);

    static ReturnType callback(CallbackHolder* base, Arg1 arg1, Arg2 arg2) {
        static_cast< FunctorHolder<Functor>* >( base )->m_functor(arg1, arg2);
        return ReturnType();
    }

    static CallbackFunc functionAddress() { 
        return reinterpret_cast<CallbackFunc>(&callback);
    }
};

/// Callback with three arguments
template <typename Functor, typename ReturnType,
          typename Arg1, typename Arg2, typename Arg3>
struct Callback3 {
    typedef ReturnType (Functor::* CallbackType)(CallbackHolder*, Arg1, Arg2, Arg3);

    static ReturnType callback(CallbackHolder* base, Arg1 arg1, Arg2 arg2, Arg3 arg3) {
        static_cast< FunctorHolder<Functor>* >( base )->m_functor( arg1, arg2, arg3 );
        return ReturnType();
    }

    static CallbackFunc functionAddress() { 
        return reinterpret_cast<CallbackFunc>(&callback);
    }
};

/// Holds callback holder and handles the copying of callback holders for the
/// \c Slots.
class SlotHolder {
public:
    SlotHolder(const SlotHolder& other):
        m_holder( other.m_holder ? other.m_holder->clone() : 0 ) {
    }

    ~SlotHolder() {
        delete m_holder;
    }

    SlotHolder& operator = (const SlotHolder& other) {
        if ( &other == this ) {
            return *this;
        }
        delete m_holder;
        if ( other.m_holder ) {
            m_holder = other.m_holder->clone();
        } else {
            m_holder = 0;
        }
        return *this;
    }

    SlotHolder():m_holder( 0 ) { }

protected:
    explicit SlotHolder(CallbackHolder* holder):
        m_holder( holder ) {
    }

    CallbackHolder* m_holder;
};

/// Slot with no argument.
template <typename ReturnType>
class Slot0: public SlotHolder {
public:
    typedef ReturnType (*CallbackType)(CallbackHolder*);

    template <typename Functor>
    Slot0( const Functor& functor ):
        SlotHolder( new FunctorHolder<Functor>
                    (functor, Callback0<Functor, ReturnType>::functionAddress())) {
    }

    void operator()() {
        if (m_holder)
            reinterpret_cast<CallbackType>(m_holder->m_callback)( m_holder );
    }
};

/// Slot with one argument.
template <typename ReturnType, typename Arg1>
class Slot1:public SlotHolder {
public:
    typedef ReturnType (*CallbackType)(CallbackHolder*, Arg1);

    template <typename Functor>
    Slot1( const Functor& functor ):
        SlotHolder( new FunctorHolder<Functor>
                    (functor, Callback1<Functor, ReturnType, Arg1>::functionAddress())){
        
    }

    void operator()(Arg1 arg) {
        if (m_holder)
            reinterpret_cast<CallbackType>(m_holder->m_callback)(m_holder, arg);
    }

};

/// Slot with two arguments
template <typename ReturnType, typename Arg1, typename Arg2>
class Slot2: public SlotHolder {
public:
    typedef ReturnType (*CallbackType)(CallbackHolder*, Arg1, Arg2);
    template <typename Functor>
    Slot2( const Functor& functor ):
        SlotHolder( new FunctorHolder<Functor>
                    (functor, Callback2<Functor, ReturnType, Arg1, Arg2>::functionAddress())){
        
    }

    void operator()(Arg1 arg1, Arg2 arg2) {
        if (m_holder)
            reinterpret_cast<CallbackType>(m_holder->m_callback)(m_holder, arg1, arg2);
    }
};

/// Slot with three arguments
template <typename ReturnType, typename Arg1, typename Arg2, typename Arg3>
class Slot3: public SlotHolder {
public:
    typedef ReturnType (*CallbackType)(CallbackHolder*, Arg1, Arg2, Arg3);
    template <typename Functor>
    Slot3( const Functor& functor ):
        SlotHolder( new FunctorHolder<Functor>
                    (functor, Callback3<Functor, ReturnType, Arg1, Arg2, Arg3>::functionAddress())){
        
    }

    void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3) {
        if (m_holder)
            reinterpret_cast<CallbackType>(m_holder->m_callback)
                ( m_holder, arg1, arg2, arg3 );
    }
};

} // namespace SigImpl

} // namespace FbTk

#endif // FBTK_SLOT_H
