// Signal.cc for FbTk, Fluxbox Toolkit
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

// The parts of the signal/slot machinery that do not depend on argument
// types; see Signal.hh.

#include "Signal.hh"

namespace FbTk {

namespace SigImpl {

SignalHolder::Tracker::~Tracker() {
}

SignalHolder::SignalHolder(): m_emitting(0) {
}

SignalHolder::~SignalHolder() {
    // Disconnect this holder from all trackers.
    for (Trackers::iterator it = m_trackers.begin(),
             it_end = m_trackers.end();
         it != it_end; ++it ) {
        (*it)->disconnect(*this);
    }
}

void SignalHolder::disconnect(SlotID slotIt) const {
    if(m_emitting) {
        // if we are emitting, we must not erase the actual element, as that would
        // invalidate iterators in the emit() function
        *slotIt = SlotPtr();
    } else
        m_slots.erase( slotIt );
}

void SignalHolder::clear() {
    if(m_emitting)
        std::fill(m_slots.begin(), m_slots.end(), SlotPtr());
    else
        m_slots.clear();
}

void SignalHolder::connectTracker(SignalHolder::Tracker& tracker) const {
    m_trackers.insert(&tracker);
}

void SignalHolder::disconnectTracker(SignalHolder::Tracker& tracker) const {
    m_trackers.erase(&tracker);
}

SignalHolder::SlotID SignalHolder::connect(const SlotPtr& slot) const {
    return m_slots.insert(m_slots.end(), slot);
}

void SignalHolder::end_emitting() {
    if(--m_emitting == 0) {
        // remove elements which belonged slots that detached themselves
        m_slots.erase(std::remove(m_slots.begin(), m_slots.end(), SlotPtr()), m_slots.end());
    }
}

} // namespace SigImpl


SignalTracker::SignalTracker() {
}

SignalTracker::~SignalTracker() {
    leaveAll();
}

SignalTracker::TrackID SignalTracker::track(const SigImpl::SignalHolder &sig,
                                            SigImpl::SignalHolder::SlotID slot) {
    ValueType value = ValueType(&sig, slot);
    std::pair<TrackID, bool> ret = m_connections.insert(value);
    if ( !ret.second ) {
        // failed to insert this functor
        sig.disconnect(value.second);
    }

    sig.connectTracker(*this);

    return ret.first;
}

void SignalTracker::leave(TrackID id) {
    // keep temporary, while disconnecting we can
    // in some strange cases get a call to this again
    ValueType tmp = *id;
    m_connections.erase(id);
    tmp.first->disconnect(tmp.second);
    tmp.first->disconnectTracker(*this);
}

void SignalTracker::leave(const SigImpl::SignalHolder &sig) {
    Iterator it = m_connections.find(&sig);
    if (it != m_connections.end()) {
        leave(it);
    }
}

void SignalTracker::leaveAll() {
    // disconnect all connections
    for ( ; !m_connections.empty(); ) {
        leave(m_connections.begin());
    }
}

void SignalTracker::disconnect(SigImpl::SignalHolder& signal) {
    m_connections.erase(&signal);
}

} // namespace FbTk
