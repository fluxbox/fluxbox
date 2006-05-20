// Xinerama.hh for Fluxbox - helpful tools for multiple heads
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef XINERAMA_HH
#define XINERAMA_HH

#include "MenuItem.hh"
#include "FbMenu.hh"
#include "RefCount.hh"
#include "SimpleCommand.hh"

#include "fluxbox.hh"

// provides a generic way for giving an object a xinerama head menu
// The object must have two functions:
// int getOnHead(), and
// void setOnHead(int)

/// this class holds the xinerama items
template <typename ItemType> 
class XineramaHeadMenuItem : public FbTk::MenuItem {
public:
    XineramaHeadMenuItem(const FbTk::FbString &label, ItemType &object, int headnum,
                  FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label,cmd), m_object(object), m_headnum(headnum) {}
    XineramaHeadMenuItem(const FbTk::FbString &label, ItemType &object, int headnum):
        FbTk::MenuItem(label), m_object(object), m_headnum(headnum) {}

    bool isEnabled() const { return m_object.getOnHead() != m_headnum; } 
    void click(int button, int time) {
        m_object.saveOnHead(m_headnum);
        FbTk::MenuItem::click(button, time);
    }
    
private:
    ItemType &m_object;
    int m_headnum;
};


/// Create a xinerama menu
template <typename ItemType>
class XineramaHeadMenu : public FbMenu {
public:
    XineramaHeadMenu(MenuTheme &tm, BScreen &screen, FbTk::ImageControl &imgctrl,
                     FbTk::XLayer &layer, ItemType &item, const FbTk::FbString & title = "");

private:
    ItemType &m_object;
};


template <typename ItemType>
XineramaHeadMenu<ItemType>::XineramaHeadMenu(MenuTheme &tm, BScreen &screen, FbTk::ImageControl &imgctrl,
                               FbTk::XLayer &layer, ItemType &item, const FbTk::FbString & title):
    FbMenu(tm, imgctrl, layer), 
    m_object(item) 
{
    setLabel(title);
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(
                                     *Fluxbox::instance(), 
                                     &Fluxbox::save_rc));
    char tname[128];
    for (int i=1; i <= screen.numHeads(); ++i) {
        // TODO: nls
/*
        sprintf(tname, I18n::instance()->
                getMessage(
                    FBNLS::ScreenSet, 
                    FBNLS::XineramaDefaultHeadFormat,
                    "Head %d"), i); //m_id starts at 0
*/
        sprintf(tname, "Head %d", i);
        insert(new XineramaHeadMenuItem<ItemType>(
                   tname, m_object, i, saverc_cmd));
    }
    // TODO: nls
    insert(new XineramaHeadMenuItem<ItemType>(
               "All Heads", m_object, 0, saverc_cmd));
    updateMenu();
}

#endif // XINERAMA_HH
