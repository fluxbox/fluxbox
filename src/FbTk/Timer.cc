// Timer.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

#include "Command.hh"

//use GNU extensions
#ifndef	_GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>

namespace FbTk {

Timer::TimerList Timer::m_timerlist;

Timer::Timer():m_timing(false), m_once(false) {

}

Timer::Timer(RefCount<Command> &handler):
    m_handler(handler),
    m_timing(false),
    m_once(false) {
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


void Timer::setTimeout(timeval t) {
    m_timeout.tv_sec = t.tv_sec;
    m_timeout.tv_usec = t.tv_usec;
}

void Timer::setCommand(RefCount<Command> &cmd) {
    m_handler = cmd;
}

void Timer::start() {
    gettimeofday(&m_start, 0);

    if (! m_timing) {
        m_timing = true;
        addTimer(this); //add us to the list
    }
}


void Timer::stop() {
    m_timing = false;
    removeTimer(this); //remove us from the list
}


void Timer::fireTimeout() {
    if (*m_handler)
        m_handler->execute();
}

void Timer::updateTimers(int fd) {
    fd_set rfds;
    timeval now, tm, *timeout = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    if (m_timerlist.size() > 0) {
        gettimeofday(&now, 0);

        tm.tv_sec = tm.tv_usec = 0l;

        Timer *timer = m_timerlist.front();

        tm.tv_sec = timer->getStartTime().tv_sec +
            timer->getTimeout().tv_sec - now.tv_sec;
        tm.tv_usec = timer->getStartTime().tv_usec +
            timer->getTimeout().tv_usec - now.tv_usec;

        while (tm.tv_usec >= 1000000) {
            tm.tv_sec++;
            tm.tv_usec -= 1000000;
        }

        while (tm.tv_usec < 0) {
            if (tm.tv_sec > 0) {
                tm.tv_sec--;
                tm.tv_usec += 1000000;
            } else {
                tm.tv_usec = 0;
                break;
            }
        }

        timeout = &tm;
    }

    select(fd + 1, &rfds, 0, 0, timeout);

    // check for timer timeout
    gettimeofday(&now, 0);

    TimerList::iterator it = m_timerlist.begin();
    //must check end ...the timer might remove
    //it self from the list (should be fixed in the future)
    for(; it != m_timerlist.end(); ++it) {
        //This is to make sure we don't get an invalid iterator
        //when we do fireTimeout
        Timer &t = *(*it);
        tm.tv_sec = t.getStartTime().tv_sec +
            t.getTimeout().tv_sec;
        tm.tv_usec = t.getStartTime().tv_usec +
            t.getTimeout().tv_usec;

        if ((now.tv_sec < tm.tv_sec) ||
            (now.tv_sec == tm.tv_sec && now.tv_usec < tm.tv_usec))
            break;

        t.fireTimeout();
        // restart the current timer so that the start time is updated
        if (! t.doOnce())
            t.start();
        else {
            t.stop();			
            it--;
        }					
    }
}

void Timer::addTimer(Timer *timer) {
    assert(timer);

    TimerList::iterator it = m_timerlist.begin();
    TimerList::iterator it_end = m_timerlist.end();
    int index = 0;
    for (; it != it_end; ++it, ++index) {
        if (((*it)->getTimeout().tv_sec > timer->getTimeout().tv_sec) ||
            (((*it)->getTimeout().tv_sec == timer->getTimeout().tv_sec) &&
             ((*it)->getTimeout().tv_usec >= timer->getTimeout().tv_usec))) {
            break;
        }
    }
    m_timerlist.insert(it, timer); 

}

void Timer::removeTimer(Timer *timer) {
    assert(timer);
    m_timerlist.remove(timer);
}
	
}; // end namespace FbTk
