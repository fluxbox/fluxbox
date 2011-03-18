// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "FbTk/App.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/Font.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/GContext.hh"
#include "FbTk/Color.hh"
#include "FbTk/FbString.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace std;

class App:public FbTk::App, public FbTk::EventHandler {
public:
    App(const char *displayname, const string &foreground, const string background):
        FbTk::App(displayname),
        m_win(DefaultScreen(display()),
              0, 0, 640, 480, KeyPressMask | ExposureMask) { 
        m_background = background;
        m_foreground = foreground;
        m_orient = FbTk::ROT0;
        m_win.show();
        m_win.setBackgroundColor(FbTk::Color(background.c_str(), m_win.screenNumber()));
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
        /*
        else { // toggle antialias
            m_font.setAntialias(!m_font.isAntialias());
            cerr<<boolalpha;
            cerr<<"antialias: "<<m_font.isAntialias()<<endl;
            redraw();
        } */
    }

    void exposeEvent(XExposeEvent &event) {
        redraw();
    }

    void redraw() {
        size_t text_w = m_font.textWidth(m_text.c_str(), m_text.size());
        int mult = 1;
        if (m_orient == FbTk::ROT180)
            mult = -1;
        size_t text_h = m_font.height();
        int x = 640/2 - mult* text_w/2;
        int y = 480/2 - mult*text_h/2;
        m_win.clear();
        FbTk::GContext wingc(m_win.drawable());

        int bx1 = 0;
        int by1 = 0;
        int bx2 = text_w;
        int by2 = 0;

        switch (m_orient) {
        case FbTk::ROT90:
            by2 = bx2;
            bx2 = 0;
            break;
        case FbTk::ROT180:
            bx2 = -bx2;
            break;
        case FbTk::ROT270:
            by2 = -bx2;
            bx2 = 0;
            break;
        default:
            break;
        }

/*
        m_win.drawLine(wingc.gc(),
                       x, y + m_font.descent(),
                       x + text_w, y + m_font.descent());
        m_win.drawLine(wingc.gc(),
                       x, y - text_h, 
                       x + text_w, y - text_h);
*/
        // draw the baseline in red
        wingc.setForeground(FbTk::Color("red", m_win.screenNumber()));
        m_win.drawLine(wingc.gc(),
                       x + bx1, y + by1, x + bx2, y+by2);
        wingc.setForeground(FbTk::Color(m_foreground.c_str(), m_win.screenNumber()));
        cerr<<"text size "<<text_w<<"x"<<text_h<<endl;
        m_font.drawText(m_win,
                        0, wingc.gc(),
			m_text.c_str(), m_text.size(),
                        x, y, m_orient);

    }

    FbTk::Font &font() { return m_font; }
    void setText(const std::string& text, const FbTk::Orientation orient) { m_text = text; m_orient = orient; }

private:
    string m_foreground, m_background;
    FbTk::FbWindow m_win;
    FbTk::Font m_font;
    FbTk::Orientation m_orient;
    string m_text;
};

int main(int argc, char **argv) {
    //bool antialias = false;
    FbTk::Orientation orient = FbTk::ROT0;
    bool xft = false;
    string fontname("");
    string displayname("");
    string background("black");
    string foreground("white");
    string text("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-_¯åäöÅÄÖ^~+=`\"!#¤%&/()=¡@£$½¥{[]}¶½§±");
    for (int a=1; a<argc; ++a) {
        if (strcmp("-font", argv[a])==0 && a + 1 < argc) {
            fontname = argv[++a];
        } else if (strcmp("-xft", argv[a])==0) {
            xft = true;
        } else if (strcmp("-display", argv[a]) == 0 && a + 1 < argc) {
            displayname = argv[++a];
        } else if (strcmp("-text", argv[a]) == 0 && a + 1 < argc) {
            text = argv[++a];
        } else if (strcmp("-orient", argv[a]) == 0) {
            orient = (FbTk::Orientation) (atoi(argv[++a]) % 4);
        } else if (strcmp("-bg", argv[a]) == 0 && a + 1 < argc) {
            background = argv[++a];
        } else if (strcmp("-fg", argv[a]) == 0 && a + 1 < argc) {
            foreground = argv[++a];
        } else if (strcmp("-h", argv[a]) == 0) {
            cerr<<"Arguments: "<<endl;
            cerr<<"-font <fontname>"<<endl;
         //   cerr<<"-antialias"<<endl;
            cerr<<"-display <display>"<<endl;
            cerr<<"-text <text>"<<endl;
            cerr<<"-orient"<<endl;
            cerr<<"-fg <foreground color>"<<endl;
            cerr<<"-bg <background color>"<<endl;
            cerr<<"-h"<<endl;
            exit(0);
        }
            
    }
    

    App app(displayname.c_str(), foreground, background);
    //app.font().setAntialias(antialias);
    if (!app.font().load(fontname.c_str()))
        cerr<<"Failed to load: "<<fontname<<endl;
    if (orient && !app.font().validOrientation(orient)) {
        cerr<<"Orientation not valid ("<<orient<<")"<<endl;
        orient = FbTk::ROT0;
    }
    // utf-8 it

    cerr<<"Setting text: "<<text<<endl;
    app.setText(FbTk::FbStringUtil::XStrToFb(text), orient);

    app.redraw();
    app.eventLoop();
	
}
