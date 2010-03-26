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

FocusableList::FocusableList(BScreen &scr, const string pat):
    m_pat(0), m_parent(0), m_screen(scr) {

    int options = 0;
    string pattern;
    parseArgs(pat, options, pattern);
    m_parent = getListFromOptions(scr, options);
    m_pat.reset(new ClientPattern(pattern.c_str()));

    init();
}

FocusableList::FocusableList(BScreen &scr, const FocusableList &parent,
                             const string pat):
    m_pat(new ClientPattern(pat.c_str())), m_parent(&parent), m_screen(scr) {

    init();
}

void FocusableList::init() {
    addMatching();
    m_parent->attachChild(*this);

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

void FocusableList::update(FbTk::Subject *subj) {
    if (subj == 0 || m_screen.isShuttingdown())
        return;

    if (typeid(*subj) == typeid(Focusable::FocusSubject)) {
        Focusable::FocusSubject *fsubj =
            static_cast<Focusable::FocusSubject *>(subj);
        if (fsubj == &fsubj->win().dieSig()) {
            remove(fsubj->win());
        }
    }
    if (typeid(*subj) == typeid(FluxboxWindow::WinSubject)) {
        FluxboxWindow::WinSubject *fsubj =
            static_cast<FluxboxWindow::WinSubject *>(subj);
        // we only bind these for matching patterns, so skip finding out signal
        FluxboxWindow &fbwin = fsubj->win();
        if (m_parent->contains(fbwin))
            checkUpdate(fbwin);
        std::list<WinClient *> list = fbwin.clientList();
        std::list<WinClient *>::iterator it = list.begin(), it_end = list.end();
        for (; it != it_end; ++it) {
            if (m_parent->contains(**it))
                checkUpdate(**it);
        }
    }
    if (typeid(*subj) == typeid(FocusableListSubject)) {
        FocusableListSubject *fsubj =
            static_cast<FocusableListSubject *>(subj);
        if (subj == &m_parent->addSig()) {
            attachSignals(*fsubj->win());
            if (m_pat->match(*fsubj->win())) {
                insertFromParent(*fsubj->win());
                m_addsig.notify(fsubj->win());
            }
        } else if (subj == &m_parent->removeSig())
            remove(*fsubj->win());
        else if (subj == &m_parent->resetSig())
            reset();
        else if (subj == &m_parent->orderSig()) {
            Focusable *win = fsubj->win();
            if (!win || !contains(*win))
                return;
            if (insertFromParent(*win))
                m_ordersig.notify(win);
        }
    }
}

void FocusableList::checkUpdate(Focusable &win) {
    if (contains(win)) {
        if (!m_pat->match(win)) {
            m_list.remove(&win);
            m_pat->removeMatch();
            m_removesig.notify(&win);
        }
    } else if (m_pat->match(win)) {
        insertFromParent(win);
        m_pat->addMatch();
        m_addsig.notify(&win);
    }
}

// returns whether or not the window was moved
bool FocusableList::insertFromParent(Focusable &win) {
    const Focusables list = m_parent->clientList();
    Focusables::const_iterator p_it = list.begin(), p_it_end = list.end();
    Focusables::iterator our_it = m_list.begin(), our_it_end = m_list.end();
    // walk through our list looking for corresponding entries in
    // parent's list, until we find the window that moved
    for (; our_it != our_it_end && p_it != p_it_end; p_it++) {
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

    const Focusables list = m_parent->clientList();
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
    m_addsig.notify(&win);
}

void FocusableList::pushBack(Focusable &win) {
    m_list.push_back(&win);
    attachSignals(win);
    m_addsig.notify(&win);
}

void FocusableList::moveToFront(Focusable &win) {
    // if the window isn't already in this list, we could accidentally add it
    if (!contains(win))
        return;

    m_list.remove(&win);
    m_list.push_front(&win);
    m_ordersig.notify(&win);
}

void FocusableList::moveToBack(Focusable &win) {
    // if the window isn't already in this list, we could accidentally add it
    if (!contains(win))
        return;

    m_list.remove(&win);
    m_list.push_back(&win);
    m_ordersig.notify(&win);
}

void FocusableList::remove(Focusable &win) {
    // if the window isn't already in this list, we could send a bad signal
    bool contained = contains(win);

    detachSignals(win);
    if (!contained) {
        return;
    }
    m_list.remove(&win);
    m_removesig.notify(&win);
}

void FocusableList::updateTitle(Focusable& win) {
    checkUpdate(win);
}

void FocusableList::attachSignals(Focusable &win) {
    win.dieSig().attach(this);
    if (m_parent) {
        // attach various signals for matching
        if (m_signal_map.find(&win) == m_signal_map.end()) {
            m_signal_map[&win] = join(win.titleSig(),
                                      MemFunSelectArg1(*this,
                                                       &FocusableList::updateTitle));
        }

        FluxboxWindow *fbwin = win.fbwindow();
        if (!fbwin)
            return;
        fbwin->workspaceSig().attach(this);
        fbwin->stateSig().attach(this);
        fbwin->layerSig().attach(this);
        // TODO: can't watch (head=...) yet
    }
}

void FocusableList::detachSignals(Focusable &win) {
    win.dieSig().detach(this);
    if (m_parent) {
        // disconnect client
        SignalMap::iterator sigIt = m_signal_map.find(&win);
        if (sigIt != m_signal_map.end()) {
            leave(sigIt->second);
            m_signal_map.erase(sigIt);
        }

        // detach various signals for matching
        FluxboxWindow *fbwin = win.fbwindow();
        if (!fbwin)
            return;
        fbwin->workspaceSig().detach(this);
        fbwin->stateSig().detach(this);
        fbwin->layerSig().detach(this);
        // TODO: can't watch (head=...) yet
    }
}

void FocusableList::reset() {
    while (!m_list.empty()) {
        detachSignals(*m_list.back());
        m_list.pop_back();
    }
    m_pat->resetMatches();
    if (m_parent)
        addMatching();
    m_resetsig.notify(0);
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

void FocusableList::attachChild(FocusableList &child) const {
    m_addsig.attach(&child);
    m_removesig.attach(&child);
    m_resetsig.attach(&child);
    m_ordersig.attach(&child);
}

void FocusableList::workspaceChanged(BScreen &screen) {
    reset();
}

void FocusableList::focusedWindowChanged(BScreen &screen,
                                         FluxboxWindow *focused_win,
                                         WinClient *client) {
    reset();
}
