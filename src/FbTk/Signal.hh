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

#include "RefCount.hh"
#include "Slot.hh"
#include <algorithm>
#include <list>
#include <map>
#include <set>

namespace FbTk {

/// \namespace Implementation details for signals, do not use anything in this namespace
namespace SigImpl {

/**
 * Parent class for all \c Signal template classes.
 * It handles the disconnect and holds all the slots. The connect must be
 * handled by the child class so it can do the type checking.
 */
class SignalHolder {
protected:
    typedef RefCount<SlotBase> SlotPtr;
    typedef std::list<SlotPtr> SlotList;

public:
    /// Special tracker interface used by SignalTracker.
    class Tracker {
    public:
        virtual ~Tracker() { }
        /// Disconnect this holder.
        virtual void disconnect(SignalHolder& signal) = 0;
    };

    typedef SlotList::iterator Iterator;
    typedef Iterator SlotID;
    typedef SlotList::const_iterator ConstIterator;

    SignalHolder() : m_emitting(0) {}

    ~SignalHolder() {
        // Disconnect this holder from all trackers.
        for (Trackers::iterator it = m_trackers.begin(),
                 it_end = m_trackers.end();
             it != it_end; ++it ) {
            (*it)->disconnect(*this);
        }
    }

    /// Remove a specific slot \c id from this signal
    void disconnect(SlotID slotIt) const {
        if(m_emitting) {
            // if we are emitting, we must not erase the actual element, as that would
            // invalidate iterators in the emit() function
            *slotIt = SlotPtr();
        } else
            m_slots.erase( slotIt );
    }


    /// Removes all slots connected to this
    void clear() {
        if(m_emitting)
            std::fill(m_slots.begin(), m_slots.end(), SlotPtr());
        else
            m_slots.clear();
    }

    void connectTracker(SignalHolder::Tracker& tracker) const {
        m_trackers.insert(&tracker);
    }

    void disconnectTracker(SignalHolder::Tracker& tracker) const {
        m_trackers.erase(&tracker);
    }

protected:
    ConstIterator begin() const { return m_slots.begin(); }
    ConstIterator end() const { return m_slots.end(); }

    Iterator begin() { return m_slots.begin(); }
    Iterator end() { return m_slots.end(); }

    /// Connect a slot to this signal. Must only be called by child classes.
    SlotID connect(const SlotPtr& slot) const {
        return m_slots.insert(m_slots.end(), slot);
    }

    void begin_emitting() { ++m_emitting; }
    void end_emitting() {
        if(--m_emitting == 0) {
            // remove elements which belonged slots that detached themselves
            m_slots.erase(std::remove(m_slots.begin(), m_slots.end(), SlotPtr()), m_slots.end());
        }
    }
private:
    typedef std::set<Tracker*> Trackers;
    mutable SlotList m_slots; ///< all slots connected to a signal
    mutable Trackers m_trackers; ///< all instances that tracks this signal.
    unsigned m_emitting;
};

} // namespace SigImpl


/// Specialization for three arguments.
template <typename Arg1 = SigImpl::EmptyArg, typename Arg2 = SigImpl::EmptyArg, typename Arg3 = SigImpl::EmptyArg >
class Signal: public SigImpl::SignalHolder {
public:
    void emit(Arg1 arg1, Arg2 arg2, Arg3 arg3) {
        begin_emitting();
        for ( Iterator it = begin(); it != end(); ++it ) {
            if(*it)
                static_cast<Slot<void, Arg1, Arg2, Arg3> &>(**it)(arg1, arg2, arg3);
        }
        end_emitting();
    }

    template<typename Functor>
    SlotID connect(const Functor& functor) const {
        return SignalHolder::connect(SlotPtr(
                        new SlotImpl<Functor, void, Arg1, Arg2, Arg3>(functor)
                    ));
    }

    SlotID connectSlot(const RefCount<FbTk::Slot<void, Arg1, Arg2, Arg3> > &slot) const {
        return SignalHolder::connect(slot);
    }
};

/// Specialization for two arguments.
template <typename Arg1, typename Arg2>
class Signal<Arg1, Arg2, SigImpl::EmptyArg>: public SigImpl::SignalHolder {
public:
    void emit(Arg1 arg1, Arg2 arg2) {
        begin_emitting();
        for ( Iterator it = begin(); it != end(); ++it ) {
            if(*it)
                static_cast<Slot<void, Arg1, Arg2> &>(**it)(arg1, arg2);
        }
        end_emitting();
    }

    template<typename Functor>
    SlotID connect(const Functor& functor) const {
        return SignalHolder::connect(SlotPtr(
                        new SlotImpl<Functor, void, Arg1, Arg2>(functor)
                    ));
    }

    SlotID connectSlot(const RefCount<FbTk::Slot<void, Arg1, Arg2> > &slot) const {
        return SignalHolder::connect(slot);
    }
};

/// Specialization for one argument.
template <typename Arg1>
class Signal<Arg1, SigImpl::EmptyArg, SigImpl::EmptyArg>: public SigImpl::SignalHolder {
public:
    void emit(Arg1 arg) {
        begin_emitting();
        for ( Iterator it = begin(); it != end(); ++it ) {
            if(*it)
                static_cast<Slot<void, Arg1> &>(**it)(arg);
        }
        end_emitting();
    }

    template<typename Functor>
    SlotID connect(const Functor& functor) const {
        return SignalHolder::connect(SlotPtr(
                        new SlotImpl<Functor, void, Arg1>(functor)
                    ));
    }

    SlotID connectSlot(const RefCount<FbTk::Slot<void, Arg1> > &slot) const {
        return SignalHolder::connect(slot);
    }
};

/// Specialization for no arguments.
template <>
class Signal<SigImpl::EmptyArg, SigImpl::EmptyArg, SigImpl::EmptyArg>: public SigImpl::SignalHolder {
public:
    void emit() {
        begin_emitting();
        for ( Iterator it = begin(); it != end(); ++it ) {
            if(*it)
                static_cast<Slot<void> &>(**it)();
        }
        end_emitting();
    }

    template<typename Functor>
    SlotID connect(const Functor& functor) const {
        return SignalHolder::connect(SlotPtr(
                        new SlotImpl<Functor, void>(functor)
                    ));
    }

    SlotID connectSlot(const RefCount<FbTk::Slot<void> > &slot) const {
        return SignalHolder::connect(slot);
    }
};

/**
 * Tracks a signal during it's life time. All signals connected using \c
 * SignalTracker::join will be erased when this instance dies.
 */
class SignalTracker: public SigImpl::SignalHolder::Tracker {
public:
    /// Internal type, do not use.
    typedef std::map<const SigImpl::SignalHolder*,
                     SigImpl::SignalHolder::SlotID> Connections;
    typedef Connections::iterator TrackID; ///< \c ID type for join/leave.

    ~SignalTracker() {
        leaveAll();
    }

    /// Starts tracking a signal.
    /// @return A tracking ID
    template<typename Arg1, typename Arg2, typename Arg3, typename Functor>
    TrackID join(const Signal<Arg1, Arg2, Arg3> &sig, const Functor &functor) {
        return joinSlot(sig, RefCount<Slot<void, Arg1, Arg2, Arg3> >(
                    new SlotImpl<Functor, void, Arg1, Arg2, Arg3>(functor)
                    ));
    }

    template<typename Arg1, typename Arg2, typename Arg3>
    TrackID
    joinSlot(const Signal<Arg1, Arg2, Arg3> &sig,
            const RefCount<Slot<void, Arg1, Arg2, Arg3> > &slot) {

        ValueType value = ValueType(&sig, sig.connectSlot(slot));
        std::pair<TrackID, bool> ret = m_connections.insert(value);
        if ( !ret.second ) {
            // failed to insert this functor
            sig.disconnect(value.second);
        }

        sig.connectTracker(*this);

        return ret.first;
    }

    /// Leave tracking for a signal
    /// @param id the \c id from the previous \c join 
   void leave(TrackID id) {
       // keep temporary, while disconnecting we can
       // in some strange cases get a call to this again
        ValueType tmp = *id;
        m_connections.erase(id);
        tmp.first->disconnect(tmp.second);
        tmp.first->disconnectTracker(*this);
    }

    /// Leave tracking for a signal
    /// @param sig the signal to leave
    template <typename Signal>
    void leave(Signal &sig) {
        Iterator it = m_connections.find(&sig);
        if (it != m_connections.end()) {
            leave(it);
        }
    }


    void leaveAll() {
        // disconnect all connections
        for ( ; !m_connections.empty(); ) {
            leave(m_connections.begin());
        }
    }

protected:

    virtual void disconnect(SigImpl::SignalHolder& signal) {
        m_connections.erase(&signal);
    }

private:
    typedef Connections::value_type ValueType;
    typedef Connections::iterator Iterator;
    /// holds all connections to different signals and slots.
    Connections m_connections;
};


} // namespace FbTk

#endif // FBTK_SIGNAL_HH
