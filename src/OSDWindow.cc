// OSDWindow.cc
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

#include "OSDWindow.hh"

#include "Screen.hh"
#include "FbWinFrameTheme.hh"

#include "FbTk/ImageControl.hh"

void OSDWindow::reconfigTheme() {

    setBorderWidth(m_theme->border().width());
    setBorderColor(m_theme->border().color());

    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);

    if (m_theme->iconbarTheme().texture().type() &
        FbTk::Texture::PARENTRELATIVE) {
        if (!m_theme->titleTexture().usePixmap()) {
            m_pixmap = None;
            setBackgroundColor(m_theme->titleTexture().color());
        } else {
            m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                    m_theme->titleTexture());
            setBackgroundPixmap(m_pixmap);
        }
    } else {
        if (!m_theme->iconbarTheme().texture().usePixmap()) {
            m_pixmap = None;
            setBackgroundColor(m_theme->iconbarTheme().texture().color());
        } else {
            m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                    m_theme->iconbarTheme().texture());
            setBackgroundPixmap(m_pixmap);
        }
    }

}

void OSDWindow::resizeForText(const FbTk::BiDiString &text) {

    int bw = 2 * m_theme->bevelWidth();
    int h = m_theme->font().height() + bw;
    int w = m_theme->font().textWidth(text) + bw;
    FbTk::FbWindow::resize(w, h);
}

void OSDWindow::showText(const FbTk::BiDiString &text) {
    show();
    clear();
    m_theme->font().drawText(*this, m_screen.screenNumber(),
            m_theme->iconbarTheme().text().textGC(), text,
            m_theme->bevelWidth(),
            m_theme->bevelWidth() + m_theme->font().ascent());
}

void OSDWindow::show() {
    if (m_visible)
        return;

    m_visible = true;
    unsigned int head = m_screen.getCurrHead();
    move(m_screen.getHeadX(head) + (m_screen.getHeadWidth(head) - width()) / 2,
         m_screen.getHeadY(head) + (m_screen.getHeadHeight(head) - height()) / 2);
    raise();
    FbTk::FbWindow::show();
}

void OSDWindow::hide() {
    if (!m_visible)
        return;
    m_visible = false;
    FbTk::FbWindow::hide();
}
