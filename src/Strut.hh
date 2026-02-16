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

struct StrutDimensions {
    int left, right, top, bottom;
    StrutDimensions(int l, int r, int t, int b):
        left(l), right(r), top(t), bottom(b) {}
    void max_by(const StrutDimensions& dims) {
        if (dims.left > left)     left   = dims.left;
        if (dims.right  > right)  right  = dims.right;
        if (dims.top    > top)    top    = dims.top;
        if (dims.bottom > bottom) bottom = dims.bottom;
    }
    bool nonzero() const {
        return left != 0 || right != 0 || top != 0 || bottom != 0;
    }
    bool operator==(const StrutDimensions& dims) const {
        return left == dims.left && right  == dims.right
            && top  == dims.top  && bottom == dims.bottom;
    }
};

class Strut {
public:
    Strut(int head, const StrutDimensions& dims, Strut* next = 0)
        :m_head(head), m_dims(dims), m_next(next) { }
    Strut(): m_head(0), m_dims(0,0,0,0), m_next(0) {}
    int head() const { return m_head; }
    StrutDimensions& dims() { return m_dims; }
    const StrutDimensions& dims() const { return m_dims; }
    int left() const { return m_dims.left; }
    int right() const { return m_dims.right; }
    int bottom() const { return m_dims.bottom; }
    int top() const { return m_dims.top; }
    Strut* next() const { return m_next; }
    bool operator == (const Strut &test) const {
        return (head() == test.head() && dims() == test.dims());
    }
private:
    int m_head;
    StrutDimensions m_dims;
    Strut *m_next; ///< link to struts on all heads
};

#endif // STRUT_HH
