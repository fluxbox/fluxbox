// ToolbarTheme.hh  a theme class for Toolbar
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: ToolbarTheme.hh,v 1.7 2003/08/13 09:53:35 fluxgen Exp $

#ifndef TOOLBARTHEME_HH
#define TOOLBARTHEME_HH

#include "FbTk/Theme.hh"
#include "Font.hh"
#include "Texture.hh"
#include "Color.hh"
#include "Text.hh"
#include "Subject.hh"

/// toolbar theme class container
class ToolbarTheme: public FbTk::Theme {
public:
    explicit ToolbarTheme(int screen_num);
    virtual ~ToolbarTheme();

    void reconfigTheme();
    
    /**
       @name colors
    */
    ///@{
    const FbTk::Color &buttonColor() const { return *m_button_color; }
    const FbTk::Color &borderColor() const { return *m_border_color; }
    ///@}
    /**
       @name textures
    */
    ///@{
    const FbTk::Texture &toolbar() const { return *m_toolbar; }
    const FbTk::Texture &button() const { return *m_button; }
    const FbTk::Texture &pressedButton() const { return *m_pressed_button; }
    ///@}

    /**
       @name graphic context
     */
    ///@{
    GC buttonPicGC() const { return m_button_pic_gc; }
    ///@}

    inline int borderWidth() const { return *m_border_width; }
    inline int bevelWidth() const { return *m_bevel_width; }    
    inline int buttonBorderWidth() const { return *m_button_border_width; }
    inline bool shape() const { return *m_shape; }
    inline unsigned char alpha() const { return *m_alpha; }

private:
    // text colors
    FbTk::ThemeItem<FbTk::Color> m_button_color, m_border_color;
    // textures
    FbTk::ThemeItem<FbTk::Texture> m_toolbar,  m_button, m_pressed_button;

    FbTk::ThemeItem<int> m_border_width, m_bevel_width, m_button_border_width;
    FbTk::ThemeItem<bool> m_shape;
    FbTk::ThemeItem<int> m_alpha;

    // graphic context
    GC m_button_pic_gc;

    Display *m_display;

};

#endif // TOOLBARTHEME_HH
