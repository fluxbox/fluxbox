// FbRun.cc
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

#include "FbRun.hh"

#include "FbTk/App.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/Color.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/FileUtil.hh"

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
#include <algorithm>

#ifdef _WIN32
#include <cstring>
#endif

using std::cerr;
using std::endl;
using std::string;
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::ios;

FbRun::FbRun(int x, int y, size_t width):
    FbTk::TextBox(DefaultScreen(FbTk::App::instance()->display()),
                  m_font, ""),
    m_print(false),
    m_font("fixed"),
    m_display(FbTk::App::instance()->display()),
    m_bevel(4),
    m_padding(0),
    m_gc(*this),
    m_end(false),
    m_current_history_item(0),
    m_current_files_item(-1),
    m_last_completion_path(""),
    m_current_apps_item(-1),
    m_completion_pos(std::string::npos),
    m_cursor(XCreateFontCursor(FbTk::App::instance()->display(), XC_xterm)) {

    setGC(m_gc.gc());
    setCursor(m_cursor);
    // setting nomaximize in local resize
    resize(width, font().height() + m_bevel);

    // setup class name
    XClassHint ch;
    ch.res_name = const_cast<char *>("fbrun");
    ch.res_class = const_cast<char *>("FbRun");
    XSetClassHint(m_display, window(), &ch);

#ifdef HAVE_XPM
    Pixmap mask = 0;
    Pixmap pm;
    XpmCreatePixmapFromData(m_display,
                            window(),
                            const_cast<char **>(fbrun_xpm),
                            &pm,
                            &mask,
                            0); // attribs
    if (mask != 0)
        XFreePixmap(m_display, mask);

    m_pixmap = pm;
#endif // HAVE_XPM

    if (m_pixmap.drawable()) {
        XWMHints wmhints;
        wmhints.flags = IconPixmapHint;
        wmhints.icon_pixmap = m_pixmap.drawable();
        XSetWMHints(m_display, window(), &wmhints);
    }

}


FbRun::~FbRun() {
    hide();
}

void FbRun::run(const std::string &command) {
    FbTk::App::instance()->end(); // end application
    m_end = true; // mark end of processing

    hide(); // hide gui

    if (m_print) {
        std::cout << command;
        return;
    }

    if (command.empty()) {
        return;
    }

#ifdef HAVE_FORK
    // fork and execute program
    if (!fork()) {

        const char *shell = getenv("SHELL");
        if (!shell)
            shell = "/bin/sh";

        setsid();
        execl(shell, shell, "-c", command.c_str(), static_cast<void*>(NULL));
        exit(0); //exit child
    }
#elif defined(_WIN32)
	/// @todo - unduplicate from FbCommands.cc
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
    char comspec[PATH_MAX] = {0};
    char * env_var = getenv("COMSPEC");
    if (env_var != NULL) {
        strncpy(comspec, env_var, PATH_MAX - 1);
        comspec[PATH_MAX - 1] = '\0';
    } else {
        strncpy(comspec, "cmd.exe", 7);
        comspec[7] = '\0';
    }

    spawnlp(P_NOWAIT, comspec, comspec, "/c", command.c_str(), static_cast<void*>(NULL));

#else
#error "Can't build FbRun - don't know how to launch without fork on your platform"
#endif

    ofstream outfile(m_history_file.c_str());
    if (!outfile) {
        cerr<<"FbRun Warning: Can't write command history to file: "<<m_history_file<<endl;
        return;
    }

    int n = 1024;
    char *a = getenv("FBRUN_HISTORY_SIZE");
    if (a)
        n = atoi(a);
    int j = m_history.size();
    --n; // NOTICE: this should be "-=2", but a duplicate entry in the late
         // (good) section would wait "too" long
         // (we'd wait until 3 items are left and then still skip one for being a dupe)
         // IOW: the limit is either n or n+1, depending in the history structure
    for (unsigned int i = 0; i != m_history.size(); ++i) {
        // don't allow duplicates into the history file
        if (--j > n || m_history[i] == command)
            continue;
        outfile<<m_history[i]<<endl;
    }
    if (++n > 0) // n was decremented for the loop
        outfile << command << endl;
    outfile.close();
}


bool FbRun::loadHistory(const char *filename) {
    if (filename == 0)
        return false;
    ifstream infile(filename);
    if (!infile) {
        //even though we fail to load file, we should try save to it
        ofstream outfile(filename);
        if (outfile) {
            m_history_file = filename;
            return true;
        }
        return false;
    }

    m_history.clear();
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) // don't add empty lines
            m_history.push_back(line);
    }
    // set no current histor to display
    m_current_history_item = m_history.size();
    // set history file
    m_history_file = filename;
    return true;
}

bool FbRun::loadCompletion(const char *filename) {
    if (!filename)
        return false;
    ifstream infile(filename);
    if (!infile)
        return false;

    m_apps.clear();
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) // don't add empty lines
            m_apps.push_back(line);
    }
    return true;
}


bool FbRun::loadFont(const string &fontname) {
    if (!m_font.load(fontname.c_str()))
        return false;

    // resize to fit new font height
    resize(width(), font().height() + m_bevel);
    return true;
}

void FbRun::setForegroundColor(const FbTk::Color &color) {
    m_gc.setForeground(color);
}

void FbRun::setTitle(const string &title) {
    setName(title.c_str());
}

void FbRun::resize(unsigned int width, unsigned int height) {
    FbTk::TextBox::resize(width, height);
}

void FbRun::setPadding(int padding) {
    m_padding = padding;
    FbTk::TextBox::setPadding(padding);
}

void FbRun::redrawLabel() {
    clear();
}

void FbRun::keyPressEvent(XKeyEvent &ke) {
    // reset last completion prefix if we don't do a tab completion thing
    bool did_tab_complete = false;

    ke.state = FbTk::KeyUtil::instance().cleanMods(ke.state);

    FbTk::TextBox::keyPressEvent(ke);
    KeySym ks;
    char keychar[1];
    XLookupString(&ke, keychar, 1, &ks, 0);
    // a modifier key by itself doesn't do anything
    if (IsModifierKey(ks))
        return;

    if (m_autocomplete && isprint(keychar[0])) {
        did_tab_complete = true;
        if (m_completion_pos == std::string::npos) {
            m_completion_pos = cursorPosition();
        } else {
            ++m_completion_pos;
        }
        tabCompleteApps();
    } else if (FbTk::KeyUtil::instance().isolateModifierMask(ke.state)) {
        // a modifier key is down
        if ((ke.state & ControlMask) == ControlMask) {
            switch (ks) {
            case XK_p:
                did_tab_complete = true;
                prevHistoryItem();
                break;
            case XK_n:
                did_tab_complete = true;
                nextHistoryItem();
                break;
            case XK_Tab:
                did_tab_complete = true;
                tabComplete(m_history, m_current_history_item, true); // reverse
                break;
            }
        } else if ((ke.state & (Mod1Mask|ShiftMask)) == (Mod1Mask | ShiftMask)) {
            switch (ks) {
            case XK_less:
                did_tab_complete = true;
                firstHistoryItem();
                break;
            case XK_greater:
                did_tab_complete = true;
                lastHistoryItem();
                break;
            }
        }
    } else { // no modifier key
        switch (ks) {
        case XK_Escape:
            m_end = true;
            hide();
            FbTk::App::instance()->end(); // end program
            break;
        case XK_KP_Enter:
        case XK_Return:
            run(text());
            break;
        case XK_Up:
            prevHistoryItem();
            break;
        case XK_Down:
            nextHistoryItem();
            break;
        case XK_Tab:
            did_tab_complete = true;
            tabCompleteApps();
            break;
        }
    }
    if (!did_tab_complete)
        m_completion_pos = std::string::npos;
    clear();
}

void FbRun::lockPosition(bool size_too) {
    // we don't need to maximize this window
    XSizeHints sh;
    sh.flags = PMaxSize | PMinSize;
    sh.max_width = width();
    sh.max_height = height();
    sh.min_width = width();
    sh.min_height = height();
    if (size_too) {
        sh.flags |= USPosition;
        sh.x = x();
        sh.y = y();
    }
    XSetWMNormalHints(m_display, window(), &sh);
}

void FbRun::prevHistoryItem() {
    if (m_history.empty() || m_current_history_item == 0) {
        XBell(m_display, 0);
    } else {
        m_current_history_item--;
        setText(m_history[m_current_history_item]);
    }
}

void FbRun::nextHistoryItem() {
    if (m_current_history_item == m_history.size()) {
        XBell(m_display, 0);
    } else {
        m_current_history_item++;
        FbTk::BiDiString text("");
        if (m_current_history_item == m_history.size()) {
            m_current_history_item = m_history.size();
        } else
            text.setLogical((m_history[m_current_history_item]));

        setText(text);
    }
}

void FbRun::firstHistoryItem() {
    if (m_history.empty() || m_current_history_item == 0) {
        XBell(m_display, 0);
    } else {
        m_current_history_item = 0;
        setText(FbTk::BiDiString(m_history[m_current_history_item]));
    }
}

void FbRun::lastHistoryItem() {
    // actually one past the end
    if (m_history.empty()) {
        XBell(m_display, 0);
    } else {
        m_current_history_item = m_history.size();
        setText(FbTk::BiDiString(""));
    }
}

void FbRun::tabComplete(const std::vector<std::string> &list, int &currentItem, bool reverse) {
    if (list.empty()) {
        XBell(m_display, 0);
        return;
    }

    if (m_completion_pos == std::string::npos)
        m_completion_pos = textStartPos() + cursorPosition();
    size_t split = text().find_last_of(' ', m_completion_pos);
    if (split == std::string::npos)
        split = 0;
    else
        ++split; // skip space
    std::string prefix = text().substr(split, m_completion_pos - split);

    if (currentItem < 0)
        currentItem = 0;
    else if (currentItem >= list.size())
        currentItem = list.size() - 1;
    int item = currentItem;

    while (true) {
        if (reverse) {
            if (--item < 0)
                item = list.size() - 1;
        } else {
            if (++item >= list.size())
                item = 0;
        }
        if (list.at(item).find(prefix) == 0) {
            setText(FbTk::BiDiString(text().substr(0, split) + list.at(item)));
            if (item == currentItem) {
                cursorEnd();
                m_completion_pos = std::string::npos;
            } else {
                select(split + prefix.size(), text().size() - (prefix.size() + split));
            }
            currentItem = item;
            return;
        }
        if (item == currentItem) {
            cursorEnd();
            m_completion_pos = std::string::npos;
            return;
        }
    }
    // found nothing
    XBell(m_display, 0);
}


void FbRun::tabCompleteApps() {
    if (m_completion_pos == std::string::npos)
        m_completion_pos = textStartPos() + cursorPosition();
    size_t split = text().find_last_of(' ', m_completion_pos);
    if (split == std::string::npos)
        split = 0;
    else
        ++split; // skip the space
    std::string prefix = text().substr(split, m_completion_pos - split);
    if (prefix.empty()) {
        XBell(m_display, 0);
        return;
    }
    if (prefix.at(0) == '/' || prefix.at(0) == '.' || prefix.at(0) == '~') {
        // we're completing a directory, find subdirs
        split = prefix.find_last_of('/');
        if (split == std::string::npos) {
            split = prefix.size();
            prefix.append("/");
        }
        prefix = prefix.substr(0, split+1);
        if (prefix != m_last_completion_path) {
            m_files.clear();
            m_current_files_item = -1;
            m_last_completion_path = prefix;

            FbTk::Directory dir;
            std::string path = prefix;
            if (path.at(0) == '~')
                path.replace(0,1,getenv("HOME"));
            dir.open(path.c_str());
            int n = dir.entries();
            while (--n > -1) {
                std::string entry = dir.readFilename();
                if (entry == "." || entry == "..")
                    continue;
                // escape special characters
                std::string needle(" !\"$&'()*,:;<=>?@[\\]^`{|}");
                std::size_t pos = 0;
                while ((pos = entry.find_first_of(needle, pos)) != std::string::npos) {
                    entry.insert(pos, "\\");
                    pos += 2;
                }
                if (FbTk::FileUtil::isDirectory(std::string(path + entry).c_str()))
                    m_files.push_back(prefix + entry + "/");
                else
                    m_files.push_back(prefix + entry);
            }
            dir.close();
            sort(m_files.begin(), m_files.end());
        }
        tabComplete(m_files, m_current_files_item);
    } else {
        static bool first_run = true;
        if (first_run && m_apps.empty()) {
            first_run = false;
            std::string path = getenv("PATH");
            FbTk::Directory dir;
            for (unsigned int l = 0, r = 0; r < path.size(); ++r) {
                if ((path.at(r) == ':' || r == path.size() - 1) && r - l > 1) {
                    dir.open(path.substr(l, r - l).c_str());
                    prefix = dir.name() + (*dir.name().rbegin() == '/' ? "" : "/");
                    int n = dir.entries();
                    while (--n > -1) {
                        std::string entry = dir.readFilename();
                        std::string file = prefix + entry;
                        if (FbTk::FileUtil::isExecutable(file.c_str()) &&
                                     !FbTk::FileUtil::isDirectory(file.c_str())) {
                            m_apps.push_back(entry);
                        }
                    }
                    dir.close();
                    l = r + 1;
                }
            }
            sort(m_apps.begin(), m_apps.end());
            unique(m_apps.begin(), m_apps.end());
        }
        tabComplete(m_apps, m_current_apps_item);
    }
}

void FbRun::insertCharacter(char keychar) {
    char val[2] = {keychar, 0};
    insertText(val);
}
