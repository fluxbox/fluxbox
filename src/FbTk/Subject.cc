// Subject.cc for FbTk
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Subject.hh"
#include "Observer.hh"

#include <algorithm>
#include <functional>

namespace FbTk {

Subject::SubjectList Subject::s_subjectlist;

Subject::Subject():m_notify_mode(false) {
    s_subjectlist.push_back(this);
}

Subject::~Subject() {
    s_subjectlist.erase(std::remove(s_subjectlist.begin(),
                                    s_subjectlist.end(), this));
}

void Subject::attach(Observer *obj) {
    m_observerlist.push_back(obj);
    // no need to have more than one instance of an observer
    m_observerlist.erase(std::unique(m_observerlist.begin(), m_observerlist.end()),
                         m_observerlist.end());
}

void Subject::detach(Observer *obj) {
    if (m_notify_mode)
        m_dead_observers.push_back(obj);
    else {
        m_observerlist.erase(std::remove(m_observerlist.begin(),
                                         m_observerlist.end(), obj),
                             m_observerlist.end());
    }
}

void Subject::notify() {
    m_notify_mode = true;
    std::for_each(m_observerlist.begin(), m_observerlist.end(),
                  std::bind2nd(std::mem_fun(&Observer::update), this));
    m_notify_mode = false;

    // remove dead observers
    if (!m_dead_observers.empty()) {
        std::for_each(m_dead_observers.begin(),
                      m_dead_observers.end(),
                      std::bind1st(std::mem_fun(&Subject::detach), this));
        m_dead_observers.clear();
    }
}

void Subject::removeObserver(Observer *obj) {
    std::for_each(s_subjectlist.begin(), s_subjectlist.end(),
                  std::bind2nd(std::mem_fun(&Subject::detach), obj));

}

}; // end namespace FbTk
