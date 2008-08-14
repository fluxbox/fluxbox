// AutoReloadHelper.cc
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

#include "AutoReloadHelper.hh"

#include "FileUtil.hh"
#include "StringUtil.hh"

namespace FbTk {

void AutoReloadHelper::checkReload() {
    if (!m_reload_cmd.get())
        return;
    TimestampMap::const_iterator it = m_timestamps.begin();
    TimestampMap::const_iterator it_end = m_timestamps.end();
    for (; it != it_end; ++it) {
        if (FileUtil::getLastStatusChangeTimestamp(it->first.c_str()) !=
            it->second) {
            reload();
            return;
        }
    }
}

void AutoReloadHelper::setMainFile(const std::string& file) {
    std::string expanded_file = StringUtil::expandFilename(file);
    if (expanded_file == m_main_file)
        return;
    m_main_file = expanded_file;
    reload();
}

void AutoReloadHelper::addFile(const std::string& file) {
    if (file.empty())
        return;
    std::string expanded_file = StringUtil::expandFilename(file);
    m_timestamps[expanded_file] = FileUtil::getLastStatusChangeTimestamp(expanded_file.c_str());
}

void AutoReloadHelper::reload() {
    if (!m_reload_cmd.get())
        return;
    m_timestamps.clear();
    addFile(m_main_file);
    m_reload_cmd->execute();
}

} // end namespace FbTk
