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

// $Id: $

#ifndef FOCUSABLELIST_HH
#define FOCUSABLELIST_HH

#include "FbTk/NotCopyable.hh"
#include "FbTk/Observer.hh"
#include "FbTk/Subject.hh"

#include "ClientPattern.hh"

#include <list>
#include <string>

class BScreen;
class Focusable;

class FocusableList: public FbTk::Observer, private FbTk::NotCopyable {
public:
    typedef std::list<Focusable *> Focusables;

    /// list option bits
    enum { 
        LIST_GROUPS = 0x01,  //< list groups instead of clients
        STATIC_ORDER = 0x02,  ///< use creation order instead of focused order
    };

    FocusableList(BScreen &scr): m_pat(0), m_parent(0), m_screen(scr) { }
    FocusableList(BScreen &scr, const std::string pat);
    FocusableList(BScreen &scr, const FocusableList &parent,
                  const std::string pat);

    static void parseArgs(const std::string &in, int &opts, std::string &out);
    static const FocusableList *getListFromOptions(BScreen &scr, int opts);

    void update(FbTk::Subject *subj);

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

    /**
       @name signals
       @{
    */
    FbTk::Subject &orderSig() { return m_ordersig; }
    const FbTk::Subject &orderSig() const { return m_ordersig; }
    FbTk::Subject &addSig() { return m_addsig; }
    const FbTk::Subject &addSig() const { return m_addsig; }
    FbTk::Subject &removeSig() { return m_removesig; }
    const FbTk::Subject &removeSig() const { return m_removesig; }
    FbTk::Subject &resetSig() { return m_resetsig; }
    const FbTk::Subject &resetSig() const { return m_resetsig; }
    /** @} */ // end group signals

    /**
     * Signaling object to attatch observers to.
     */
    class FocusableListSubject: public FbTk::Subject {
    public:
        explicit FocusableListSubject(): m_win(0) { }
        void notify(Focusable *win) { m_win = win; FbTk::Subject::notify(); }
        /// @return context for this signal
        Focusable *win() { return m_win; }

    private:
        Focusable *m_win;
    };

private:
    void init();
    void addMatching();
    void checkUpdate(Focusable &win);
    bool insertFromParent(Focusable &win);
    void attachSignals(Focusable &win);
    void detachSignals(Focusable &win);
    void reset();
    void attachChild(FocusableList &child) const;

    std::auto_ptr<ClientPattern> m_pat;
    const FocusableList *m_parent;
    BScreen &m_screen;
    std::list<Focusable *> m_list;

    mutable FocusableListSubject m_ordersig, m_addsig, m_removesig, m_resetsig;
};

#endif // FOCUSABLELIST_HH
