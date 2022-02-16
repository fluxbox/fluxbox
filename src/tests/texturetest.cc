// texturetest.cc a test app for Textures
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)

#include "FbTk/ImageControl.hh"
#include "FbTk/Color.hh"
#include "FbTk/GContext.hh"
#include "FbTk/FbPixmap.hh"
#include "FbTk/Texture.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/EventHandler.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/Theme.hh"
#include "FbTk/Font.hh"
#include "FbTk/App.hh"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory>
#include <string>

using namespace std;
using namespace FbTk;

class TestTheme: public Theme {
public:
    TestTheme(int screen):Theme(screen) { }
    bool fallback(ThemeItem_base &item) { return false; }
    void reconfigTheme() { } 
};

class Application: public FbTk::FbWindow, public FbTk::EventHandler {
public:
    Application(int box_size, int num):
        FbWindow(0,  0, 0, 640, ((5+box_size)*num)/9 + 5, ExposureMask | KeyPressMask),
        m_box_size(box_size),
        m_num(num),
        m_font("fixed"),
        m_imgctrl(screenNumber()),
        m_background(*this, width(), height(), depth()),
        m_gc(m_background) {
        setName("Texture Test");
        setBackgroundPixmap(m_background.drawable());

        FbTk::EventManager::instance()->add(*this, *this);

        renderPixmaps();

        show();        
    }
    void keyPressEvent(XKeyEvent &ev) {
        //App::instance()->end();
    }
    void exposeEvent(XExposeEvent &ev) {
        clear();
    }

private:

    void renderPixmap(const Texture &text, int x, int y) {
        Pixmap pm = m_imgctrl.renderImage(2*m_box_size, m_box_size,
                                          text);

        m_background.copyArea(pm, m_gc.gc(),
                              0, 0,
                              x, y,
                              2*m_box_size, m_box_size);
        m_imgctrl.removeImage(pm);
    }

    void renderPixmaps() {

        m_gc.setForeground(Color("gray", screenNumber()));

        m_background.fillRectangle(m_gc.gc(),
                                   0, 0,
                                   width(), height());
        // for text color
        m_gc.setForeground(Color("black", screenNumber()));

        const int step_size = m_box_size + 5;
        unsigned int next_x = 5;
        unsigned int next_y = 5;

        TestTheme tm(screenNumber());
        std::unique_ptr<ThemeItem<Texture> > text;
        char value[18];
        for (int i=0; i<m_num; ++i) {
            sprintf(value, "%d", i);
            text.reset(new ThemeItem<Texture> (tm, string("texture") + value, string("Texture") + value));

            ThemeManager::instance().load("test.theme", "");
            renderPixmap(**text.get(), next_x, next_y);

            next_x += (step_size + m_box_size);
            if (next_x + 2*m_box_size > width()) {
                m_font.drawText(m_background,
                                screenNumber(),
                                m_gc.gc(),
                                value, strlen(value),
                                next_x, next_y + m_box_size/2);
                next_x = 5;
                next_y += step_size;
            }

        }


    }


    const int m_box_size;
    const int m_num;
    FbTk::Font m_font;
    ImageControl m_imgctrl;
    FbPixmap m_background;
    FbTk::GContext m_gc;
};

int main(int argc, char **argv) {
    int boxsize= 30;
    int num = 63;
    for (int i=1; i<argc; ++i) {
        if (strcmp(argv[i], "-boxsize") == 0 && i + 1 < argc)
            boxsize = atoi(argv[++i]);
        else if (strcmp(argv[i], "-num") == 0 && i + 1 < argc)
            num = atoi(argv[++i]);
     }
    App realapp;
    Application app(boxsize, num);

    realapp.eventLoop();

}
