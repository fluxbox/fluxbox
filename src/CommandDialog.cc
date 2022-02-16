// CommandDialog.cc for Fluxbox
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#include "CommandDialog.hh"

#include "FbTk/CommandParser.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/App.hh"

#include <X11/Xutil.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

using std::string;
using std::vector;
using std::less;
using std::out_of_range;

CommandDialog::CommandDialog(BScreen &screen, const string &title,
                             const string &precommand) : 
    TextDialog(screen, title),
    m_precommand(precommand) { }

void CommandDialog::exec(const std::string &text){

    // create Command<void> from line
    std::unique_ptr<FbTk::Command<void> > cmd(FbTk::CommandParser<void>::instance().parse(m_precommand + text));
    if (cmd.get())
        cmd->execute();
    // post execute
    if (m_postcommand != 0)
        m_postcommand->execute();
}

void CommandDialog::tabComplete() {
    try {
        string::size_type first = m_textbox.text().find_last_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "abcdefghijklmnopqrstuvwxyz"
                                  "0123456789", m_textbox.cursorPosition());
        if (first == string::npos)
            first = 0;
        string prefix = FbTk::StringUtil::toLower(m_textbox.text().substr(first,
                                                   m_textbox.cursorPosition()));
        if (prefix.empty()) {
            XBell(FbTk::App::instance()->display(), 0);
            return;
        }

        FbTk::CommandParser<void>::CreatorMap::const_iterator it = FbTk::CommandParser<void>::instance().creatorMap().begin();
        const FbTk::CommandParser<void>::CreatorMap::const_iterator it_end = FbTk::CommandParser<void>::instance().creatorMap().end();
        vector<string> matches;
        for (; it != it_end; ++it) {
            if ((*it).first.find(prefix) == 0) {
                matches.push_back((*it).first);
            }
        }

        if (!matches.empty()) {
            // sort and apply larges match
            sort(matches.begin(), matches.end(), less<string>());
            m_textbox.setText(m_textbox.text() + matches[0].substr(prefix.size()));
        } else
            XBell(FbTk::App::instance()->display(), 0);

    } catch (out_of_range &oor) {
        XBell(FbTk::App::instance()->display(), 0);
    }
}
