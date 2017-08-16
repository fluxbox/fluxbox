// ButtonTool.cc for Fluxbox
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "ButtonTool.hh"
#include "ButtonTheme.hh"
#include "FbTk/Button.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/TextButton.hh"

ButtonTool::ButtonTool(FbTk::Button *button, 
                       ToolbarItem::Type type, 
                       FbTk::ThemeProxy<ButtonTheme> &theme,
                       FbTk::ImageControl &img_ctrl):
    GenericTool(button, type, dynamic_cast<FbTk::ThemeProxy<ToolTheme> &>(theme)),
    m_cache_pm(0),
    m_cache_pressed_pm(0),
    m_image_ctrl(img_ctrl) {

}

ButtonTool::~ButtonTool() {
    if (m_cache_pm)
        m_image_ctrl.removeImage(m_cache_pm);

    if (m_cache_pressed_pm)
        m_image_ctrl.removeImage(m_cache_pressed_pm);

}

void ButtonTool::updateSizing() {
    FbTk::Button &btn = static_cast<FbTk::Button &>(window());
    int bw = theme()->border().width();
    btn.setBorderWidth(bw);
    if (FbTk::TextButton *txtBtn = dynamic_cast<FbTk::TextButton*>(&btn)) {
        bw += 2; // extra padding, seems somehow required...

		unsigned int new_width = theme()->font().textWidth(txtBtn->text()) + 2*bw;
		unsigned int new_height = theme()->font().height() + 2*bw;

		if (orientation() == FbTk::ROT0 || orientation() == FbTk::ROT180)  {
			resize(new_width, new_height);
		} else {
			resize(new_height, new_width);
		}
	}
}

void ButtonTool::renderTheme(int alpha) {
    FbTk::Button &btn = static_cast<FbTk::Button &>(window());

    btn.setGC(static_cast<const ButtonTheme &>(*theme()).gc());
    btn.setBorderColor(theme()->border().color());
    btn.setBorderWidth(theme()->border().width());
    btn.setAlpha(alpha);
    btn.updateTheme(*theme());

    Pixmap old_pm = m_cache_pm;
    if (!theme()->texture().usePixmap()) {
        m_cache_pm = 0;
        btn.setBackgroundColor(theme()->texture().color());
    } else {
        m_cache_pm = m_image_ctrl.renderImage(width(), height(),
                                              theme()->texture(), orientation());
        btn.setBackgroundPixmap(m_cache_pm);
    }
    if (old_pm)
        m_image_ctrl.removeImage(old_pm);

    old_pm = m_cache_pressed_pm;
    if (! static_cast<const ButtonTheme &>(*theme()).pressed().usePixmap()) {
        m_cache_pressed_pm = 0;
        btn.setPressedColor(static_cast<const ButtonTheme &>(*theme()).pressed().color());
    } else {
        m_cache_pressed_pm = m_image_ctrl.renderImage(width(), height(),
                                                      static_cast<const ButtonTheme &>(*theme()).pressed(), orientation());
        btn.setPressedPixmap(m_cache_pressed_pm);
    }

    if (old_pm)
        m_image_ctrl.removeImage(old_pm);

    btn.clear();
}

void ButtonTool::setOrientation(FbTk::Orientation orient) {
    FbTk::Button &btn = static_cast<FbTk::Button &>(window());
    btn.setOrientation(orient);
    ToolbarItem::setOrientation(orient);
}

