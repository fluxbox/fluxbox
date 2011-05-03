// AttentionNoticeHandler.hh for fluxbox
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ATTENTIONNOTICEHANDLER_HH
#define ATTENTIONNOTICEHANDLER_HH

#include <map>

#include "FbTk/Signal.hh"

class Focusable;

namespace FbTk {
class Timer;
}
/** 
 * Handles demands attention signals.
 * Makes the title and iconbutton flash when the window 
 * demands attention.
 */
class AttentionNoticeHandler: private FbTk::SignalTracker {
public:
    ~AttentionNoticeHandler();

    typedef std::map<Focusable*, FbTk::Timer*> NoticeMap;
    /// Adds a client that requires attention,
    /// will fail if the client is already active
    void addAttention(Focusable &client); 

    bool isDemandingAttention(const Focusable &client);

    /// Called when window focus changes.
    void windowFocusChanged(Focusable& win);
    /// Remove window from attentionHandler.
    void removeWindow(Focusable& win);

private:
    /// updates the windows state in this instance.
    void updateWindow(Focusable& win, bool died);

    NoticeMap m_attentions;
};

#endif // ATTENTIONNOTICEHANDLER_HH
