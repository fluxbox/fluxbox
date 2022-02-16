// SpacerTool.hh for Fluxbox
// Copyright (c) 2016 Thomas Lübking <thomas.luebking@gmail.com>
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

#ifndef SPACERTOOL_HH
#define SPACERTOOL_HH

#include "ToolbarItem.hh"


class SpacerTool: public ToolbarItem {
public:
    SpacerTool(int size = -1);
    virtual ~SpacerTool();
    void move(int x, int y) {}
    void resize(unsigned int x, unsigned int y) {}
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height) {}
    void show() {}
    void hide() {}

    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const {return 0;}

    void parentMoved() {}
    void updateSizing() {}

    virtual void renderTheme(int alpha) {}

private:
    int m_size;
};

#endif // SPACERTOOL_HH
