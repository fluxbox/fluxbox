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

// $Id: IconbarTool.cc,v 1.4 2003/08/12 11:09:46 fluxgen Exp $

#include "IconbarTool.hh"

#include "Screen.hh"
#include "ImageControl.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"
#include "Workspace.hh"

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
    m_theme(theme),
    m_focused_pm(0),
    m_unfocused_pm(0),
    m_empty_pm(0) {

    // setup signals
    theme.reconfigSig().attach(this);
    screen.clientListSig().attach(this);
    screen.currentWorkspaceSig().attach(this);

    update(0);
    renderTheme();
}

IconbarTool::~IconbarTool() {
    while (!m_icon_list.empty()) {
        delete m_icon_list.back();
        m_icon_list.pop_back();
    }

    // remove cached images
    if (m_focused_pm)
        m_screen.imageControl().removeImage(m_focused_pm);
    if (m_unfocused_pm)
        m_screen.imageControl().removeImage(m_focused_pm);
    if (m_empty_pm)
        m_screen.imageControl().removeImage(m_empty_pm);

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

    // just focus signal?
    if (subj != 0 && typeid(*subj) == typeid(FluxboxWindow::WinSubject)) {
        // we handle everything except die signal here
        FluxboxWindow::WinSubject *winsubj = static_cast<FluxboxWindow::WinSubject *>(subj);
        if (subj != &(winsubj->win().dieSig())) {
            renderButton(winsubj->win());
            return;
        }
    }

    // ok, we got some signal that we need to update our iconbar container

    // remove all clients and add them again...the only way to do it now
    m_icon_container.removeAll();

    while (!m_icon_list.empty()) {
        delete m_icon_list.back();
        m_icon_list.pop_back();
    }

    // get current workspace and all it's clients
    Workspace &space = *m_screen.currentWorkspace();
    // build a ItemList and add it (faster than adding single items)
    Container::ItemList items;
    Workspace::Windows::iterator it = space.windowList().begin();
    Workspace::Windows::iterator it_end = space.windowList().end();
    for (; it != it_end; ++it) {
        // we just want windows that has clients
        if ((*it)->clientList().size() == 0)
            continue;

        IconButton *button = new IconButton(m_icon_container, m_theme.focusedText().font(), **it);
        items.push_back(button);
        m_icon_list.push_back(button);

        (*it)->focusSig().attach(this);
        (*it)->dieSig().attach(this);
    }

    m_icon_container.showSubwindows();
    m_icon_container.insertItems(items);

    renderTheme();
}

void IconbarTool::renderButton(FluxboxWindow &win) {
    
    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if (&(*icon_it)->win() == &win)
            break;
    }
    if (icon_it == m_icon_list.end())
        return;

    IconButton &button = *(*icon_it);
    
    if (button.win().isFocused()) { // focused texture
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
    
}

void IconbarTool::renderTheme() {
    Pixmap tmp = m_focused_pm;
    if (m_theme.focusedTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_focused_pm = 0;        
    } else {
        m_focused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                           m_icon_container.height(),
                                                           m_theme.focusedTexture());
    }
        
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    tmp = m_unfocused_pm;
    if (m_theme.unfocusedTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_unfocused_pm = 0;        
    } else {
        m_unfocused_pm = m_screen.imageControl().renderImage(m_icon_container.maxWidthPerClient(),
                                                             m_icon_container.height(),
                                                             m_theme.unfocusedTexture());
    }
    if (tmp)
        m_screen.imageControl().removeImage(tmp);

    // if we dont have any icons then we should render empty texture
    tmp = m_empty_pm;
    if (m_theme.emptyTexture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_empty_pm = 0;
        m_icon_container.setBackgroundColor(m_theme.emptyTexture().color());
    } else {
        m_empty_pm = m_screen.imageControl().renderImage(m_icon_container.width(), m_icon_container.height(),
                                                         m_theme.emptyTexture());
        m_icon_container.setBackgroundPixmap(m_empty_pm);
    }

    if (tmp)
        m_screen.imageControl().removeImage(m_empty_pm);

    // update buttons
    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {

        IconButton &button = *(*icon_it);

        if (button.win().isFocused()) { // focused texture
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
