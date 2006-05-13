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

// $Id$

#include "AttentionNoticeHandler.hh"

#include "Window.hh"
#include "Screen.hh"
#include "STLUtil.hh"

#include "FbTk/Subject.hh"
#include "FbTk/Timer.hh"
#include "FbTk/Resource.hh"

namespace {
class ToggleFrameFocusCmd: public FbTk::Command {
public:
    ToggleFrameFocusCmd(FluxboxWindow &win):
        m_win(win) {}
    void execute() {
        m_win.frame().setFocus( ! m_win.frame().focused() );
        m_win.attentionSig().notify();
    }
private:
    FluxboxWindow& m_win;
};

} // end anonymous namespace 


AttentionNoticeHandler::~AttentionNoticeHandler() {
    STLUtil::destroyAndClearSecond(m_attentions);
}

void AttentionNoticeHandler::addAttention(FluxboxWindow &win) {
    // no need to add already focused window
    if (win.isFocused())
        return;

    // Already have a notice for it?
    NoticeMap::iterator it = m_attentions.find(&win);
    if (it != m_attentions.end()) {
        return; 
    }

    using namespace FbTk;

    ResourceManager &res = win.screen().resourceManager();
    std::string res_name = win.screen().name() + ".demandsAttentionTimeout";
    std::string res_alt_name = win.screen().name() + ".DemandsAttentionTimeout";
    Resource<int> *timeout_res = dynamic_cast<Resource<int>* >(res.findResource(res_name));
    if (timeout_res == 0) {
        // no resource, create one and add it to managed resources
        timeout_res = new FbTk::Resource<int>(res, 500, res_name, res_alt_name);
        win.screen().addManagedResource(timeout_res);
    }
    // disable if timeout is zero
    if (**timeout_res == 0) 
        return;

    Timer *timer = new Timer();
    // setup timer
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = **timeout_res * 1000;
    RefCount<Command> cmd(new ToggleFrameFocusCmd(win));
    timer->setCommand(cmd);
    timer->setTimeout(timeout); 
    timer->fireOnce(false); // will repeat until window has focus
    timer->start();

    m_attentions[&win] = timer; 
    // attach signals that will make notice go away
    win.dieSig().attach(this);
    win.focusSig().attach(this);
}

void AttentionNoticeHandler::update(FbTk::Subject *subj) {

    // all signals results in destruction of the notice

    FluxboxWindow::WinSubject *winsubj = 
        static_cast<FluxboxWindow::WinSubject*>(subj);
    delete m_attentions[&winsubj->win()];
    m_attentions.erase(&winsubj->win());
}

