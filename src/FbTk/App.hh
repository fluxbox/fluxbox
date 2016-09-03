// App.hh
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_APP_HH
#define FBTK_APP_HH

#include <X11/Xlib.h>

namespace FbTk {

///   Main class for applications, every application must create an instance of this class
/**
 * Usage: \n
 * App app; \n
 * ... \n
 * init some other stuff; \n
 * ... \n
 *  main loop starts here: \n
 * app.eventLoop(); \n
 *
 * To end main loop you call App::instance()->end()
 */
class App {
public:
    /// @return singleton instance of App
    static App *instance();
    /// creates a display connection
    explicit App(const char *displayname=0);
    virtual ~App();
    /// display connection
    Display *display() const { return m_display; }
    void sync(bool discard);
    /// starts event loop
    virtual void eventLoop();
    /// forces an end to event loop
    void end();
    bool done() const { return m_done; }
    const XIM &inputModule() const { return m_xim; }

    // the setenv()-routine is not everywhere available and
    // putenv() doesnt manage the strings in the environment
    // and hence we have to do that on our own to avoid memleaking
    static bool setenv(const char* key, const char* value);
private:
    static App *s_app;
    bool m_done;
    Display *m_display;
    XIM m_xim;
};

} // end namespace FbTk

#endif // FBTK_APP_HH
