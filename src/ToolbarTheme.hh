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

// $Id: ToolbarTheme.hh,v 1.6 2003/08/11 16:54:46 fluxgen Exp $

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
    const FbTk::Color &labelTextColor() const { return *m_label_textcolor; }
    const FbTk::Color &windowTextColor() const { return *m_window_textcolor; }
    const FbTk::Color &clockTextColor() const { return *m_clock_textcolor; }
    const FbTk::Color &buttonColor() const { return *m_button_color; }
    const FbTk::Color &borderColor() const { return *m_border_color; }
    ///@}
    /**
       @name textures
    */
    ///@{
    const FbTk::Texture &toolbar() const { return *m_toolbar; }
    const FbTk::Texture &iconbarFocused() const { return *m_iconbar_focused; }
    const FbTk::Texture &iconbarUnfocused() const { return *m_iconbar_unfocused; }
    const FbTk::Texture &label() const { return *m_label; }
    const FbTk::Texture &window() const { return *m_window; }
    const FbTk::Texture &button() const { return *m_button; }
    const FbTk::Texture &pressedButton() const { return *m_pressed_button; }
    const FbTk::Texture &clock() const { return *m_clock; }    
    ///@}
    const FbTk::Font &font() const { return *m_font; }
    FbTk::Font &font() { return *m_font; }
    const FbTk::Font &iconFont() const { return *m_icon_font; }
    FbTk::Font &iconFont() { return *m_icon_font; }
    /**
       @name graphic context
     */
    ///@{
    GC labelTextGC() const { return m_label_text_gc; }
    GC windowTextGC() const { return m_window_text_gc; }
    GC clockTextGC() const { return m_clock_text_gc; }
    GC buttonPicGC() const { return m_button_pic_gc; }
    GC iconTextFocusedGC() const { return m_icon_text_focused_gc; }
    GC iconTextUnfocusedGC() const { return m_icon_text_unfocused_gc; }
    ///@}
    FbTk::Justify justify() const { return *m_justify; }

    inline int borderWidth() const { return *m_border_width; }
    inline int bevelWidth() const { return *m_bevel_width; }    
    inline int buttonBorderWidth() const { return *m_button_border_width; }
    inline bool shape() const { return *m_shape; }
    inline unsigned char alpha() const { return *m_alpha; }

private:
    // text colors
    FbTk::ThemeItem<FbTk::Color> m_label_textcolor, m_window_textcolor, m_clock_textcolor;
    FbTk::ThemeItem<FbTk::Color> m_button_color, m_border_color;
    // textures
    FbTk::ThemeItem<FbTk::Texture> m_toolbar, m_iconbar_focused, m_iconbar_unfocused, 
        m_label, m_window, m_button, m_pressed_button, m_clock;
    FbTk::ThemeItem<FbTk::Font> m_font, m_icon_font;
    FbTk::ThemeItem<FbTk::Justify> m_justify;

    FbTk::ThemeItem<int> m_border_width, m_bevel_width, m_button_border_width;
    FbTk::ThemeItem<bool> m_shape;
    FbTk::ThemeItem<int> m_alpha;

    // graphic context
    GC m_label_text_gc, m_window_text_gc, m_clock_text_gc, m_button_pic_gc;
    GC m_icon_text_unfocused_gc, m_icon_text_focused_gc;
    Display *m_display;

};

#endif // TOOLBARTHEME_HH
