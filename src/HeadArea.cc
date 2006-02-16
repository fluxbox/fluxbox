// HeadArea.cc for Fluxbox Window Manager
// Copyright (c) 2004 - 2006 Mathieu De Zutter (mathieu at dezutter dot org)
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

#include "HeadArea.hh"
#include "Strut.hh"

#include <algorithm>
#include <iostream>

HeadArea::HeadArea() : m_available_workspace_area(new Strut(0,0,0,0,0)) {
}

Strut *HeadArea::requestStrut(int head, int left, int right, int top, int bottom, Strut* next) {
    Strut *str = new Strut(head, left, right, top, bottom, next);
    m_strutlist.push_back(str);
    return str;
}

void HeadArea::clearStrut(Strut *str) {
    if (str == 0)
        return;
    // find strut and erase it
    std::list<Strut *>::iterator pos = std::find(m_strutlist.begin(),
                                                 m_strutlist.end(),
                                                 str);
    if (pos == m_strutlist.end()) {
        std::cerr << "clearStrut() failed because the strut was not found" << std::endl;
        return;
    }

    m_strutlist.erase(pos);
    delete str;
}

/// helper class for for_each in HeadArea::updateAvailableWorkspaceArea()
namespace {
class MaxArea {
public:
    MaxArea(Strut &max_area):m_available_workspace_area(max_area) { }
    void operator ()(const Strut *str) {
        static int left, right, bottom, top;
        left = std::max(m_available_workspace_area.left(), str->left());
        right = std::max(m_available_workspace_area.right(), str->right());
        bottom = std::max(m_available_workspace_area.bottom(), str->bottom());
        top = std::max(m_available_workspace_area.top(), str->top());
        m_available_workspace_area = Strut(0, left, right, top, bottom);
    }
private:
    Strut &m_available_workspace_area;
};

} // end anonymous namespace

bool HeadArea::updateAvailableWorkspaceArea() {
    // find max of left, right, top and bottom and set avaible workspace area

    // clear old area
    Strut oldarea = *(m_available_workspace_area.get());
    m_available_workspace_area.reset(new Strut(0, 0, 0, 0, 0));
    
    // calculate max area
    std::for_each(m_strutlist.begin(),
             m_strutlist.end(),
             MaxArea(*m_available_workspace_area.get()));

    // only notify if the area changed
    return !(oldarea == *(m_available_workspace_area.get()));
}

