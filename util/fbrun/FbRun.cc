// FbRun.cc
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen<at>users.sourceforge.net)
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

// $Id: FbRun.cc,v 1.17 2003/08/25 01:18:00 fluxgen Exp $

#include "FbRun.hh"

#include "App.hh"
#include "EventManager.hh"
#include "Color.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XPM
#include <X11/xpm.h>
#include "fbrun.xpm"
#endif // HAVE_XPM

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include <unistd.h>

#include <iostream>
#include <iterator>
#include <fstream>
#include <cassert>

using namespace std;
FbRun::FbRun(int x, int y, size_t width):
    FbTk::FbWindow((int)DefaultScreen(FbTk::App::instance()->display()), 
                   x, y,
                   width, 10,
                   KeyPressMask | ExposureMask),
    m_font("fixed"),
    m_display(FbTk::App::instance()->display()),
    m_bevel(4),
    m_gc(DefaultGC(m_display, DefaultScreen(m_display))),
    m_end(false),
    m_current_history_item(0),
    m_cursor(XCreateFontCursor(FbTk::App::instance()->display(), XC_xterm)),
    m_start_pos(0),
    m_end_pos(0),
    m_cursor_pos(0),
    m_pixmap(0) {

    setCursor(m_cursor);
    // setting nomaximize in local resize
    resize(width, m_font.height());
    FbTk::EventManager::instance()->add(*this, *this);
    // setup class name
    XClassHint *class_hint = XAllocClassHint();
    if (class_hint == 0)
        throw string("Out of memory");
    class_hint->res_name = "fbrun";
    class_hint->res_class = "FbRun";    
    XSetClassHint(m_display, window(), class_hint);
    
    XFree(class_hint);
#ifdef HAVE_XPM
    Pixmap mask = 0;
    XpmCreatePixmapFromData(m_display,
                            window(),
                            fbrun_xpm,
                            &m_pixmap,
                            &mask,
                            0); // attribs
    if (mask != 0)
        XFreePixmap(m_display, mask);
#endif // HAVE_XPM

    XWMHints wmhints;
    wmhints.flags = IconPixmapHint;
    wmhints.icon_pixmap = m_pixmap;
    XSetWMHints(m_display, window(), &wmhints);
}


FbRun::~FbRun() {
    hide();
    if (m_pixmap != 0)
        XFreePixmap(FbTk::App::instance()->display(), m_pixmap);
}

void FbRun::run(const std::string &command) {
    FbTk::App::instance()->end(); // end application
    m_end = true; // mark end of processing

    // fork and execute program
    if (!fork()) {
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), 0);
        exit(0); //exit child
    }

    hide(); // hide gui
    
    // save command history to file
    if (m_runtext.size() != 0) { // no need to save empty command

        // don't allow duplicates into the history file, first
        // look for a duplicate
        if (m_current_history_item < m_history.size()
            && m_runtext == m_history[m_current_history_item]) {
            // m_current_history_item is the duplicate
        } else {
            m_current_history_item = 0;
            for (; m_current_history_item < m_history.size(); 
                 ++m_current_history_item) {
                if (m_history[m_current_history_item] == m_runtext)
                    break;
            }
        }

        // now m_current_history_item points at the duplicate, or
        // at m_history.size() if no duplicate
        fstream inoutfile(m_history_file.c_str(), ios::in|ios::out);
        if (inoutfile) {
            int i = 0;
            // read past history items before current
            for (string line; !inoutfile.eof() && i < m_current_history_item; i++)
                getline(inoutfile, line);
            // write the history items that come after current
            for (i++; i < m_history.size(); i++)
                inoutfile<<m_history[i]<<endl;
            
            // and append the current one back to the end
            inoutfile<<m_runtext<<endl;
        } else
            cerr<<"FbRun Warning: Can't write command history to file: "<<m_history_file<<endl;
    }

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
    resize(width(), m_font.height() + m_bevel);
    return true;
}

void FbRun::setForegroundColor(const FbTk::Color &color) {
    XSetForeground(m_display, m_gc, color.pixel());
    redrawLabel();
}

void FbRun::setBackgroundColor(const FbTk::Color &color) {
    FbTk::FbWindow::setBackgroundColor(color);
    redrawLabel();
}

void FbRun::setText(const string &text) {
    m_runtext = text;
    redrawLabel();
}

void FbRun::setTitle(const string &title) {
    setName(title.c_str());
}

void FbRun::resize(size_t width, size_t height) {
    FbTk::FbWindow::resize(width, height);    
    setNoMaximize();
}

void FbRun::redrawLabel() {
    clear();
    drawString(m_bevel/2, m_font.ascent() + m_bevel/2,
               m_runtext.c_str(), m_runtext.size());

}

void FbRun::drawString(int x, int y,
                       const char *text, size_t len) {
    assert(m_gc);

    m_font.drawText(window(), screenNumber(), 
                    m_gc, text + m_start_pos, 
                    m_end_pos - m_start_pos, x, y - 2);
    // draw cursor position
    int cursor_pos = m_font.textWidth(text + m_start_pos, m_cursor_pos) + 1;
    drawLine(m_gc, cursor_pos, 0, cursor_pos, m_font.height());
}

void FbRun::keyPressEvent(XKeyEvent &ke) {
    KeySym ks;
    char keychar[1];
    XLookupString(&ke, keychar, 1, &ks, 0);
    // a modifier key by itself doesn't do anything
    if (IsModifierKey(ks)) return;

    if (ke.state) { // a modifier key is down
        if (ke.state == ControlMask) {
            switch (ks) {
            case XK_b:
                cursorLeft();
                break;
            case XK_f:
                cursorRight();
                break;
            case XK_p:
                prevHistoryItem();
                break;
            case XK_n:
                nextHistoryItem();
                break;
            case XK_a:
                cursorHome();
                break;
            case XK_e:
                cursorEnd();
                break;
            case XK_d:
                deleteForward();
                break;
            case XK_k:
                killToEnd();
                break;
            }
        } else if (ke.state == (Mod1Mask | ShiftMask)) {
            switch (ks) {
            case XK_less:
                firstHistoryItem();
                break;
            case XK_greater:
                lastHistoryItem();
                break;
            }
        } else if (ke.state == ShiftMask) {
            if (isprint(keychar[0]))insertCharacter(ks, keychar);
        }
    } else { // no modifier key
        switch (ks) {
        case XK_Escape:
            m_end = true;
            hide();
            FbTk::App::instance()->end(); // end program
            break;
        case XK_Return:
            run(m_runtext);
            m_runtext = ""; // clear text
            break;
        case XK_BackSpace:
            backspace();
            break;
        case XK_Home:
            cursorHome();
            break;
        case XK_End:
            cursorEnd();
            break;
        case XK_Up:
            prevHistoryItem();
            break;
        case XK_Down:
            nextHistoryItem();
            break;
        case XK_Left:
            cursorLeft();
            break;
        case XK_Right:
            cursorRight();
            break;
        case XK_Tab:
            tabCompleteHistory();
            break;
        default:
            if (isprint(keychar[0])) insertCharacter(ks, keychar);
        }
    }
    redrawLabel();
}

void FbRun::exposeEvent(XExposeEvent &ev) {
    redrawLabel();
}

void FbRun::setNoMaximize() {
    // we don't need to maximize this window
    XSizeHints sh;
    sh.flags = PMaxSize | PMinSize;
    sh.max_width = width();
    sh.max_height = height();
    sh.min_width = width();
    sh.min_height = height();
    XSetWMNormalHints(m_display, window(), &sh);
}

void FbRun::prevHistoryItem() {
    if (m_history.size() == 0 || m_current_history_item == 0) {
        XBell(m_display, 0);
    } else {
        m_current_history_item--;
        m_runtext = m_history[m_current_history_item];
        m_cursor_pos = m_end_pos = m_runtext.size();
        adjustStartPos();
    }
}

void FbRun::nextHistoryItem() {
    if (m_current_history_item == m_history.size()) {
        XBell(m_display, 0);
    } else {
        m_current_history_item++;
        if (m_current_history_item == m_history.size()) {
            m_current_history_item = m_history.size();
            m_runtext = "";
            m_start_pos = m_cursor_pos = m_end_pos = 0;
        } else {
            m_runtext = m_history[m_current_history_item];
            m_cursor_pos = m_end_pos = m_runtext.size();
            adjustStartPos();
        }
    }
}

void FbRun::firstHistoryItem() {
    if (m_history.size() == 0 || m_current_history_item == 0) {
        XBell(m_display, 0);
    } else {
        m_current_history_item = 0;
        m_runtext = m_history[m_current_history_item];
        m_cursor_pos = m_end_pos = m_runtext.size();
        adjustStartPos();
    }
}

void FbRun::lastHistoryItem() {
    // actually one past the end
    if (m_history.size() == 0) {
        XBell(m_display, 0);
    } else {
        m_current_history_item = m_history.size();
        m_runtext = "";
        m_start_pos = m_cursor_pos = m_end_pos = 0;
    }
}

void FbRun::tabCompleteHistory() {
    if (m_current_history_item == 0) {
        XBell(m_display, 0);
    } else {
        int history_item = m_current_history_item - 1;
        string prefix = m_runtext.substr(0, m_cursor_pos);
        while (history_item > - 1) {
            if (m_history[history_item].find(prefix) == 0) {
                m_current_history_item = history_item;
                m_runtext = m_history[m_current_history_item];
                adjustEndPos();
                break;
            }
            history_item--;
        }
        if (history_item == -1) XBell(m_display, 0);
    }
}

void FbRun::cursorLeft() {
    if (m_cursor_pos)
        m_cursor_pos--;
    else if (m_start_pos) {
        m_start_pos--;
        adjustEndPos();
    }
}

void FbRun::cursorRight() {
    if (m_start_pos + m_cursor_pos < m_end_pos)
        m_cursor_pos++;
    else if (m_end_pos < m_runtext.size()) {
        m_cursor_pos++;
        m_end_pos++;
        adjustStartPos();
    }
}

void FbRun::cursorHome() {
    m_start_pos = m_cursor_pos = 0;
    adjustEndPos();
}

void FbRun::cursorEnd() {
    m_cursor_pos = m_end_pos = m_runtext.size();
    adjustStartPos();
}

void FbRun::backspace() {
    if (m_start_pos || m_cursor_pos) {
        m_runtext.erase(m_start_pos + m_cursor_pos - 1, 1);
        if (m_cursor_pos)
            m_cursor_pos--;
        else
            m_start_pos--;
        adjustEndPos();
    }
}

void FbRun::deleteForward() {
    if (m_start_pos + m_cursor_pos < m_end_pos) {
        m_runtext.erase(m_start_pos + m_cursor_pos, 1);
        adjustEndPos();
    }
}

void FbRun::killToEnd() {
    if (m_start_pos + m_cursor_pos < m_end_pos) {
        m_runtext.erase(m_start_pos + m_cursor_pos);
        adjustEndPos();
    }
}

void FbRun::insertCharacter(KeySym ks, char *keychar) {
    char in_char[2] = {keychar[0], 0};
    m_runtext.insert(m_start_pos + m_cursor_pos, in_char);
    m_cursor_pos++;
    m_end_pos++;
    if (m_start_pos + m_cursor_pos < m_end_pos)
        adjustEndPos();
    else
        adjustStartPos();
}

void FbRun::adjustEndPos() {
    m_end_pos = m_runtext.size();
    const char *text = m_runtext.c_str();
    int text_width = m_font.textWidth(text + m_start_pos, m_end_pos - m_start_pos);
    while (text_width > width()) {
        m_end_pos--;
        text_width = m_font.textWidth(text + m_start_pos, m_end_pos - m_start_pos);
    }
}

void FbRun::adjustStartPos() {
    const char *text = m_runtext.c_str();
    int text_width = m_font.textWidth(text + m_start_pos, m_end_pos - m_start_pos);
    if (text_width < width()) return;
    int start_pos = 0;
    text_width = m_font.textWidth(text + start_pos, m_end_pos - start_pos);
    while (text_width > width()) {
        start_pos++;
        text_width = m_font.textWidth(text + start_pos, m_end_pos - start_pos);
    }
    // adjust m_cursor_pos according relative to change to m_start_pos
    m_cursor_pos -= start_pos - m_start_pos;
    m_start_pos = start_pos;
}
