// Strut.hh for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                     and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef STRUT_HH
#define STRUT_HH

class Strut {
public:
    Strut(int head, int left, int right,
          int top, int bottom, Strut* next = 0)
        :m_head(head), m_left(left), m_right(right),
         m_top(top), m_bottom(bottom), m_next(next) { }
    int head() const { return m_head; }
    int left() const { return m_left; }
    int right() const { return m_right; }
    int bottom() const { return m_bottom; }
    int top() const { return m_top; }
    Strut* next() const { return m_next; }
    bool operator == (const Strut &test) const {
        return (head() == test.head() &&
                left() == test.left() &&
                right() == test.right() &&
                top() == test.top() &&
                bottom() == test.bottom());
    }
private:
    Strut():m_head(0), m_left(0), m_right(0), m_top(0), m_bottom(0), m_next(0) {}
    int m_head;
    int m_left, m_right, m_top, m_bottom;
    Strut *m_next; ///< link to struts on all heads
};

#endif // STRUT_HH
