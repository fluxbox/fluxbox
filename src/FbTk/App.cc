// App.cc
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

#include "App.hh"
#include "FbString.hh"
#include "Font.hh"
#include "Image.hh"
#include "EventManager.hh"

#include <cstring>
#include <cstdlib>

#include <set>
#include <iostream>

namespace FbTk {

App *App::s_app = 0;

App *App::instance() {
    if (s_app == 0)
        throw std::string("You must create an instance of FbTk::App first!");
    return s_app;
}

App::App(const char *displayname):m_done(false), m_display(0) {
    if (s_app != 0)
        throw std::string("Can't create more than one instance of FbTk::App");
    s_app = this;
    // with recent versions of X11 one needs to specify XSetLocaleModifiers,
    // otherwise no extra events won't be generated.
    const bool setmodifiers = XSetLocaleModifiers("@im=none");
    // this allows the use of std::string.c_str(), which returns 
    // a blank string, rather than a null string, so we make them equivalent
    if (displayname != 0 && displayname[0] == '\0')
        displayname = 0;
    m_display = XOpenDisplay(displayname);
    if (!m_display) {
        if (displayname) {
            throw std::string("Couldn't connect to XServer") + displayname;
        } else {
            throw std::string("Couldn't connect to XServer passing null display");
        }
    }

    FbStringUtil::init();

    m_xim = 0;
    if (setmodifiers && FbStringUtil::haveUTF8()) {
        m_xim = XOpenIM(m_display, NULL, NULL, NULL);
    }
}

App::~App() {
    if (m_display != 0) {

        Font::shutdown();

        XCloseDisplay(m_display);
        m_display = 0;
    }
    s_app = 0;
}

void App::sync(bool discard) {
    XSync(display(), discard);
}

void App::eventLoop() {
    XEvent ev;
    while (!m_done) {
        XNextEvent(display(), &ev);
        EventManager::instance()->handleEvent(ev);
    }
}


void App::end() {
    m_done = true; //end loop in App::eventLoop
}

bool App::setenv(const char* key, const char* value) {

    if (!key || !*key)
        return false;

    static std::set<char*> stored;

    const size_t key_size = strlen(key);
    const size_t value_size = value ? strlen(value) : 0;
    const size_t newenv_size = key_size + value_size + 2;

    char* newenv = new char[newenv_size];
    if (!newenv) {
        return false;
    }


    // someone might have used putenv("key=value") (or setenv()) before. 
    // if getenv("key") succeeds, the returning value points to the address
    // of "value" (right after the "="). this means, that that address 
    // minus "key=" points to the beginning of what was set. if fluxbox 
    // set that variable we have the pointer in "stored" and can dealloc it.
    //
    // we use putenv() to have control over the dealloction (valgrind and
    // other complaint about it)

    char* oldenv = getenv(key);
    if (oldenv) {
        oldenv = oldenv - (key_size + 1);
    }
    if (stored.find(oldenv) == stored.end()) {
        oldenv = NULL;
    }

    // create the new environment
    strncpy(newenv, key, key_size);
    newenv[key_size] = '=';
    if (value_size > 0) {
        strncpy(newenv+key_size+1, value, value_size);
    }
    newenv[newenv_size-1] = 0;

    if (putenv(newenv) == 0) {
        if (oldenv) {
            stored.erase(oldenv);
            delete[] oldenv;
        }
        stored.insert(newenv);
    }
    return true;
}

} // end namespace FbTk
