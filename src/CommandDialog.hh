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

// $Id$

#ifndef RUNCOMMANDDIALOG_HH
#define RUNCOMMANDDIALOG_HH

#include "FbTk/TextBox.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/Font.hh"
#include "FbTk/GContext.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"

class BScreen;

class CommandDialog: public FbTk::FbWindow, public FbTk::EventHandler {
public:
    CommandDialog(BScreen &screen, const std::string &title,
                  const std::string pre_command = "");
    virtual ~CommandDialog();

    void setText(const std::string &text);
    void setPostCommand(FbTk::RefCount<FbTk::Command> &postcommand) { m_postcommand = postcommand; }
    void show();
    void hide();

    void exposeEvent(XExposeEvent &event);
    void motionNotifyEvent(XMotionEvent &event);
    void buttonPressEvent(XButtonEvent &event);
    void handleEvent(XEvent &event);
    void keyPressEvent(XKeyEvent &event);

protected:
    virtual void tabComplete();

private:
    void init();
    void render();
    void updateSizes();

    FbTk::TextBox m_textbox;
    FbTk::TextButton m_label;
    FbTk::GContext m_gc;
    FbTk::RefCount<FbTk::Command> m_postcommand; ///< command to do after the first command was issued (like reconfigure)
    BScreen &m_screen;
    int m_move_x, m_move_y;
    Pixmap m_pixmap;
    const std::string m_precommand; ///< command to be used before the text (usefull for setting workspace name)
};


#endif // SETWORKSPACENAME_HH
