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

// $Id: FbRun.cc,v 1.9 2002/12/05 00:07:39 fluxgen Exp $

#include "FbRun.hh"

#include "App.hh"
#include "EventManager.hh"
#include "Color.hh"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <unistd.h>

#include <iostream>
#include <fstream>

using namespace std;
FbRun::FbRun(int x, int y, size_t width):
    m_font("fixed"),
    m_win((int)0, x, y,  //screen num and position
          width + m_bevel, m_font.height(),  // size
          KeyPressMask|ExposureMask), // eventmask
    m_display(FbTk::App::instance()->display()),
    m_bevel(4),
    m_gc(DefaultGC(m_display, DefaultScreen(m_display))),
    m_end(false),
    m_current_history_item(0) {
    // setting nomaximize in local resize
    resize(width, m_font.height());
    FbTk::EventManager::instance()->registerEventHandler(*this, m_win.window());
}

FbRun::~FbRun() {
    hide();
    FbTk::EventManager::instance()->unregisterEventHandler(m_win.window());
}

void FbRun::run(const std::string &command) {
    //fork and execute program
    if (!fork()) {
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), 0);
        exit(0); //exit child
    }

    hide(); // hide gui
	
    // save command history to file
    if (m_runtext.size() != 0) { // no need to save empty command
        // open file in append mode
        ofstream outfile(m_history_file.c_str(), ios::app);
        if (outfile)
            outfile<<m_runtext<<endl;
        else 
            cerr<<"FbRun Warning: Can't write command history to file: "<<m_history_file<<endl;
    }
    FbTk::App::instance()->end(); // end application
    m_end = true; // mark end of processing
}

bool FbRun::loadHistory(const char *filename) {
    if (filename == 0)
        return false;
    ifstream infile(filename);
    if (!infile) {
        //even though we fail to load file, we should try save to it
        m_history_file = filename;
        return false;
    }
    // clear old history and load new one from file
    m_history.clear();
    // each line is a command
    string line;
    while (!infile.eof()) {
        getline(infile, line);
        if (line.size()) // don't add empty lines
            m_history.push_back(line);
    }
    // set no current histor to display
    m_current_history_item = m_history.size();
    // set history file
    m_history_file = filename;
    return true;
}

bool FbRun::loadFont(const string &fontname) {
    if (!m_font.load(fontname.c_str()))
        return false;

    // resize to fit new font height
    resize(m_win.width(), m_font.height() + m_bevel);
    return true;
}

void FbRun::setForeground(const FbTk::Color &color) {
    XSetForeground(m_display, m_gc, color.pixel());
    redrawLabel();
}

void FbRun::setBackground(const FbTk::Color &color) {
    m_win.setBackgroundColor(color);
    redrawLabel();
}


void FbRun::setText(const string &text) {
    m_runtext = text;
    redrawLabel();
}

void FbRun::setTitle(const string &title) {
    m_win.setName(title.c_str());
}

void FbRun::move(int x, int y) {
    m_win.move(x, y);
}

void FbRun::resize(size_t width, size_t height) {
    m_win.resize(width, height);	
    setNoMaximize();
}

void FbRun::show() {
    m_win.show();
}

void FbRun::hide() {
    m_win.hide();
}

void FbRun::redrawLabel() {
    m_win.clear();
    drawString(m_bevel/2, m_font.ascent() + m_bevel/2,
               m_runtext.c_str(), m_runtext.size());

}

void FbRun::drawString(int x, int y,
                       const char *text, size_t len) {
    assert(m_gc);

    // check right boundary and adjust text drawing
    size_t text_width = m_font.textWidth(text, len);
    size_t startpos = 0;
    if (text_width > m_win.width()) {
        for (; startpos < len; ++startpos) {
            if (m_font.textWidth(text+startpos, len-startpos) < m_win.width())
                break;
        }		
    }

    m_font.drawText(m_win.window(), DefaultScreen(m_display), m_gc, text + startpos, len-startpos, x, y);
}

void FbRun::keyPressEvent(XKeyEvent &ke) {
    KeySym ks;
    char keychar[1];
    XLookupString(&ke, keychar, 1, &ks, 0);
    if (ks == XK_Escape) {
        m_end = true;
        hide();
        FbTk::App::instance()->end(); // end program
        return; // no more processing
    } else if (ks == XK_Return) {
        run(m_runtext);
        m_runtext = ""; // clear text
    } else if (ks == XK_BackSpace) {
        if (m_runtext.size() != 0) { // we can't erase what we don't have ;)
            m_runtext.erase(m_runtext.size()-1);
            redrawLabel();
        }
    } else if (! IsModifierKey(ks) && !IsCursorKey(ks)) {
        m_runtext+=keychar[0]; // append character
        redrawLabel(); 
    } else if (IsCursorKey(ks)) {
		
        switch (ks) {
        case XK_Up:
            prevHistoryItem();
            break;
        case XK_Down:
            nextHistoryItem();
            break;
        }
        redrawLabel();
    }
}

void FbRun::exposeEvent(XExposeEvent &ev) {
    redrawLabel();
}


void FbRun::setNoMaximize() {
    // we don't need to maximize this window
    XSizeHints sh;
    sh.flags = PMaxSize | PMinSize;
    sh.max_width = m_win.width();
    sh.max_height = m_win.height();
    sh.min_width = m_win.width();
    sh.min_height = m_win.height();
    XSetWMNormalHints(m_display, m_win.window(), &sh);
}

void FbRun::prevHistoryItem() {

    if (m_current_history_item > 0 && m_history.size() > 0)
        m_current_history_item--;
    if (m_current_history_item < m_history.size())
        m_runtext = m_history[m_current_history_item];
}

void FbRun::nextHistoryItem() {
    m_current_history_item++;
    if (m_current_history_item >= m_history.size()) {
        m_current_history_item = m_history.size();
        m_runtext = "";
        return;
    } else 
        m_runtext = m_history[m_current_history_item];

}
