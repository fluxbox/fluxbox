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

// $Id: testFont.cc,v 1.2 2002/10/19 10:04:20 fluxgen Exp $

#include "Font.hh"
#include "BaseDisplay.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

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
		XClearWindow(getXDisplay(), m_win);
		GC wingc = DefaultGC(getXDisplay(), 0);
		m_font.drawText(m_win, 0, wingc,
		m_text.c_str(), m_text.size(),
		640/2 - m_font.textWidth(m_text.c_str(), m_text.size())/2, 480/2);

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
	string text("testTEST0123456789,-+.;:\\!{}[]()");
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
