// SetWorkspaceName.hh for Fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: SetWorkspaceName.hh,v 1.2 2003/08/27 18:05:12 fluxgen Exp $

#ifndef SETWORKSPACENAME_HH
#define SETWORKSPACENAME_HH

#include "FbTk/TextBox.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/Font.hh"
#include "FbTk/GContext.hh"

class BScreen;

class SetWorkspaceName: public FbTk::FbWindow, public FbTk::EventHandler {
public:
    explicit SetWorkspaceName(BScreen &screen);
    virtual ~SetWorkspaceName();

    void show();
    void hide();

    void motionNotifyEvent(XMotionEvent &event);
    void buttonPressEvent(XButtonEvent &event);
    void handleEvent(XEvent &event);
    void keyPressEvent(XKeyEvent &event);

private:
    FbTk::TextBox m_textbox;
    FbTk::TextButton m_label;
    FbTk::Font m_font;
    FbTk::GContext m_gc;
    BScreen &m_screen;
    int m_move_x, m_move_y;
};


#endif // SETWORKSPACENAME_HH
