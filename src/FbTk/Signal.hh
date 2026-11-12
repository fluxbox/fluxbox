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
 *
 * Nothing here depends on the signal's argument types, so all of it is
 * compiled once, in Signal.cc, instead of in every file using signals.
 */
class SignalHolder {
protected:
    typedef RefCount<SlotBase> SlotPtr;
    typedef std::list<SlotPtr> SlotList;

public:
    /// Special tracker interface used by SignalTracker.
    class Tracker {
    public:
        virtual ~Tracker();
        /// Disconnect this holder.
        virtual void disconnect(SignalHolder& signal) = 0;
    };

    typedef SlotList::iterator Iterator;
    typedef Iterator SlotID;
    typedef SlotList::const_iterator ConstIterator;

    SignalHolder();
    ~SignalHolder();

    /// Remove a specific slot \c id from this signal
    void disconnect(SlotID slotIt) const;

    /// Removes all slots connected to this
    void clear();

    void connectTracker(SignalHolder::Tracker& tracker) const;
    void disconnectTracker(SignalHolder::Tracker& tracker) const;

protected:
    ConstIterator begin() const { return m_slots.begin(); }
    ConstIterator end() const { return m_slots.end(); }

    Iterator begin() { return m_slots.begin(); }
    Iterator end() { return m_slots.end(); }

    /// Connect a slot to this signal. Must only be called by child classes.
    SlotID connect(const SlotPtr& slot) const;

    void begin_emitting() { ++m_emitting; }
    void end_emitting();
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
 *
 * Only join/joinSlot are templates (for the type check); the bookkeeping is
 * compiled once, in Signal.cc.
 */
class SignalTracker: public SigImpl::SignalHolder::Tracker {
public:
    /// Internal type, do not use.
    typedef std::map<const SigImpl::SignalHolder*,
                     SigImpl::SignalHolder::SlotID> Connections;
    typedef Connections::iterator TrackID; ///< \c ID type for join/leave.

    SignalTracker();
    ~SignalTracker();

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
        return track(sig, sig.connectSlot(slot));
    }

    /// Leave tracking for a signal
    /// @param id the \c id from the previous \c join
    void leave(TrackID id);

    /// Leave tracking for a signal
    /// @param sig the signal to leave
    void leave(const SigImpl::SignalHolder &sig);

    void leaveAll();

protected:

    virtual void disconnect(SigImpl::SignalHolder& signal);

private:
    typedef Connections::value_type ValueType;
    typedef Connections::iterator Iterator;

    /// Remember the connection \c slot to \c sig; a second connection to the
    /// same signal is refused (the slot is disconnected again) and the first
    /// one's id is returned.
    TrackID track(const SigImpl::SignalHolder &sig, SigImpl::SignalHolder::SlotID slot);

    /// holds all connections to different signals and slots.
    Connections m_connections;
};


} // namespace FbTk

#endif // FBTK_SIGNAL_HH
