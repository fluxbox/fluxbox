// FocusableList.hh
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

#ifndef FOCUSABLELIST_HH
#define FOCUSABLELIST_HH

#include "FbTk/NotCopyable.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Signal.hh"

#include "ClientPattern.hh"

#include <list>
#include <string>
#include <memory>

class BScreen;
class Focusable;
class WinClient;
class FluxboxWindow;

class FocusableList: private FbTk::NotCopyable,
                     private FbTk::SignalTracker {
public:
    typedef std::list<Focusable *> Focusables;

    /// list option bits
    enum { 
        LIST_GROUPS = 0x01,  //< list groups instead of clients
        STATIC_ORDER = 0x02  ///< use creation order instead of focused order
    };

    FocusableList(BScreen &scr): m_parent(0), m_screen(scr) { }
    FocusableList(BScreen &scr, const std::string & pat);
    FocusableList(BScreen &scr, const FocusableList &parent,
                  const std::string & pat);

    static void parseArgs(const std::string &in, int &opts, std::string &out);
    static const FocusableList *getListFromOptions(BScreen &scr, int opts);

    /// functions for modifying the list contents
    void pushFront(Focusable &win);
    void pushBack(Focusable &win);
    void moveToFront(Focusable &win);
    void moveToBack(Focusable &win);
    void remove(Focusable &win);

    /// accessor for list
    Focusables &clientList() { return m_list; }
    const Focusables &clientList() const { return m_list; }

    /// does the list contain any windows?
    bool empty() const { return m_list.empty(); }
    /// does the list contain the given window?
    bool contains(const Focusable &win) const;
    /// find the first window matching the pattern
    Focusable *find(const ClientPattern &pattern) const;

    /**
       @name signals
       @{
    */
    const FbTk::Signal<Focusable *> &orderSig() const { return m_ordersig; }
    const FbTk::Signal<Focusable *> &addSig() const { return m_addsig; }
    const FbTk::Signal<Focusable *> &removeSig() const { return m_removesig; }
    const FbTk::Signal<> &resetSig() const { return m_resetsig; }
    /** @} */ // end group signals

private:
    void init();
    void addMatching();
    void checkUpdate(Focusable &win);
    bool insertFromParent(Focusable &win);
    void attachSignals(Focusable &win);
    void reset();
    void workspaceChanged(BScreen &screen);
    void focusedWindowChanged(BScreen &screen, FluxboxWindow *win, WinClient *client);
    /// Title has changed for a window
    /// @param win The window that title changed for.
    void updateTitle(Focusable& win);
    void parentOrderChanged(Focusable* win);
    void parentWindowAdded(Focusable* win);
    void parentWindowRemoved(Focusable* win);
    void windowUpdated(FluxboxWindow &fbwin);


    std::unique_ptr<ClientPattern> m_pat;
    const FocusableList *m_parent;
    BScreen &m_screen;
    std::list<Focusable *> m_list;

    FbTk::Signal<Focusable *> m_ordersig, m_addsig, m_removesig;
    FbTk::Signal<> m_resetsig;
    typedef std::map<Focusable*, FbTk::RefCount<FbTk::SignalTracker> > SignalMap;
    SignalMap m_signal_map;
};

#endif // FOCUSABLELIST_HH
