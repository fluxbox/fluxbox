// StyleMenuItem.cc for Fluxbox Window Manager
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                    and Simon Bowden (rathnor at users.sourceforge.net)
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

#include "StyleMenuItem.hh"

#include "FbCommands.hh"
#include "fluxbox.hh"

#include "FbTk/StringUtil.hh"

StyleMenuItem::StyleMenuItem(const FbTk::FbString &label, const std::string &filename):
    FbTk::RadioMenuItem(label), 
    m_filename(filename) {
    // perform shell style ~ home directory expansion
    // and insert style      
    FbTk::RefCount<FbTk::Command<void> > 
        setstyle_cmd(new FbCommands::
                     SetStyleCmd(m_filename));
    setCommand(setstyle_cmd);
    setToggleItem(true);
    setCloseOnClick(false);
}


bool StyleMenuItem::isSelected() const {
    return Fluxbox::instance()->getStyleFilename() == m_filename;
}

