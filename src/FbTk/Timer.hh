// Timer.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//
// Timer.hh for Blackbox - An X11 Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.
	
#ifndef	 FBTK_TIMER_HH
#define	 FBTK_TIMER_HH

#include "RefCount.hh"

#include <ctime> 
#include <list>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif //HAVE_CONFIG_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#include <sys/types.h>
#endif // HAVE_INTTYPES_H

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace FbTk {

class Command;

/**
	Handles Timeout
*/
class Timer {
public:
    Timer();
    explicit Timer(RefCount<Command> &handler);
    virtual ~Timer();

    inline void fireOnce(bool once) { m_once = once; }
    /// set timeout
    void setTimeout(time_t val);
    /// set timeout 
    void setTimeout(timeval val);
    void setCommand(RefCount<Command> &cmd);
    /// start timing
    void start();
    /// stop timing
    void stop();
    /// update all timers
    static void updateTimers(int file_descriptor);

    inline int isTiming() const { return m_timing; } 
    inline int doOnce() const { return m_once; }
    
    inline const timeval &getTimeout() const { return m_timeout; }
    inline const timeval &getStartTime() const { return m_start; }

protected:
    /// force a timeout
    void fireTimeout();

private:
    /// add a timer to the static list
    static void addTimer(Timer *timer);
    /// remove a timer from the static list
    static void removeTimer(Timer *timer);
	
    typedef std::list<Timer *> TimerList;
    static TimerList m_timerlist; ///< list of all timers
	
    RefCount<Command> m_handler; ///< what to do on a timeout
	
    bool m_timing; ///< clock running?
    bool m_once;  ///< do timeout only once?

    timeval m_start;    ///< start time
    timeval m_timeout; ///< time length

};

} // end namespace FbTk

#endif // FBTK_TIMER_HH

