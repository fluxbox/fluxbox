// HeadArea.hh for Fluxbox Window Manager
// Copyright (c) 2004 Mathieu De Zutter (mathieu at dezutter.org)
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
 
#ifndef HEADAREA_HH
#define HEADAREA_HH

#include "FbTk/NotCopyable.hh"
#include <memory>
#include <list>

class Strut;

class HeadArea: private FbTk::NotCopyable {
public:
    HeadArea();

    Strut *requestStrut(int head, int left, int right, int top, int bottom, Strut* next = 0);
    void clearStrut(Strut *str);
    bool updateAvailableWorkspaceArea();
    const Strut *availableWorkspaceArea() const {
        return m_available_workspace_area.get();
    }

private:
    std::unique_ptr<Strut> m_available_workspace_area;
    std::list<Strut*> m_strutlist;
};

#endif // HEADAREA_HH
