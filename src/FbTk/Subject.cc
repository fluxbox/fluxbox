// Subject.cc for FbTk
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@fluxbox.org)
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

// $Id: Subject.cc,v 1.1 2003/02/15 01:21:40 fluxgen Exp $

#include "Subject.hh"
#include "Observer.hh"

#include <algorithm>
#include <functional>

namespace FbTk {

Subject::SubjectList Subject::s_subjectlist;

Subject::Subject() {
    s_subjectlist.push_back(this);
}

Subject::~Subject() {
    SubjectList::iterator it = s_subjectlist.begin();
    SubjectList::iterator it_end = s_subjectlist.end();
    for (; it != it_end; ++it) {
        if (this == (*it)) {
            s_subjectlist.erase(it);
            break;
        }
    }
}

void Subject::attach(Observer *obj) {
    m_observerlist.push_back(obj);
    // no need to have more than one instance of an observer
    std::unique(m_observerlist.begin(), m_observerlist.end());
}

void Subject::detach(Observer *obj) {
    ObserverList::iterator it = m_observerlist.begin();
    ObserverList::iterator it_end = m_observerlist.end();
    for (; it != it_end; ++it) {
        if (obj == (*it)) {
            m_observerlist.erase(it);
            break;
        }
    }
}

void Subject::notify() {
    ObserverList::iterator it = m_observerlist.begin();
    for (; it != m_observerlist.end(); ++it) {
        (*it)->update(this);
    }
}

void Subject::removeObserver(Observer *obj) {
    SubjectList::iterator it = s_subjectlist.begin();
    for(; it != s_subjectlist.end(); ++it) {
        (*it)->detach(obj);
    }
}

}; // end namespace FbTk
