// FocusableList.cc
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "FocusableList.hh"

#include "Focusable.hh"
#include "FocusControl.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Window.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/MemFun.hh"

#include <vector>
#include <algorithm>
#include <iterator>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;
using std::vector;

void FocusableList::parseArgs(const string &in, int &opts, string &pat) {
    string options;
    int err = FbTk::StringUtil::getStringBetween(options, in.c_str(), '{', '}',
                                                 " \t\n");

    // the rest of the string is a ClientPattern
    pat = in.c_str() + (err > 0 ? err : 0);

    // now parse the options
    vector<string> args;
    FbTk::StringUtil::stringtok(args, options);
    vector<string>::iterator it = args.begin(), it_end = args.end();
    opts = 0;
    for (; it != it_end; ++it) {
        if (strcasecmp((*it).c_str(), "static") == 0)
            opts |= STATIC_ORDER;
        else if (strcasecmp((*it).c_str(), "groups") == 0)
            opts |= LIST_GROUPS;
    }
}

const FocusableList *FocusableList::getListFromOptions(BScreen &scr, int opts) {
    if (opts & LIST_GROUPS)
        return (opts & STATIC_ORDER) ?
                &scr.focusControl().creationOrderWinList() :
                &scr.focusControl().focusedOrderWinList();
    return (opts & STATIC_ORDER) ?
            &scr.focusControl().creationOrderList() :
            &scr.focusControl().focusedOrderList();
}

FocusableList::FocusableList(BScreen &scr, const string & pat):
    m_parent(0), m_screen(scr), m_manual_order(false) {

    int options = 0;
    string pattern;
    parseArgs(pat, options, pattern);
    m_parent = getListFromOptions(scr, options);
    m_pat.reset(new ClientPattern(pattern.c_str()));

    init();
}

FocusableList::FocusableList(BScreen &scr, const FocusableList &parent,
                             const string & pat):
    m_pat(new ClientPattern(pat.c_str())), m_parent(&parent), m_screen(scr),
    m_manual_order(false) {

    init();
}

void FocusableList::init() {
    addMatching();

    join(m_parent->addSig(), FbTk::MemFun(*this, &FocusableList::parentWindowAdded));
    join(m_parent->orderSig(), FbTk::MemFun(*this, &FocusableList::parentOrderChanged));
    join(m_parent->removeSig(), FbTk::MemFun(*this, &FocusableList::parentWindowRemoved));
    join(m_parent->resetSig(), FbTk::MemFun(*this, &FocusableList::reset));

    // TODO: can't handle (head=[mouse]) yet
    if (m_pat->dependsOnCurrentWorkspace()) {
        join(m_screen.currentWorkspaceSig(),
             FbTk::MemFun(*this, &FocusableList::workspaceChanged));
    }
    if (m_pat->dependsOnFocusedWindow()) {
        join(m_screen.focusedWindowSig(),
             FbTk::MemFun(*this, &FocusableList::focusedWindowChanged));
    }
}

void FocusableList::windowUpdated(FluxboxWindow &fbwin) {
    if (m_screen.isShuttingdown())
        return;

    // we only bind these for matching patterns, so skip finding out signal
    if (m_parent->contains(fbwin))
        checkUpdate(fbwin);
    const std::list<WinClient *> &list = fbwin.clientList();
    std::list<WinClient *>::const_iterator it = list.begin(), it_end = list.end();
    for (; it != it_end; ++it) {
        if (m_parent->contains(**it))
            checkUpdate(**it);
    }
}

void FocusableList::parentOrderChanged(Focusable *win) {
    if(!m_screen.isShuttingdown() && contains(*win)) {
        if(insertFromParent(*win))
            m_ordersig.emit(win);
    }
}

void FocusableList::parentWindowAdded(Focusable *win) {
    if(!m_screen.isShuttingdown()) {
        attachSignals(*win);
        if (m_pat->match(*win)) {
            insertFromParent(*win);
            m_addsig.emit(win);
        }
    }
}

void FocusableList::parentWindowRemoved(Focusable *win) {
    if(!m_screen.isShuttingdown())
        remove(*win);
}

void FocusableList::checkUpdate(Focusable &win) {
    if (contains(win)) {
        if (!m_pat->match(win)) {
            m_list.remove(&win);
            m_pat->removeMatch();
            m_removesig.emit(&win);
        }
    } else if (m_pat->match(win)) {
        insertFromParent(win);
        m_pat->addMatch();
        m_addsig.emit(&win);
    }
}

// returns whether or not the window was moved
bool FocusableList::insertFromParent(Focusable &win) {
    if (m_manual_order) {
        // the order of this list is controlled by the user, so only the
        // membership is kept in sync with the parent list. A window that
        // comes back goes to the place the user gave it, a window that was
        // never placed goes to the end.
        if (!contains(win)) {
            m_list.push_back(&win);
            restoreOrder();
        }
        return false;
    }

    const Focusables &list = m_parent->clientList();
    Focusables::const_iterator p_it = list.begin(), p_it_end = list.end();
    Focusables::iterator our_it = m_list.begin(), our_it_end = m_list.end();
    // walk through our list looking for corresponding entries in
    // parent's list, until we find the window that moved
    for (; our_it != our_it_end && p_it != p_it_end; ++p_it) {
        if (*p_it == &win) {
            if (*our_it == &win) // win didn't move in our list
                return false;
            m_list.remove(&win);
            m_list.insert(our_it, &win);
            return true;
        }
        if (*p_it == *our_it)
            ++our_it;
    }
    m_list.remove(&win);
    m_list.push_back(&win);
    return true;
}

void FocusableList::addMatching() {
    if (!m_parent)
        return;

    const Focusables &list = m_parent->clientList();
    Focusables::const_iterator it = list.begin(), it_end = list.end();
    for (; it != it_end; ++it) {
        if (m_pat->match(**it)) {
            m_list.push_back(*it);
            m_pat->addMatch();
        }
        attachSignals(**it);
    }
}

void FocusableList::pushFront(Focusable &win) {
    m_list.push_front(&win);
    attachSignals(win);
    m_addsig.emit(&win);
}

void FocusableList::pushBack(Focusable &win) {
    m_list.push_back(&win);
    attachSignals(win);
    m_addsig.emit(&win);
}

void FocusableList::moveToFront(Focusable &win) {
    // if the window isn't already in this list, we could accidentally add it
    if (!contains(win))
        return;

    m_list.remove(&win);
    m_list.push_front(&win);
    rememberOrder();
    m_ordersig.emit(&win);
}

void FocusableList::moveToBack(Focusable &win) {
    // if the window isn't already in this list, we could accidentally add it
    if (!contains(win))
        return;

    m_list.remove(&win);
    m_list.push_back(&win);
    rememberOrder();
    m_ordersig.emit(&win);
}

bool FocusableList::moveTo(Focusable &win, size_t index) {
    Focusables::iterator it = std::find(m_list.begin(), m_list.end(), &win);
    if (it == m_list.end())
        return false; // the window is not in this list

    // the index is the slot the window should end up in, so the last slot
    // is the highest one that means anything. Clamp before comparing with
    // the current position, otherwise a move beyond the end looks like a
    // change when it leaves the window where it is.
    if (index >= m_list.size())
        index = m_list.size() - 1;

    size_t current = static_cast<size_t>(std::distance(m_list.begin(), it));
    if (current == index)
        return false; // the window is already in the right place

    m_list.erase(it);

    Focusables::iterator pos = m_list.begin();
    std::advance(pos, static_cast<Focusables::difference_type>(index));
    m_list.insert(pos, &win);

    rememberOrder();
    m_ordersig.emit(&win);
    return true;
}

void FocusableList::setManualOrder(bool manual) {
    if (m_manual_order == manual)
        return;

    m_manual_order = manual;

    if (manual) {
        // the list was built from the parent, put the windows back where
        // the user had them
        restoreOrder();
        return;
    }

    // when the user gives up control of the order, the order of the
    // parent list is authoritative again, so the remembered one is dropped
    if (m_order)
        m_order->clear();
    if (m_parent)
        reset();
}

void FocusableList::setOrder(std::shared_ptr<Focusables> order) {
    m_order = order;
    if (!m_order || !m_parent)
        return;

    // drop the windows that are not in the parent list any more: they are
    // gone, or this list holds another kind of object than the one that
    // placed them. Only pointers are compared, they may no longer be valid.
    const Focusables &parent = m_parent->clientList();
    Focusables::iterator it = m_order->begin();
    while (it != m_order->end()) {
        if (std::find(parent.begin(), parent.end(), *it) == parent.end())
            it = m_order->erase(it);
        else
            ++it;
    }
}

void FocusableList::remove(Focusable &win) {
    // if the window isn't already in this list, we could send a bad signal
    bool contained = contains(win);

    m_signal_map.erase(&win);
    // the window is gone for good, so it must not keep its place in the
    // remembered order: a new window could be given its address
    if (m_order)
        m_order->remove(&win);
    if (!contained) {
        return;
    }
    m_list.remove(&win);
    m_removesig.emit(&win);
}

void FocusableList::updateTitle(Focusable& win) {
    checkUpdate(win);
}
#include "Debug.hh"

void FocusableList::attachSignals(Focusable &win) {
    if (m_parent == NULL)
        return;

    FluxboxWindow *fbwin = win.fbwindow();

    // attach various signals for matching
    FbTk::RefCount<FbTk::SignalTracker> &tracker = m_signal_map[&win];
    if (! tracker) {
        // we have not attached to this window yet
        tracker.reset(new SignalTracker);
        tracker->join(win.titleSig(), MemFunSelectArg1(*this, &FocusableList::updateTitle));
        tracker->join(win.dieSig(), MemFun(*this, &FocusableList::remove));
        if(fbwin) {
            tracker->join(fbwin->workspaceSig(), MemFun(*this, &FocusableList::windowUpdated));
            tracker->join(fbwin->stateSig(), MemFun(*this, &FocusableList::windowUpdated));
            tracker->join(fbwin->layerSig(), MemFun(*this, &FocusableList::windowUpdated));
            // TODO: can't watch (head=...) yet
        }
    }
}

void FocusableList::reset() {
    m_signal_map.clear();
    m_list.clear();
    m_pat->resetMatches();
    if (m_parent)
        addMatching();
    restoreOrder();
    m_resetsig.emit();
}

// put the windows back in the order that was remembered for them. The
// windows the user placed come first, in the order he gave them, the ones
// he never placed follow in the order they were found in.
void FocusableList::restoreOrder() {
    if (!m_manual_order || !m_order || m_order->empty())
        return;

    Focusables ordered;
    Focusables::const_iterator o_it = m_order->begin(), o_it_end = m_order->end();
    for (; o_it != o_it_end; ++o_it) {
        // only the pointers are compared, they are known to be valid here
        if (std::find(m_list.begin(), m_list.end(), *o_it) != m_list.end())
            ordered.push_back(*o_it);
    }

    Focusables::iterator it = m_list.begin(), it_end = m_list.end();
    for (; it != it_end; ++it) {
        if (std::find(m_order->begin(), m_order->end(), *it) == m_order->end())
            ordered.push_back(*it);
    }

    m_list.swap(ordered);
}

// record the current order as the one the user gave. The windows that are
// shown keep the order they have now, the ones that are away keep the place
// they had, so that they come back to it.
void FocusableList::rememberOrder() {
    if (!m_manual_order || !m_order)
        return;

    Focusables ordered(m_list);
    Focusables::const_iterator it = m_order->begin(), it_end = m_order->end();
    for (; it != it_end; ++it) {
        if (std::find(m_list.begin(), m_list.end(), *it) == m_list.end())
            ordered.push_back(*it);
    }

    m_order->swap(ordered);
}

bool FocusableList::contains(const Focusable &win) const {
    Focusables::const_iterator it = m_list.begin(), it_end = m_list.end();
    it = std::find(it, it_end, &win);
    return (it != it_end);
}

Focusable *FocusableList::find(const ClientPattern &pat) const {
    Focusables::const_iterator it = m_list.begin(), it_end = m_list.end();
    for (; it != it_end; ++it) {
        if (pat.match(**it))
            return *it;
    }
    return 0;
}

void FocusableList::workspaceChanged(BScreen &screen) {
    reset();
}

void FocusableList::focusedWindowChanged(BScreen &screen,
                                         FluxboxWindow *focused_win,
                                         WinClient *client) {
    reset();
}
