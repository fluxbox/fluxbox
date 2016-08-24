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

#ifndef XINERAMA_HH
#define XINERAMA_HH

#include "FbMenu.hh"
#include "fluxbox.hh"
#include "Screen.hh"

#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/RadioMenuItem.hh"

// provides a generic way for giving an object a xinerama head menu
// The object must have two functions:
// int getOnHead(), and
// void setOnHead(int)

/// this class holds the xinerama items
template <typename ItemType> 
class XineramaHeadMenuItem : public FbTk::RadioMenuItem {
public:
    XineramaHeadMenuItem(const FbTk::FbString &label, ItemType &object, int headnum,
                  FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label,cmd), m_object(object), m_headnum(headnum) {}
    XineramaHeadMenuItem(const FbTk::FbString &label, ItemType &object, int headnum):
        FbTk::RadioMenuItem(label), m_object(object), m_headnum(headnum) {}

    bool isSelected() const { return m_object.getOnHead() == m_headnum; } 
    void click(int button, int time, unsigned int mods) {
        m_object.saveOnHead(m_headnum);
        FbTk::RadioMenuItem::click(button, time, mods);
    }

private:
    ItemType &m_object;
    int m_headnum;
};


/// Create a xinerama menu
template <typename ItemType>
class XineramaHeadMenu : public ToggleMenu {
public:
    XineramaHeadMenu(FbTk::ThemeProxy<FbTk::MenuTheme> &tm, BScreen &screen,
                     FbTk::ImageControl &imgctrl, FbTk::Layer &layer,
                     ItemType &item, const FbTk::FbString & title = "");
    void reloadHeads();

private:
    ItemType &m_object;
    BScreen &m_screen;
};


template <typename ItemType>
XineramaHeadMenu<ItemType>::XineramaHeadMenu(
        FbTk::ThemeProxy<FbTk::MenuTheme> &tm, BScreen &screen,
        FbTk::ImageControl &imgctrl, FbTk::Layer &layer, ItemType &item,
        const FbTk::FbString & title):
    ToggleMenu(tm, imgctrl, layer),
    m_object(item), m_screen(screen)
{
    setLabel(title);
    reloadHeads();
}

template <typename ItemType>
void XineramaHeadMenu<ItemType>::reloadHeads()
{
    removeAll();
    FbTk::RefCount<FbTk::Command<void> > saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(
                                     *Fluxbox::instance(), 
                                     &Fluxbox::save_rc));
    for (int i=1; i <= m_screen.numHeads(); ++i) {
        // TODO: nls
/*
        sprintf(tname, I18n::instance()->
                getMessage(
                    FBNLS::ScreenSet, 
                    FBNLS::XineramaDefaultHeadFormat,
                    "Head %d"), i); //m_id starts at 0
*/
        std::string tname("Head ");
        tname += FbTk::StringUtil::number2String(i);
        insertItem(new XineramaHeadMenuItem<ItemType>(
                   tname.c_str(), m_object, i, saverc_cmd));
    }
    // TODO: nls
    insertItem(new XineramaHeadMenuItem<ItemType>(
               "All Heads", m_object, 0, saverc_cmd));
    updateMenu();
}

#endif // XINERAMA_HH
