// SimpleObserver.hh
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "Observer.hh"
#include "SimpleCommand.hh"

namespace FbTk {

/** Functor for observers, instead of using this directly use makeObserver.
 * Usage: 
 * @code
 * class SomeClass {
 * public:
 *    void doAction();
 * };
 *
 * SomeClass some;
 *
 * Observer* obs = makeProxyObserver(some, &SomeClass::doAction);
 * SomeSubject subj;
 * subj.attach(obs);
 * @endcode
 */
template <typename Receiver>
class SimpleObserver: public Observer {
public:
    typedef void (Receiver::* Action)();
    SimpleObserver(Receiver &r, Action a):
        m_receiver(r), m_action(a) {
        
    }
    void update(Subject *changedSubj) {
        (m_receiver.*m_action)();
    }
private:
    Receiver &m_receiver;
    Action m_action;
};

// Helpers
/** Creates an observer that takes no arguments.
 * @param receiver The receiving instance.
 * @param action A function in the receiving class.
 * @return allocated simple observer. @see SimpleObserver
 */
template <typename Receiver, typename Action>
Observer *makeObserver(Receiver &receiver, Action action) {
    return new SimpleObserver<Receiver>( receiver, action );
}

}

