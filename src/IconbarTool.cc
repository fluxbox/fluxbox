// IconbarTool.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: IconbarTool.cc,v 1.1 2003/08/11 15:42:29 fluxgen Exp $

#include "IconbarTool.hh"

#include "Screen.hh"
#include "ImageControl.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"

#include <iostream>
using namespace std;
class ShowTextCmd: public FbTk::Command {
    void execute() {
        cerr<<"ShowTextCmd"<<endl;
    }
};

IconbarTool::IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, BScreen &screen):
    ToolbarItem(ToolbarItem::RELATIVE),
    m_screen(screen),
    m_icon_container(parent),
    m_theme(theme) {

    theme.reconfigSig().attach(this);
    screen.clientListSig().attach(this);

    // for debug
    /*    FbTk::RefCount<FbTk::Command> show_text(new ShowTextCmd());
    FbTk::Button *button = new TextButton(m_icon_container, theme.focusedText().font(), "ONE WINDOW");
    button->setOnClick(show_text);
    button->setGC(theme.focusedText().textGC());
    button->setBackgroundColor(theme.focusedTexture().color());
    button->clear();
    m_icon_container.insertItem(button);

    button = new TextButton(m_icon_container, theme.focusedText().font(), "ONE WINDOW");
    button->setOnClick(show_text);
    button->setGC(theme.focusedText().textGC());
    button->setBackgroundColor(theme.focusedTexture().color());
    button->clear();
    m_icon_container.insertItem(button);

    button = new TextButton(m_icon_container, theme.focusedText().font(), "ONE WINDOW");
    button->setOnClick(show_text);
    button->setGC(theme.focusedText().textGC());
    button->setBackgroundColor(theme.focusedTexture().color());
    button->clear();
    m_icon_container.insertItem(button);
    
    m_icon_container.showSubwindows();
    */
    renderTheme();
}

void IconbarTool::move(int x, int y) {
    m_icon_container.move(x, y);
}

void IconbarTool::resize(unsigned int width, unsigned int height) {
    m_icon_container.resize(width, height);
    renderTheme();
}

void IconbarTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {

    m_icon_container.moveResize(x, y, width, height);
    renderTheme();
}

void IconbarTool::show() {
    m_icon_container.show();
}

void IconbarTool::hide() {
    m_icon_container.hide();
}

unsigned int IconbarTool::width() const {
    return m_icon_container.width();
}

unsigned int IconbarTool::height() const {
    return m_icon_container.height();
}

void IconbarTool::update(FbTk::Subject *subj) {

    renderTheme();
}

void IconbarTool::renderTheme() {
    Pixmap tmp = m_focused_pm;
    if (m_theme.focusedTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_focused_pm = 0;        
    } else {
        m_focused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                           m_icon_container.maxHeightPerClient(),
                                                           m_theme.focusedTexture());
    }
        
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    tmp = m_unfocused_pm;
    if (m_theme.unfocusedTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_unfocused_pm = 0;        
    } else {
        m_unfocused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                             m_icon_container.maxHeightPerClient(),
                                                             m_theme.unfocusedTexture());
    }
    if (tmp)
        m_screen.imageControl().removeImage(tmp);


    // update buttons
    Icon2WinMap::iterator icon_it = m_icon2winmap.begin();
    Icon2WinMap::iterator icon_it_end = m_icon2winmap.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        IconButton &button = *(*icon_it).second;
        if (button.win().isFocused()) {
            button.setGC(m_theme.focusedText().textGC());     
            button.setFont(m_theme.focusedText().font());
            button.setJustify(m_theme.focusedText().justify());

            if (m_focused_pm != 0)
                button.setBackgroundPixmap(m_focused_pm);
            else
                button.setBackgroundColor(m_theme.focusedTexture().color());            


        } else { // unfocused
            button.setGC(m_theme.unfocusedText().textGC());
            button.setFont(m_theme.unfocusedText().font());
            button.setJustify(m_theme.unfocusedText().justify());

            if (m_unfocused_pm != 0)
                button.setBackgroundPixmap(m_unfocused_pm);
            else
                button.setBackgroundColor(m_theme.unfocusedTexture().color());
        }

        button.clear();
    }
}
