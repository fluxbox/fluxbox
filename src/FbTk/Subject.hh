// Subject.hh for FbTk
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef FBTK_SUBJECT_HH
#define FBTK_SUBJECT_HH

#include "NotCopyable.hh"

#include <list>

namespace FbTk {

class Observer;

class Subject:private FbTk::NotCopyable {
public:
    Subject();
    virtual ~Subject();
    /// attach an observer
    void attach(Observer *obs);
    /// detach an observer
    void detach(Observer *obs);
    /// notify all attached observers
    void notify();
    static void removeObserver(Observer *obs);
private:
    bool m_notify_mode;
    
    typedef std::list<Observer *> ObserverList;
    ObserverList m_observerlist;
    ObserverList m_dead_observers;

    typedef std::list<Subject *> SubjectList;
    static SubjectList s_subjectlist;
};

} // end namespace FbTk

#endif // FBTK_SUBJECT_HH
