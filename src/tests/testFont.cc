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

// $Id: testFont.cc,v 1.5 2002/12/01 13:42:15 rathnor Exp $

#include "Font.hh"
#include "BaseDisplay.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <iostream>
using namespace std;

class App:public BaseDisplay {
public:
    App(const char *displayname):BaseDisplay("app", displayname) { 

        // using screen 0
        m_win = XCreateSimpleWindow(getXDisplay(),
                                    DefaultRootWindow(getXDisplay()),
                                    0, 0,
                                    640, 480,
                                    1,
                                    0,			
                                    0xFFFF);
        XSelectInput(getXDisplay(), m_win,  KeyPressMask|ExposureMask);
        XMapWindow(getXDisplay(), m_win);
    }
    ~App() {
        XDestroyWindow(getXDisplay(), m_win);
    }
	
    void handleEvent(XEvent * const ev) {
        switch (ev->type) {
        case KeyPress:
            KeySym ks;
            char keychar[1];
            XLookupString(&ev->xkey, keychar, 1, &ks, 0);
            if (ks == XK_Escape)
                shutdown();
            else { // toggle antialias
                m_font.setAntialias(!m_font.isAntialias());
                cerr<<boolalpha;
                cerr<<"antialias: "<<m_font.isAntialias()<<endl;
                redraw();
            }
            break;
        case Expose:
            redraw();
            break;
        }
    }
    void redraw() {
        size_t text_w = m_font.textWidth(m_text.c_str(), m_text.size());
        size_t text_h = m_font.height();
        int x = 640/2 - text_w/2;
        int y = 480/2 - text_h/2;
        XClearWindow(getXDisplay(), m_win);
        GC wingc = DefaultGC(getXDisplay(), 0);

        XDrawLine(getXDisplay(), m_win, wingc,
                  x, y + m_font.descent(), x + text_w, y + m_font.descent());	
        XSetForeground(getXDisplay(), wingc, 0xFF00FF); // don't care what color it is
        XDrawLine(getXDisplay(), m_win, wingc,
                  x, y - text_h , x + text_w, y - text_h );
        XSetForeground(getXDisplay(), wingc, 0xFF0000); // don't care what color it is
        XDrawLine(getXDisplay(), m_win, wingc,
                  x, y, x + text_w, y);

        XSetForeground(getXDisplay(), wingc, 0);
        m_font.drawText(m_win, 0, wingc,
			m_text.c_str(), m_text.size(),
                        x, y);

    }
    Window win() const { return m_win; }
    FbTk::Font &font() { return m_font; }
    void setText(const std::string& text) { m_text = text; }
private:
    Window m_win;
    FbTk::Font m_font;
    string m_text;
};

int main(int argc, char **argv) {
    bool antialias = false;
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
        }
    }

    App app(displayname.c_str());
    app.font().setAntialias(antialias);
    if (!app.font().load(fontname.c_str()))
        cerr<<"Failed to load: "<<fontname<<endl;
	
    app.setText(text);
    app.redraw();
    app.eventLoop();
	
}
