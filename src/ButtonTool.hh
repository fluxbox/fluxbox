// ButtonTool.hh for Fluxbox
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

#ifndef BUTTONTOOL_HH
#define BUTTONTOOL_HH

#include "GenericTool.hh"

#include <X11/Xlib.h>

class ButtonTheme;

namespace FbTk {
class Button;
class ImageControl;
}

class ButtonTool: public GenericTool {
public:
    ButtonTool(FbTk::Button *button, ToolbarItem::Type type, 
               FbTk::ThemeProxy<ButtonTheme> &theme,
               FbTk::ImageControl &img_ctrl);
    virtual ~ButtonTool();
    void setOrientation(FbTk::Orientation orient);

protected:
    void renderTheme(int alpha);
    void updateSizing();
    Pixmap m_cache_pm, m_cache_pressed_pm;
    FbTk::ImageControl &m_image_ctrl;
};

#endif // BUTTONTOOL_HH
