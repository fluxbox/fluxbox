// Timer.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 - 2012 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Timer.cc for Blackbox - An X11 Window Manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

#include "Timer.hh"

#include "CommandParser.hh"
#include "StringUtil.hh"

#ifdef HAVE_CASSERT
  #include <cassert>
#else
  #include <assert.h>
#endif

// sys/select.h on solaris wants to use memset()
#ifdef HAVE_CSTRING
#  include <cstring>
#else
#  include <string.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#elif defined(_WIN32)
#  include <winsock.h>
#endif

#include <cstdio>
#include <vector>
#include <set>

namespace {

struct TimerCompare {
    // stable sort order and allows multiple timers to have
    // the same end-time
    bool operator() (const FbTk::Timer* a, const FbTk::Timer* b) const {
        uint64_t ae = a->getEndTime();
        uint64_t be = b->getEndTime();
        return (ae < be) || (ae == be && a < b);
    }
};
typedef std::set<FbTk::Timer*, TimerCompare> TimerList;
TimerList s_timerlist;

}


namespace FbTk {

Timer::Timer() :
    m_once(false),
    m_interval(0),
    m_start(0),
    m_timeout(0) {

}

Timer::Timer(const RefCount<Slot<void> > &handler):
    m_handler(handler),
    m_once(false),
    m_interval(0),
    m_start(0),
    m_timeout(0) {
}


Timer::~Timer() {
    stop();
}


void Timer::setTimeout(uint64_t timeout, bool force_start) {

    bool was_timing = isTiming();
    if (was_timing) {
        stop();
    }
    m_timeout = timeout;

    if (force_start || was_timing) {
        start();
    }
}

void Timer::setCommand(const RefCount<Slot<void> > &cmd) {
    m_handler = cmd;
}

void Timer::start() {

    // only add Timers that actually DO something
    if ( ( ! isTiming() || m_interval > 0 ) && m_handler) {

        // in case start() gets triggered on a started 
        // timer with 'm_interval != 0' we have to remove
        // it from s_timerlist before restarting it
        stop();

        m_start = FbTk::FbTime::mono();

        // interval timers have their timeout change every 
        // time they are started!
        if (m_interval != 0) {
            m_timeout = m_interval * FbTk::FbTime::IN_SECONDS;
        }
        s_timerlist.insert(this);
    }
}


void Timer::stop() {
    s_timerlist.erase(this);
}

uint64_t Timer::getEndTime() const {
    return m_start + m_timeout;
}

int Timer::isTiming() const {
    return s_timerlist.find(const_cast<FbTk::Timer*>(this)) != s_timerlist.end();
}

void Timer::fireTimeout() {
    if (m_handler)
        (*m_handler)();
}


void Timer::updateTimers(int fd) {

    fd_set              rfds;
    timeval*            tout;
    timeval             tm;
    TimerList::iterator t;
    bool                overdue = false;
    uint64_t            now;


    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tout = NULL;

    // search for overdue timers
    if (!s_timerlist.empty()) {

        Timer*      timer = *s_timerlist.begin();
        uint64_t    end_time = timer->getEndTime();

        now = FbTime::mono();
        if (end_time <= now) {
            overdue = true;
        } else {
            uint64_t    diff = (end_time - now);
            tm.tv_sec = diff / FbTime::IN_SECONDS;
            tm.tv_usec = diff % FbTime::IN_SECONDS;
            tout = &tm;
        }
    }

    // if not overdue, wait for the next xevent via the blocking
    // select(), so OS sends fluxbox to sleep. the select() will
    // time out when the next timer has to be handled
    if (!overdue && select(fd + 1, &rfds, 0, 0, tout) != 0) {
        // didn't time out! x events are pending
        return;
    }

    // stoping / restarting the timers modifies the list in an upredictable
    // way. to avoid problems (infinite loops etc) we copy the current overdue
    // timers from the gloabl (and ordered) list of timers and work on it.

    static std::vector<FbTk::Timer*> timeouts;

    now = FbTime::mono();
    for (t = s_timerlist.begin(); t != s_timerlist.end(); ++t ) {
        if (now < (*t)->getEndTime()) {
            break;
        }
        timeouts.push_back(*t);
    }

    size_t i;
    const size_t ts = timeouts.size();
    for (i = 0; i < ts; ++i) {

        FbTk::Timer& timer = *timeouts[i];

        // first we stop the timer to remove it
        // from s_timerlist
        timer.stop();

        // then we call the handler which might (re)start 't'
        // on it's own
        timer.fireTimeout();

        // restart 't' if needed
        if (!timer.doOnce() && !timer.isTiming()) {
            timer.start();
        }
    }

    timeouts.clear();
}


Command<void> *DelayedCmd::parse(const std::string &command,
                           const std::string &args, bool trusted) {

    std::string cmd_str;
    int err = StringUtil::getStringBetween(cmd_str, args.c_str(), '{', '}',
                                           " \t\n", true);
    if (err == 0)
        return 0;

    RefCount<Command<void> > cmd(CommandParser<void>::instance().parse(cmd_str, trusted));
    if (cmd == 0)
        return 0;

    uint64_t delay = 200;
    StringUtil::extractNumber(args.c_str() + err, delay);

    return new DelayedCmd(cmd, delay);
}

REGISTER_COMMAND_PARSER(delay, DelayedCmd::parse, void);

DelayedCmd::DelayedCmd(const RefCount<Slot<void> > &cmd, uint64_t timeout) {
    initTimer(timeout);
    m_timer.setCommand(cmd);
}

void DelayedCmd::initTimer(uint64_t timeout) {
    m_timer.setTimeout(timeout);
    m_timer.fireOnce(true);
}

void DelayedCmd::execute() {
    if (m_timer.isTiming())
        m_timer.stop();
    m_timer.start();
}

} // end namespace FbTk
