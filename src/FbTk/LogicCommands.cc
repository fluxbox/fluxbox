// LogicCommands.cc for FbTk
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

// $Id: $

#include "LogicCommands.hh"

namespace FbTk {

void OrCommand::add(RefCount<BoolCommand> &com) {
    m_commandlist.push_back(com);
}

size_t OrCommand::size() const {
    return m_commandlist.size();
}

bool OrCommand::bool_execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i) {
        if (m_commandlist[i]->bool_execute())
            return true;
    }
    return false;
}

void AndCommand::add(RefCount<BoolCommand> &com) {
    m_commandlist.push_back(com);
}

size_t AndCommand::size() const {
    return m_commandlist.size();
}

bool AndCommand::bool_execute() {
    for (size_t i=0; i < m_commandlist.size(); ++i) {
        if (!m_commandlist[i]->bool_execute())
            return false;
    }
    return true;
}

void XorCommand::add(RefCount<BoolCommand> &com) {
    m_commandlist.push_back(com);
}

size_t XorCommand::size() const {
    return m_commandlist.size();
}

bool XorCommand::bool_execute() {
    bool ret = false;
    for (size_t i=0; i < m_commandlist.size(); ++i)
        ret ^= m_commandlist[i]->bool_execute();
    return ret;
}

}; // end namespace FbTk
