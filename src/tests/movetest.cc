// movetest.cc a test app for moving windows
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)

#include <iostream>

#include "../FbTk/FbWindow.hh"
#include "../FbTk/App.hh"
#include "../FbTk/EventManager.hh"
#include "../FbTk/EventHandler.hh"
#include "../FbTk/Color.hh"

using namespace std;
using namespace FbTk;

class Ev: public EventHandler {
public:
    Ev():m_window(0, 
                  0, 0,
                  512, 512,
                  ExposureMask | ButtonPressMask |
                  ButtonReleaseMask | ButtonMotionMask | SubstructureRedirectMask),
         m_drag(m_window,
                0, 0,
                32, 32,
                
                ExposureMask) { /* | ButtonPressMask) {
                ButtonReleaseMask | ButtonMotionMask) { 
                                                  */
        m_window.setName("hello");
        m_window.show();
        m_drag.setBackgroundColor(Color("blue", 0));
        m_drag.show();
        EventManager::instance()->add(*this, m_window);
        EventManager::instance()->add(*this, m_drag);
        drag = false;
    }

    void exposeEvent(XExposeEvent &event) {
        m_drag.clear();
        m_window.clear();
    }
    
    void buttonPressEvent(XButtonEvent &event) {
        if (m_drag != event.subwindow)
            return;
        
        cerr<<"drag!"<<endl;
        drag = true;
        grab_x = event.x - m_drag.x();
        grab_y = event.y - m_drag.y();
    }

    void buttonReleaseEvent(XButtonEvent &event) {
        drag = false;
    }

    void motionNotifyEvent(XMotionEvent &event) {    
        if (!drag)
            return;
        cerr<<"event.x_root: "<<event.x_root<<endl;
        cerr<<"event.y_root: "<<event.y_root<<endl;
        cerr<<"event.x: "<<event.x<<endl;
        cerr<<"event.y: "<<event.y<<endl;
        cerr<<"event.window: "<<hex<<event.window<<endl;
        cerr<<"event.subwindow: "<<event.subwindow<<endl;
        cerr<<"event.root: "<<event.root<<dec<<endl;
        cerr<<"event.is_hint: "<<event.is_hint<<endl;
        cerr<<"event.same_screen: "<<event.same_screen<<endl;
        cerr<<"event.send_event: "<<event.send_event<<endl;
        cerr<<"window pos: "<<m_window.x()<<", "<<m_window.y()<<endl;
        
        m_drag.move(event.x - grab_x,
                    event.y - grab_y);
    }
    int grab_x, grab_y;
    bool drag;
    FbWindow m_window, m_drag;
};

int main() {
    App app;
    Ev ev;
    cerr<<"Drag the blue rectangle! ...please :D"<<endl;
    app.eventLoop();

}
