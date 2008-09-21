// Signal.hh for FbTk, Fluxbox Toolkit
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

#ifndef FBTK_SIGNAL_HH
#define FBTK_SIGNAL_HH

#include "Slot.hh"
#include <list>
#include <map>
#include <vector>

namespace FbTk {

/// \namespace Implementation details for signals, do not use anything in this namespace
namespace SigImpl {

/**
 * Parent class for all \c Signal[0...*] classes.
 * It handles the disconnect and holds all the slots. The connect must be
 * handled by the child class so it can do the type checking.
 */
class SignalHolder {
public:
    /// Do not use this type outside this class
    typedef std::list<SlotHolder> SlotList;

    typedef SlotList::iterator Iterator;
    typedef Iterator SlotID;
    typedef SlotList::const_iterator ConstIterator;

    virtual ~SignalHolder() { }

    /// Remove a specific slot \c id from this signal
    void disconnect(SlotID slotIt) {
        m_slots.erase( slotIt );
    }


    /// Removes all slots connected to this
    void clear() {
        m_slots.clear();
    }

protected:
    ConstIterator begin() const { return m_slots.begin(); }
    ConstIterator end() const { return m_slots.end(); }

    Iterator begin() { return m_slots.begin(); }
    Iterator end() { return m_slots.end(); }

    /// Connect a slot to this signal. Must only be called by child classes.
    SlotID connect(const SlotHolder& slot) {
        return m_slots.insert(m_slots.end(), slot);
    }

private:
    SlotList m_slots; ///< all slots connected to a signal
};

/// Signal with no argument
template <typename ReturnType>
class Signal0: public SignalHolder {
public:
    typedef Slot0<ReturnType> SlotType;

    virtual ~Signal0() { }

    void emit() {
        for ( Iterator it = begin(); it != end(); ++it ) {
            static_cast<SlotType&>(*it)();
        }
    }

    SlotID connect(const SlotType& slot) {
        return SignalHolder::connect(slot);

    }
};

/// Signal with one argument
template <typename ReturnType, typename Arg1>
class Signal1: public SignalHolder {
public:
    typedef Slot1<ReturnType, Arg1> SlotType;

    virtual ~Signal1() { }

    void emit(Arg1 arg) {
        for ( Iterator it = begin(); it != end(); ++it ) {
            static_cast<SlotType&>(*it)(arg);
        }
    }

    SlotID connect(const SlotType& slot) {
        return SignalHolder::connect(slot);
    }

};

/// Signal with two arguments
template <typename ReturnType, typename Arg1, typename Arg2>
class Signal2: public SignalHolder {
public:
    typedef Slot2<ReturnType, Arg1, Arg2> SlotType;

    virtual ~Signal2() { }

    void emit(Arg1 arg1, Arg2 arg2) {
        for ( Iterator it = begin(); it != end(); ++it ) {
            static_cast<SlotType&>(*it)(arg1, arg2);
        }
    }

    SlotID connect(const SlotType& slot) {
        return SignalHolder::connect(slot);
    }
};

/// Signal with three arguments
template <typename ReturnType, typename Arg1, typename Arg2, typename Arg3>
class Signal3: public SignalHolder {
public:
    typedef Slot3<ReturnType, Arg1, Arg2, Arg3> SlotType;

    virtual ~Signal3() { }

    void emit(Arg1 arg1, Arg2 arg2, Arg3 arg3) {
        for ( Iterator it = begin(); it != end(); ++it ) {
            static_cast<SlotType&>(*it)(arg1, arg2, arg3);
        }
    }

    SlotID connect(const SlotType& slot) {
        return SignalHolder::connect(slot);
    }

};

struct EmptyArg {};

} // namespace SigImpl


/// Specialization for three arguments.
template <typename ReturnType,
          typename Arg1 = SigImpl::EmptyArg, typename Arg2 = SigImpl::EmptyArg, typename Arg3 = SigImpl::EmptyArg >
class Signal: public SigImpl::Signal3< ReturnType, Arg1, Arg2, Arg3 > {
public:
};

/// Specialization for two arguments.
template <typename ReturnType, typename Arg1, typename Arg2>
class Signal<ReturnType, Arg1, Arg2, SigImpl::EmptyArg>: public SigImpl::Signal2< ReturnType, Arg1, Arg2 > {
public:
};

/// Specialization for one argument.
template <typename ReturnType, typename Arg1>
class Signal<ReturnType, Arg1, SigImpl::EmptyArg, SigImpl::EmptyArg>: public SigImpl::Signal1< ReturnType, Arg1 > {
public:
};

/// Specialization for no argument.
template <typename ReturnType>
class Signal<ReturnType, SigImpl::EmptyArg, SigImpl::EmptyArg, SigImpl::EmptyArg>: public SigImpl::Signal0< ReturnType > {
public:
};

/**
 * Tracks a signal during it's life time. All signals connected using \c
 * SignalTracker::join will be erased when this instance dies.
 */
class SignalTracker {
public:
    /// Internal type, do not use.
    typedef std::map<SigImpl::SignalHolder*, SigImpl::SignalHolder::SlotID> Connections;
    typedef Connections::iterator TrackID; ///< \c ID type for join/leave.

    virtual ~SignalTracker() {
        // disconnect all connections
        for ( Connections::iterator conIt = m_connections.begin();
              conIt != m_connections.end(); ++conIt)
            conIt->first->disconnect( conIt->second );
        m_connections.clear();
    }

    /// Starts tracking a signal.
    /// @return A tracking ID ( not unique )
    template <typename Signal, typename Functor>
    TrackID join(Signal& sig, const Functor& functor) {
        return 
            m_connections.insert(m_connections.end(), 
                                 Connections::value_type(&sig, sig.connect(functor)));
    }

    /// Leave tracking for a signal
    /// @param id the \c id from the previous \c join 
    void leave(TrackID id) {
        m_connections.erase(id);
    }

    /// Leave tracking for a signal
    /// @param sig the signal to leave
    template <typename Signal>
    void leave(Signal &sig) {
        m_connections.erase(&sig);
    }

private:
    /// holds all connections to different signals and slots.
    Connections m_connections;
};


} // namespace FbTk

#endif // FBTK_SIGNAL_HH
