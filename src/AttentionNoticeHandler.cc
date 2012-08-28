// AttentionNoticeHandler.cc for fluxbox
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

#include "AttentionNoticeHandler.hh"

#include "Window.hh"
#include "Screen.hh"

#include "FbTk/STLUtil.hh"
#include "FbTk/Timer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/MemFun.hh"

namespace {
class ToggleFrameFocusCmd: public FbTk::Command<void> {
public:
    ToggleFrameFocusCmd(Focusable &client):
        m_client(client),
        m_state(false) {}
    void execute() {
        m_state ^= true;
        m_client.setAttentionState(m_state);
    }
private:
    Focusable &m_client;
    bool m_state;
};

} // end anonymous namespace 


AttentionNoticeHandler::~AttentionNoticeHandler() {
    FbTk::STLUtil::destroyAndClearSecond(m_attentions);
}

void AttentionNoticeHandler::addAttention(Focusable &client) {
    // no need to add already active client
    if (client.isFocused())
        return;

    // Already have a notice for it?
    NoticeMap::iterator it = m_attentions.find(&client);
    if (it != m_attentions.end()) {
        return; 
    }

    using namespace FbTk;

    ResourceManager &res = client.screen().resourceManager();
    std::string res_name = client.screen().name() + ".demandsAttentionTimeout";
    std::string res_alt_name = client.screen().name() + ".DemandsAttentionTimeout";
    Resource<int> *timeout_res = dynamic_cast<Resource<int>* >(res.findResource(res_name));
    if (timeout_res == 0) {
        // no resource, create one and add it to managed resources
        timeout_res = new FbTk::Resource<int>(res, 500, res_name, res_alt_name);
        client.screen().addManagedResource(timeout_res);
    }
    // disable if timeout is zero
    if (**timeout_res == 0) 
        return;

    // setup timer
    RefCount<Command<void> > cmd(new ToggleFrameFocusCmd(client));
    Timer *timer = new Timer();
    timer->setCommand(cmd);
    timer->setTimeout(**timeout_res * FbTk::FbTime::IN_MILLISECONDS);
    timer->fireOnce(false); // will repeat until window has focus
    timer->start();

    m_attentions[&client] = timer; 
    // attach signals that will make notice go away
    join(client.dieSig(), MemFun(*this, &AttentionNoticeHandler::removeWindow));
    join(client.focusSig(), MemFun(*this, &AttentionNoticeHandler::windowFocusChanged));

    // update _NET_WM_STATE atom
    if (client.fbwindow())
        client.fbwindow()->stateSig().emit(*client.fbwindow());
}

void AttentionNoticeHandler::windowFocusChanged(Focusable& win) {
    updateWindow(win, false);
}
void AttentionNoticeHandler::removeWindow(Focusable& win) {
    updateWindow(win, true);
}

void AttentionNoticeHandler::updateWindow(Focusable& win, bool died) {
    // all signals results in destruction of the notice

    delete m_attentions[&win];
    m_attentions.erase(&win);
    win.setAttentionState(false);

    // update _NET_WM_STATE atom if the window is not dead
    FluxboxWindow *fbwin = win.fbwindow();
    if (fbwin && ! died)
        fbwin->stateSig().emit(*fbwin);

}

bool AttentionNoticeHandler::isDemandingAttention(const Focusable &client) {
    NoticeMap::iterator it = m_attentions.begin(), it_end = m_attentions.end();
    for (; it != it_end; ++it) {
        if (it->first == &client)
            return true;
    }
    return false;
}
