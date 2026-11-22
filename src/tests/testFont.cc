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
#include "FbTk/FbString.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/Font.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/GContext.hh"
#include "FbTk/Color.hh"
#include "FbTk/FbString.hh"
#include "FbTk/StringUtil.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace std;


class App:public FbTk::App, public FbTk::FbWindow, public FbTk::EventHandler {
public:
    App(const char *displayname, const string& foreground, const string& background):
        FbTk::App(displayname),
        FbTk::FbWindow(DefaultScreen(this->FbTk::App::display()), 0, 0, 640, 480, KeyPressMask|ExposureMask|StructureNotifyMask),
        m_gc(drawable()),
        m_foreground(foreground.c_str(), screenNumber()),
        m_background(background.c_str(), screenNumber()),
        m_orient(FbTk::ROT0) {

        m_gc.setLineAttributes(1, FbTk::GContext::JOINMITER, FbTk::GContext::LINESOLID, FbTk::GContext::CAPNOTLAST);
        m_gc.setForeground(m_foreground);
        m_gc.setBackground(m_background);
        setBackgroundColor(m_background);

        FbTk::EventManager::instance()->add(*this, *this);
    }

    ~App() { }

    void keyPressEvent(XKeyEvent &ke) {
        KeySym ks;
        char keychar[1];
        XLookupString(&ke, keychar, 1, &ks, 0);
        if (ks == XK_Escape) {
            end();
        }
    }

    virtual void handleEvent(XEvent& event) {
        if (event.type == ConfigureNotify) {
            resize(event.xconfigure.width, event.xconfigure.height);
        }
    }


    void exposeEvent(XExposeEvent &event) {
        redraw();
    }

    void redraw() {
        size_t i;
        for (i = 0; i < m_buttons.size(); ++i) {
            FbTk::TextButton* b = m_buttons[i];
            b->clear();
            b->drawLine(m_gc.gc(), 0, b->height() / 2, b->width(), b->height() / 2);
        }
        this->clear();
    }

    void resize(unsigned int width, unsigned int height) {
        FbTk::FbWindow::resize(width, height);
        if (m_buttons.empty())
            return;
        // one row per text; rotated texts run vertically, so one column each
        bool rows = m_orient == FbTk::ROT0 || m_orient == FbTk::ROT180;
        unsigned w = rows ? width : width / m_buttons.size();
        unsigned h = rows ? height / m_buttons.size() : height;
        size_t i;
        for (i = 0; i < m_buttons.size(); ++i) {
            m_buttons[i]->moveResize(rows ? 0 : i * w, rows ? i * h : 0, w, h);
        }
        redraw();
    }

    void addText(const FbTk::BiDiString& text, FbTk::Font& font, const FbTk::Orientation orient) {

        FbTk::FbWindow* win = this;
        FbTk::TextButton* button = new FbTk::TextButton(*win, font, text);

        button->setGC(m_gc.gc());
        button->setOrientation(orient);
        button->setBackgroundColor(m_background);
        button->show();

        m_buttons.push_back(button);
        m_orient = orient;
    }

private:
    vector<FbTk::TextButton*> m_buttons;
    FbTk::GContext m_gc;
    FbTk::Color m_foreground;
    FbTk::Color m_background;
    FbTk::Orientation m_orient;
};



int main(int argc, char **argv) {

    vector<string> texts_and_fonts;
    FbTk::Orientation orient = FbTk::ROT0;
    string displayname("");
    string background("white");
    string foreground("black");

    int a;
    for (a = 1; a < argc; ++a) {
        if (strcmp("-display", argv[a]) == 0 && a + 1 < argc) {
            displayname = argv[++a];
        } else if (strcmp("-orient", argv[a]) == 0) {
            orient = (FbTk::Orientation) (atoi(argv[++a]) % 4);
        } else if (strcmp("-bg", argv[a]) == 0 && a + 1 < argc) {
            background = argv[++a];
        } else if (strcmp("-fg", argv[a]) == 0 && a + 1 < argc) {
            foreground = argv[++a];
        } else if (strcmp("-h", argv[a]) == 0) {
            cerr<<"Arguments: \"text|fontname\" [\"text|fontname2\"]"<<endl;
            cerr<<"-display <display>"<<endl;
            cerr<<"-orient"<<endl;
            cerr<<"-fg <foreground color>"<<endl;
            cerr<<"-bg <background color>"<<endl;
            cerr<<"-h"<<endl;
            exit(0);
        } else {
            texts_and_fonts.push_back(argv[a]);
        }
    }

    if (texts_and_fonts.empty()) {
        texts_and_fonts.push_back("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-_¯åäöÅÄÖ^~+=`\"!#¤%&/()=¡@£$½¥{[]}¶½§±|default");
    }

    App app(displayname.c_str(), foreground, background);
    app.show();

    for (a = 0; a < texts_and_fonts.size(); ++a) {

        // split at the last '|', the text itself may contain one
        const string& arg = texts_and_fonts[a];
        size_t bar = arg.rfind('|');
        vector<string> tf;
        tf.push_back(arg.substr(0, bar));
        tf.push_back(bar == string::npos ? "default" : arg.substr(bar + 1));

        FbTk::Font* f = new FbTk::Font(0);
        if (f->load(tf[1])) {

            if (orient && !f->validOrientation(orient)) {
                cerr<<"Orientation not valid ("<<orient<<")"<<endl;
                orient = FbTk::ROT0;
            }

            app.addText(FbTk::FbStringUtil::LocaleStrToFb(tf[0]), *f, orient);

        } else {
            cerr<<"Failed to load: "<<tf[1]<<endl;
            delete f;
        }
    }

    app.resize(app.width(), app.height());
    app.redraw();
    app.eventLoop();

    return 0;
}

