// TooltipWindow.hh
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


#include "TooltipWindow.hh"
#include "Screen.hh"
#include "FbWinFrameTheme.hh"


TooltipWindow::TooltipWindow(const FbTk::FbWindow &parent, BScreen &screen,
                             FbTk::ThemeProxy<FbWinFrameTheme> &theme):
    OSDWindow(parent, screen, theme),
    delay(-1) {

    FbTk::RefCount<FbTk::Command<void> > raisecmd(new FbTk::SimpleCommand<TooltipWindow>(*this, &TooltipWindow::raiseTooltip));
    timer.setCommand(raisecmd);
    timer.fireOnce(true);

}

void TooltipWindow::showText(const std::string &text) {

    lastText = text.c_str();
    if (delay == 0)
        raiseTooltip();
    else
        timer.start();

}

void TooltipWindow::raiseTooltip() {

    if (lastText.size() == 0)
        return;

    resize(lastText);
    reconfigTheme();
    int h = m_theme->font().height() + m_theme->bevelWidth() * 2;
    int w = m_theme->font().textWidth(lastText, lastText.size()) + m_theme->bevelWidth() * 2;

    Window root_ret; // not used
    Window window_ret; // not used
    int rx = 0, ry = 0;
    int wx, wy; // not used
    unsigned int mask; // not used

    XQueryPointer(display(), m_screen.rootWindow().window(),
                  &root_ret, &window_ret, &rx, &ry, &wx, &wy, &mask);

    // mouse position
    int mx = rx;
    int my = ry;

    // center the mouse horizontally
    rx -= w/2;
    int yoffset = 10;
    if (ry >= yoffset + h)
        ry -= yoffset + h;
    else
        ry += yoffset;

    // check that we are not out of screen
    int outOfBound = rx + w - m_screen.width(); 
    if (outOfBound > 0)
        rx -= outOfBound;
    if (rx < 0)
        rx = 0;

    moveResize(rx,ry,w, h);

    show();
    clear();
    m_theme->font().drawText(*this, m_screen.screenNumber(),
                             m_theme->iconbarTheme().text().textGC(), lastText,
                             lastText.size(), m_theme->bevelWidth(),
                             m_theme->bevelWidth() + m_theme->font().ascent());
}


void TooltipWindow::show() {
    if (m_visible)
        return;
    m_visible = true;
    raise();
    FbTk::FbWindow::show();
}

void TooltipWindow::hide() {
    timer.stop();
    OSDWindow::hide();
}
