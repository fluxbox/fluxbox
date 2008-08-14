// AutoReloadHelper.hh
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef AUTORELOADHELPER_HH
#define AUTORELOADHELPER_HH

#include <map>
#include <string>
#include <sys/types.h>

#include "Command.hh"
#include "RefCount.hh"

namespace FbTk {

class AutoReloadHelper {
public:

    void setMainFile(const std::string& filename);
    void addFile(const std::string& filename);
    void setReloadCmd(RefCount<Command<void> > cmd) { m_reload_cmd = cmd; }

    void checkReload();
    void reload();

private:
    RefCount<Command<void> > m_reload_cmd;
    std::string m_main_file;

    typedef std::map<std::string, time_t> TimestampMap;
    TimestampMap m_timestamps;
};

} // end namespace FbTk

#endif // AUTORELOADHELPER_HH
