// FbRun.hh
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

// $Id: FbRun.hh,v 1.1 2002/08/20 02:04:34 fluxgen Exp $

#ifndef FBRUN_HH
#define FBRUN_HH

#include "EventHandler.hh"
#include "Font.hh"

#include <string>

/**
	Creates and managed a run window
	TODO: a command history
*/
class FbRun: public FbTk::EventHandler<XEvent> {
public:
	FbRun(Display *disp, int x = 0, int y = 0, size_t width = 200);
	~FbRun();
	void handleEvent(XEvent * const ev);
	void setText(const std::string &text);
	void setTitle(const std::string &title);
	void move(int x, int y);
	void resize(size_t width, size_t height);
	/// hide window
	void hide();
	/// show window
	void show();
	/// load and reconfigure for new font
	bool loadFont(const std::string &fontname);
	void setForeground(const XColor &color);
	void setBackground(const XColor &color);
	const FbTk::Font &font() const { return m_font; }
	/// execute command and exit
	void run(const std::string &execstring);
	/// is this object done?
	bool end() const { return m_end; }
private:
	void drawString(int x, int y, const char *text, size_t len);
	void getSize(size_t &width, size_t &height);
	void createWindow(int x, int y, size_t width, size_t height);	
	void redrawLabel();	
	/// set no maximizable for this window
	void setNoMaximize();

	FbTk::Font m_font; ///< font used to draw command text
	Window m_win;  ///< toplevel window 
	Display *m_display;  ///< display connection
	std::string m_runtext; ///< command to execute
	size_t m_width, m_height; ///< size of window
	int m_bevel; ///< distance to window edge from font in pixels
	GC m_gc;
	bool m_end;
};

#endif // FBRUN_HH
