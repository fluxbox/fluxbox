// Timer.hh for fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Timer.hh for Blackbox - An X11 Window Manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
	
#ifndef	 TIMER_HH
#define	 TIMER_HH

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif //HAVE_CONFIG_H

#ifdef		TIME_WITH_SYS_TIME
#	include <sys/time.h>
#	include <time.h> 
#else // !TIME_WITH_SYS_TIME 
#	ifdef		HAVE_SYS_TIME_H
#include <sys/time.h>
#	else // !HAVE_SYS_TIME_H
#		include <time.h>
#	endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#include <list>

class TimeoutHandler {
public:
	virtual void timeout(void) = 0;
};


class BTimer {
public:
	explicit BTimer(TimeoutHandler *);
	virtual ~BTimer(void);

	inline const int isTiming(void) const { return m_timing; } 
	inline const int doOnce(void) const { return m_once; }

	inline const timeval &getTimeout(void) const { return m_timeout; }
	inline const timeval &getStartTime(void) const { return m_start; }

	inline void fireOnce(bool once) { m_once = once; }

	void setTimeout(long);
	void setTimeout(timeval);
	void start(void);
	void stop(void);
	static void updateTimers(int fd);
protected:
	void fireTimeout(void);

private:
	static void addTimer(BTimer *timer);
	static void removeTimer(BTimer *timer);
	
	typedef std::list<BTimer *> TimerList;
    static TimerList m_timerlist;
	
	TimeoutHandler *m_handler;
	
	bool m_timing, m_once;

	timeval m_start, m_timeout;

};


#endif // _TIMER_HH_

