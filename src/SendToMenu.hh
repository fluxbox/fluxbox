// SendToMenu.hh for Fluxbox
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

// $Id: SendToMenu.hh,v 1.1 2003/11/27 21:57:17 fluxgen Exp $

#ifndef SENDTOMENU_HH
#define SENDTOMENU_HH

#include "FbMenu.hh"

#include "FbTk/Observer.hh"

class FluxboxWindow;

class SendToMenu:public FbMenu, private FbTk::Observer {
public:
    SendToMenu(FluxboxWindow &win, const char *label);
    virtual ~SendToMenu() { }
private:
    void update(FbTk::Subject *subj);
    FluxboxWindow &m_win;
};

#endif // SENDTOMENU_HH
