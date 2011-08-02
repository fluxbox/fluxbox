// Timer.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

#include <sys/select.h>

namespace FbTk {

Timer::TimerList Timer::m_timerlist;

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


void Timer::setTimeout(time_t t) {
    m_timeout.tv_sec = t / 1000;
    m_timeout.tv_usec = t;
    m_timeout.tv_usec -= (m_timeout.tv_sec * 1000);
    m_timeout.tv_usec *= 1000;
}


void Timer::setTimeout(const timeval &t) {
    m_timeout.tv_sec = t.tv_sec;
    m_timeout.tv_usec = t.tv_usec;
}

void Timer::setTimeout(unsigned int secs, unsigned int usecs) {
    m_timeout.tv_sec = secs;
    m_timeout.tv_usec = usecs;
}

void Timer::setCommand(const RefCount<Slot<void> > &cmd) {
    m_handler = cmd;
}

void Timer::start() {
    gettimeofday(&m_start, 0);

    // only add Timers that actually DO something
    if ((! m_timing || m_interval != 0) && m_handler) {
        m_timing = true;
        addTimer(this); //add us to the list
    }
}


void Timer::stop() {
    m_timing = false;
    removeTimer(this); //remove us from the list
}

void Timer::makeEndTime(timeval &tm) const {
    tm.tv_sec = m_start.tv_sec + m_timeout.tv_sec;
    tm.tv_usec = m_start.tv_usec + m_timeout.tv_usec;
    if (tm.tv_usec >= 1000000) {
        tm.tv_usec -= 1000000;
        tm.tv_sec++;
    }
}


void Timer::fireTimeout() {
    if (m_handler)
        (*m_handler)();
}

void Timer::updateTimers(int fd) {
    fd_set rfds;
    timeval now, tm, *timeout = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    bool overdue = false;

    // see, if the first timer in the
    // list is overdue
    if (!m_timerlist.empty()) {
        gettimeofday(&now, 0);

        Timer *timer = m_timerlist.front();

        timer->makeEndTime(tm);

        tm.tv_sec -= now.tv_sec;
        tm.tv_usec -= now.tv_usec;

        while (tm.tv_usec < 0) {
            if (tm.tv_sec > 0) {
                tm.tv_sec--;
                tm.tv_usec += 1000000;
            } else {
                overdue = true;
                tm.tv_usec = 0;
                break;
            }
        }

        if (tm.tv_sec < 0) { // usec zero-ed above if negative
            tm.tv_sec = 0;
            tm.tv_usec = 0;
            overdue = true;
        }

        timeout = &tm;
    }

    // if not overdue, wait for the next xevent via the blocking
    // select(), so OS sends fluxbox to sleep. the select() will
    // time out when the next timer has to be handled
    if (!overdue && select(fd + 1, &rfds, 0, 0, timeout) != 0)
        // didn't time out! x events are pending
        return;

    TimerList::iterator it;

    // check for timer timeout
    gettimeofday(&now, 0);

    // someone set the date of the machine BACK
    // so we have to adjust the start_time
    static time_t last_time = 0;
    if (now.tv_sec < last_time) {

        time_t delta = last_time - now.tv_sec;

        for (it = m_timerlist.begin(); it != m_timerlist.end(); it++) {
            (*it)->m_start.tv_sec -= delta;
        }
    }
    last_time = now.tv_sec;


    //must check end ...the timer might remove
    //it self from the list (should be fixed in the future)
    for(it = m_timerlist.begin(); it != m_timerlist.end(); ) {
        //This is to make sure we don't get an invalid iterator
        //when we do fireTimeout
        Timer &t = *(*it);

        t.makeEndTime(tm);

        if (((now.tv_sec < tm.tv_sec) ||
             (now.tv_sec == tm.tv_sec && now.tv_usec < tm.tv_usec)))
            break;

        t.fireTimeout();
        // restart the current timer so that the start time is updated
        if (! t.doOnce()) {
            // must erase so that it's put into the right place in the list
            it = m_timerlist.erase(it);
            t.m_timing = false;
            t.start();
        } else {
            // Since the default stop behaviour results in the timer
            // being removed, we must remove it here, so that the iterator
            // lives well. Another option would be to add it to another
            // list, and then just go through that list and stop them all.
            it = m_timerlist.erase(it);
            t.stop();
        }
    }

}

void Timer::addTimer(Timer *timer) {
    assert(timer);
    int interval = timer->getInterval();
    // interval timers have their timeout change every time they are started!
    timeval tm;
    if (interval != 0) {
        tm.tv_sec = timer->getStartTime().tv_sec;
        tm.tv_usec = timer->getStartTime().tv_usec;

        // now convert to interval
        tm.tv_sec = interval - (tm.tv_sec % interval) - 1;
        tm.tv_usec = 1000000 - tm.tv_usec;
        if (tm.tv_usec == 1000000) {
            tm.tv_usec = 0;
            tm.tv_sec += 1;
        }
        timer->setTimeout(tm);
    }

    // set timeval to the time-of-trigger
    timer->makeEndTime(tm);

    // timer list is sorted by trigger time (i.e. start plus timeout)
    TimerList::iterator it = m_timerlist.begin();
    TimerList::iterator it_end = m_timerlist.end();
    for (; it != it_end; ++it) {
        timeval trig;
        (*it)->makeEndTime(trig);

        if ((trig.tv_sec > tm.tv_sec) ||
            (trig.tv_sec == tm.tv_sec &&
             trig.tv_usec >= tm.tv_usec)) {
            break;
        }
    }
    m_timerlist.insert(it, timer); 

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

    int delay = 200000;
    StringUtil::fromString<int>(args.c_str() + err, delay);

    return new DelayedCmd(cmd, delay);
}

REGISTER_COMMAND_PARSER(delay, DelayedCmd::parse, void);

DelayedCmd::DelayedCmd(const RefCount<Slot<void> > &cmd, unsigned int timeout) {
    initTimer(timeout);
    m_timer.setCommand(cmd);
}

void DelayedCmd::initTimer(unsigned int timeout) {
    timeval to;
    to.tv_sec = timeout/1000000;
    to.tv_usec = timeout % 1000000;
    m_timer.setTimeout(to);
    m_timer.fireOnce(true);
}

void DelayedCmd::execute() {
    if (m_timer.isTiming())
        m_timer.stop();
    m_timer.start();
}

void Timer::removeTimer(Timer *timer) {
    assert(timer);
    m_timerlist.remove(timer);
}
	
} // end namespace FbTk
