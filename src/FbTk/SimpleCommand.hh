// SimpleCommand.hh for FbTk
// Copyright (c) 2002-2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_SIMPLECOMMAND_HH
#define FBTK_SIMPLECOMMAND_HH

#include "Command.hh"

namespace FbTk {

/// a simple command
template <typename Receiver, typename ReturnType=void>
class SimpleCommand: public Command<ReturnType> {
public:
    typedef ReturnType (Receiver::* Action)();
    SimpleCommand(Receiver &r, Action a):
        m_receiver(r), m_action(a) { }
    void execute() { (m_receiver.*m_action)(); }
private:
    Receiver &m_receiver;
    Action m_action;
};

} // end namespace FbTk

#endif // FBTK_SIMPLECOMMAND_HH
