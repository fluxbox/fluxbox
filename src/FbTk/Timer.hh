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

#include <ctime> 
#include <list>

namespace FbTk {

/// Handles timeouts
/**
	Inherit this to have a timed object, that calls
	timeout function when the time is out
*/
class TimeoutHandler {
public:
    /// called when the time is out
    virtual void timeout() = 0;
};

/**
	Handles TimeoutHandles
*/
class Timer {
public:
    explicit Timer(TimeoutHandler *handler);
    virtual ~Timer();

    inline int isTiming() const { return m_timing; } 
    inline int doOnce() const { return m_once; }

    inline const timeval &getTimeout() const { return m_timeout; }
    inline const timeval &getStartTime() const { return m_start; }

    inline void fireOnce(bool once) { m_once = once; }
    /// set timeout
    void setTimeout(long val);
    /// set timeout 
    void setTimeout(timeval val);
    /// start timing
    void start();
    /// stop timing
    void stop();
    /// update all timers
    static void updateTimers(int file_descriptor);

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
	
    TimeoutHandler *m_handler; ///< handler
	
    bool m_timing; ///< clock running?
    bool m_once;  ///< do timeout only once?

    timeval m_start;    ///< start time
    timeval m_timeout; ///< time length

};

}; // end namespace FbTk

#endif // FBTK_TIMER_HH

