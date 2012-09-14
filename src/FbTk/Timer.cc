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

//use GNU extensions
#ifndef	_GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

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
#include <set>


namespace {

struct TimerCompare {
    bool operator() (const FbTk::Timer* a, const FbTk::Timer* b) {
        return a->getEndTime() < b->getEndTime();
    }
};
typedef std::set<FbTk::Timer*, TimerCompare> TimerList;

TimerList s_timerlist;


/// add a timer to the static list
void addTimer(FbTk::Timer *timer) {

    assert(timer);
    int interval = timer->getInterval();

    // interval timers have their timeout change every time they are started!
    if (interval != 0) {
        timer->setTimeout(interval * FbTk::FbTime::IN_SECONDS);
    }

    s_timerlist.insert(timer);
}

/// remove a timer from the static list
void removeTimer(FbTk::Timer *timer) {

    assert(timer);
    s_timerlist.erase(timer);
}


}


namespace FbTk {

Timer::Timer():m_timing(false), m_once(false), m_interval(0) {

}

Timer::Timer(const RefCount<Slot<void> > &handler):
    m_handler(handler),
    m_timing(false),
    m_once(false),
    m_interval(0) {
}


Timer::~Timer() {
    if (isTiming()) stop();
}


void Timer::setTimeout(uint64_t timeout) {

    bool was_timing = isTiming();
    if (was_timing) {
        stop();
    }
    m_timeout = timeout;

    if (was_timing) {
        start();
    }
}

void Timer::setCommand(const RefCount<Slot<void> > &cmd) {
    m_handler = cmd;
}

void Timer::start() {

    m_start = FbTk::FbTime::now();

    // only add Timers that actually DO something
    if ((! m_timing || m_interval != 0) && m_handler) {
        m_timing = true;
        ::addTimer(this);
    }
}


void Timer::stop() {
    m_timing = false;
    ::removeTimer(this);
}

uint64_t Timer::getEndTime() const {
    return m_start + m_timeout;
}


void Timer::fireTimeout() {
    if (m_handler)
        (*m_handler)();
}


void Timer::updateTimers(int fd) {

    fd_set rfds;
    timeval tm;
    timeval* timeout = 0;
    TimerList::iterator it;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    bool overdue = false;
    uint64_t now = FbTime::now();
    uint64_t end_time;

    // search for overdue timers
    if (!s_timerlist.empty()) {

        Timer* timer = *s_timerlist.begin();
        end_time = timer->getEndTime();

        if (end_time < now) {
            overdue = true;
        } else {
            uint64_t diff = (end_time - now);
            tm.tv_sec = diff / FbTime::IN_SECONDS;
            tm.tv_usec = diff % FbTime::IN_SECONDS;
        }

        timeout = &tm;
    }

    // if not overdue, wait for the next xevent via the blocking
    // select(), so OS sends fluxbox to sleep. the select() will
    // time out when the next timer has to be handled
    if (!overdue && select(fd + 1, &rfds, 0, 0, timeout) != 0) {
        // didn't time out! x events are pending
        return;
    }

    now = FbTime::now();
    for (it = s_timerlist.begin(); it != s_timerlist.end(); ) {

        // t->fireTimeout() might add timers to the list
        // this invalidates 'it'. thus we store the current timer
        Timer* t = *it;
        if (now < t->getEndTime()) {
            break;
        }

        t->fireTimeout();

        // find the iterator to the timer again
        // and continue working on the list
        it = s_timerlist.find(t);
        it++;
        s_timerlist.erase(t);

        if (! t->doOnce()) { // restart the current timer
            t->m_timing = false;
            t->start();
        } else {
            t->stop();
        }
    }

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
    StringUtil::fromString<uint64_t>(args.c_str() + err, delay);

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
