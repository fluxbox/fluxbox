// CommandDialog.hh for Fluxbox
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

#ifndef RUNCOMMANDDIALOG_HH
#define RUNCOMMANDDIALOG_HH

#include "TextDialog.hh"
#include "FbTk/RefCount.hh"

class Command;

/**
 * Displays a fluxbox command dialog which executes fluxbox
 * action commands.
 */
class CommandDialog: public TextDialog {
public:
    CommandDialog(BScreen &screen, const std::string &title,
                  const std::string &pre_command = "");

    /**
     * Sets the command to be executed after the command is done.
     * @param postcommand the command.
     */
    void setPostCommand(FbTk::RefCount<FbTk::Command<void> > &postcommand) { 
        m_postcommand = postcommand; 
    }

private:
    /// expand the current word, using the history as a references
    void tabComplete();
    void exec(const std::string &string);

    /// command to do after the first command was issued (like reconfigure)
    FbTk::RefCount<FbTk::Command<void> > m_postcommand;
    /// command to be used before the text (usefull for setting workspace name)
    const std::string m_precommand;
};


#endif // SETWORKSPACENAME_HH
