// testFont.cc for fbtk test suite
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: testFont.cc,v 1.6 2003/09/11 16:51:03 fluxgen Exp $

#include "App.hh"
#include "FbWindow.hh"
#include "Font.hh"
#include "EventHandler.hh"
#include "EventManager.hh"
#include "GContext.hh"
#include "Color.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <string>
#include <iostream>
using namespace std;

class App:public FbTk::App, public FbTk::EventHandler {
public:
    App(const char *displayname):
        FbTk::App(displayname),
        m_win(DefaultScreen(display()),
              0, 0, 640, 480, KeyPressMask | ExposureMask) { 

        m_win.show();
        m_win.setBackgroundColor(FbTk::Color("white", m_win.screenNumber()));
        FbTk::EventManager::instance()->add(*this, m_win);
    }
    ~App() {
    }
    void keyPressEvent(XKeyEvent &ke) {
        KeySym ks;
        char keychar[1];
        XLookupString(&ke, keychar, 1, &ks, 0);
        if (ks == XK_Escape)
            end();
        else { // toggle antialias
            m_font.setAntialias(!m_font.isAntialias());
            cerr<<boolalpha;
            cerr<<"antialias: "<<m_font.isAntialias()<<endl;
            redraw();
        }
    }

    void exposeEvent(XExposeEvent &event) {
        redraw();
    }

    void redraw() {
        size_t text_w = m_font.textWidth(m_text.c_str(), m_text.size());
        size_t text_h = m_font.height();
        int x = 640/2 - text_w/2;
        int y = 480/2 - text_h/2;
        m_win.clear();
        FbTk::GContext wingc(m_win.drawable());
        
        m_win.drawLine(wingc.gc(),
                       x, y + m_font.descent(),
                       x + text_w, y + m_font.descent());
        m_win.drawLine(wingc.gc(),
                       x, y - text_h, 
                       x + text_w, y - text_h);
        wingc.setForeground(FbTk::Color("red", m_win.screenNumber()));
        m_win.drawLine(wingc.gc(),
                       x, y, x + text_w, y);
        wingc.setForeground(FbTk::Color("black", m_win.screenNumber()));
        m_font.drawText(m_win.drawable(),
                        0, wingc.gc(),
			m_text.c_str(), m_text.size(),
                        x, y);

    }

    FbTk::Font &font() { return m_font; }
    void setText(const std::string& text) { m_text = text; }

private:
    FbTk::FbWindow m_win;
    FbTk::Font m_font;
    string m_text;
};

int main(int argc, char **argv) {
    bool antialias = false;
    bool rotate = false;
    string fontname("fixed");
    string displayname("");
    string text("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-_¯åäöÅÄÖ^~+=`\"!#¤%&/()=¡@£$½¥{[]}¶½§±");
    for (int a=1; a<argc; ++a) {
        if (strcmp("-font", argv[a])==0 && a + 1 < argc) {
            fontname = argv[++a];
        } else if (strcmp("-antialias", argv[a]) == 0) {
            antialias = true;
        } else if (strcmp("-display", argv[a]) == 0 && a + 1 < argc) {
            displayname = argv[++a];
        } else if (strcmp("-text", argv[a]) == 0 && a + 1 < argc) {
            text = argv[++a];
        } else if (strcmp("-rotate", argv[a]) == 0)
            rotate = true;
    
    }

    App app(displayname.c_str());
    app.font().setAntialias(antialias);
    if (!app.font().load(fontname.c_str()))
        cerr<<"Failed to load: "<<fontname<<endl;
	
    app.setText(text);
    if (rotate)
        app.font().rotate(90);

    app.redraw();
    app.eventLoop();
	
}
