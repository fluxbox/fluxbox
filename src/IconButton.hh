// IconButton.hh
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: IconButton.hh,v 1.4 2003/11/27 14:27:48 fluxgen Exp $

#ifndef ICONBUTTON_HH
#define ICONBUTTON_HH

#include "FbTk/FbPixmap.hh"
#include "FbTk/Observer.hh"
#include "FbTk/TextButton.hh"

class FluxboxWindow;

class IconButton: public FbTk::TextButton, public FbTk::Observer {
public:
    IconButton(const FbTk::FbWindow &parent, const FbTk::Font &font, 
               FluxboxWindow &window);
    virtual ~IconButton();

    void exposeEvent(XExposeEvent &event);
    void clear();
    void clearArea(int x, int y,
                   unsigned int width, unsigned int height,
                   bool exposure = false);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void resize(unsigned int width, unsigned int height);

    void update(FbTk::Subject *subj);
    void setPixmap(bool use);

    FluxboxWindow &win() { return m_win; }
    const FluxboxWindow &win() const { return m_win; }

protected:
    void drawText(int x = 0, int y = 0);
private:
    void setupWindow();

    FluxboxWindow &m_win;
    FbTk::FbWindow m_icon_window;
    FbTk::FbPixmap m_icon_pixmap;
    FbTk::FbPixmap m_icon_mask;
    bool m_use_pixmap;
};

#endif // ICONBUTTON_HH
