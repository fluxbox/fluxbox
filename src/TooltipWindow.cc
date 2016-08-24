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
    m_delay(-1) {

    FbTk::RefCount<FbTk::Command<void> > 
        raisecmd(new FbTk::SimpleCommand<TooltipWindow>(*this, 
                                                        &TooltipWindow::raiseTooltip));
    m_timer.setCommand(raisecmd);
    m_timer.fireOnce(true);

}

void TooltipWindow::showText(const FbTk::BiDiString& text) {

    m_lastText = text;
    if (m_delay == 0)
        raiseTooltip();
    else
        m_timer.start();

}

void TooltipWindow::raiseTooltip() {

    if (m_lastText.logical().empty())
        return;

    resizeForText(m_lastText);
    reconfigTheme();

    FbTk::Font& font = theme()->iconbarTheme().text().font();

    int h = font.height() + theme()->bevelWidth() * 2;
    int w = font.textWidth(m_lastText) + theme()->bevelWidth() * 2;

    Window root_ret; // not used
    Window window_ret; // not used
    int rx = 0, ry = 0;
    int wx, wy; // not used
    unsigned int mask; // not used

    XQueryPointer(display(), screen().rootWindow().window(),
                  &root_ret, &window_ret, &rx, &ry, &wx, &wy, &mask);

    int head = screen().getHead(rx, ry);
    int head_top = screen().getHeadY(head);
    int head_left = screen().getHeadX(head);
    int head_right = head_left + screen().getHeadWidth(head);

    // center the mouse horizontally
    rx -= w/2;
    int yoffset = 10;
    if (ry - yoffset - h >= head_top)
        ry -= yoffset + h;
    else
        ry += yoffset;

    // check that we are not out of screen
    if (rx + w > head_right)
        rx = head_right - w;
    if (rx < head_left)
        rx = head_left;

    moveResize(rx,ry,w, h);

    show();
    clear();
    // TODO: make this use a TextButton like TextDialog does
    font.drawText(*this, screen().screenNumber(),
                  theme()->iconbarTheme().text().textGC(), 
                  m_lastText,
                  theme()->bevelWidth(),
                  theme()->bevelWidth() + font.ascent());
}

void TooltipWindow::updateText(const FbTk::BiDiString& text) {
    m_lastText = text;
    raiseTooltip();
}

void TooltipWindow::show() {
    if (isVisible())
        return;
    setVisible(true);
    raise();
    FbTk::FbWindow::show();
}

void TooltipWindow::hide() {
    m_timer.stop();
    OSDWindow::hide();
}
