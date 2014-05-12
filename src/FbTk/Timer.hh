// Timer.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_TIMER_HH
#define FBTK_TIMER_HH

#include "RefCount.hh"
#include "Command.hh"
#include "FbTime.hh"

#include <string>

namespace FbTk {

/**
    Handles Timeout
*/
class Timer {
public:
    Timer();
    explicit Timer(const RefCount<Slot<void> > &handler);
    ~Timer();

    void fireOnce(bool once) { m_once = once; }
    void setTimeout(uint64_t timeout, bool force_start = false);
    void setCommand(const RefCount<Slot<void> > &cmd);

    template<typename Functor>
    void setFunctor(const Functor &functor) { 
        setCommand(RefCount<Slot<void> >(new SlotImpl<Functor, void>(functor)));
    }

    void setInterval(int seconds) { m_interval = seconds; }
    void start();
    void stop();

    static void updateTimers(int file_descriptor);

    int isTiming() const;
    int getInterval() const { return m_interval; }

    int doOnce() const { return m_once; }

    uint64_t getTimeout() const { return m_timeout; }
    uint64_t getStartTime() const { return m_start; }
    uint64_t getEndTime() const;

protected:
    /// force a timeout
    void fireTimeout();

private:
    RefCount<Slot<void> > m_handler; ///< what to do on a timeout

    bool m_once;  ///< do timeout only once?
    int m_interval; ///< Is an interval-only timer (e.g. clock), in seconds

    uint64_t m_start;   ///< start time in microseconds
    uint64_t m_timeout; ///< time length in microseconds
};



/// executes a command after a specified timeout
class DelayedCmd: public Command<void> {
public:

    // timeout in microseconds
    DelayedCmd(const RefCount<Slot<void> > &cmd, uint64_t timeout = 200);

    // this constructor has inverted order of parameters to avoid ambiguity with the previous
    // constructor
    template<typename Functor>
    DelayedCmd(uint64_t timeout, const Functor &functor) {
        initTimer(timeout);
        m_timer.setFunctor(functor);
    }

    void execute();
    static Command<void> *parse(const std::string &command,
                          const std::string &args, bool trusted);
private:
    void initTimer(uint64_t timeout);

    Timer m_timer;
};

} // end namespace FbTk

#endif // FBTK_TIMER_HH
