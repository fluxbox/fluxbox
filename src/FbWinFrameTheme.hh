// FbWinFrameTheme.hh for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: FbWinFrameTheme.hh,v 1.2 2003/02/15 01:55:27 fluxgen Exp $

#ifndef FBWINFRAMETHEME_HH
#define FBWINFRAMETHEME_HH

#include "Font.hh"
#include "Texture.hh"
#include "Text.hh"
#include "Color.hh"
#include "FbTk/Theme.hh"
#include "Subject.hh"

class FbWinFrameTheme: public FbTk::Theme {
public:
    FbWinFrameTheme(int screen_num);
    ~FbWinFrameTheme();

    /**
       @name textures
    */
    //@{
    const FbTk::Texture &labelFocusTexture() const { return *m_label_focus; }
    const FbTk::Texture &labelUnfocusTexture() const { return *m_label_unfocus; }

    const FbTk::Texture &titleFocusTexture() const { return *m_title_focus; }
    const FbTk::Texture &titleUnfocusTexture() const { return *m_title_unfocus; }

    const FbTk::Texture &handleFocusTexture() const { return *m_handle_focus; }
    const FbTk::Texture &handleUnfocusTexture() const { return *m_handle_unfocus; }

    const FbTk::Texture &buttonFocusTexture() const { return *m_button_focus; }
    const FbTk::Texture &buttonUnfocusTexture() const { return *m_button_unfocus; }
    const FbTk::Texture &buttonPressedTexture() const { return *m_button_pressed; }    

    const FbTk::Texture &gripFocusTexture() const { return *m_grip_focus; }
    const FbTk::Texture &gripUnfocusTexture() const { return *m_grip_unfocus; }
    //@}

    /**
       @name colors
    */
    //@{
    const FbTk::Color &labelFocusColor() const { return *m_label_focus_color; }
    const FbTk::Color &labelUnfocusColor() const { return *m_label_unfocus_color; }
    const FbTk::Color &frameFocuscolor() const { return *m_frame_focus_color; }
    const FbTk::Color &frameUnfocuscolor() const { return *m_frame_unfocus_color; }
    const FbTk::Color &buttonFocuscolor() const { return *m_button_focus_color; }
    const FbTk::Color &buttonUnfocuscolor() const { return *m_button_unfocus_color; }
    //@}
    const FbTk::Font &font() const {  return *m_font; }
    FbTk::Font &font() { return *m_font; }

    FbTk::Justify justify() const { return *m_textjustify; }

    GC labelTextFocusGC() const { return m_label_text_focus_gc; }
    GC labelTextUnfocusGC() const { return m_label_text_unfocus_gc; }

    void reconfigTheme();

    void addListener(FbTk::Observer &obs) { m_theme_change.attach(&obs); }
    void removeListener(FbTk::Observer &obs) { m_theme_change.detach(&obs); }
private:
    FbTk::ThemeItem<FbTk::Texture> m_label_focus, m_label_unfocus;
    FbTk::ThemeItem<FbTk::Texture> m_title_focus, m_title_unfocus;
    FbTk::ThemeItem<FbTk::Texture> m_handle_focus, m_handle_unfocus;
    FbTk::ThemeItem<FbTk::Texture> m_button_focus, m_button_unfocus, m_button_pressed;
    FbTk::ThemeItem<FbTk::Texture> m_grip_focus, m_grip_unfocus;

    FbTk::ThemeItem<FbTk::Color> m_label_focus_color, m_label_unfocus_color;
    FbTk::ThemeItem<FbTk::Color> m_frame_focus_color, m_frame_unfocus_color;
    FbTk::ThemeItem<FbTk::Color> m_button_focus_color, m_button_unfocus_color;
    
    FbTk::ThemeItem<FbTk::Font> m_font;
    FbTk::ThemeItem<FbTk::Justify> m_textjustify;

    GC m_label_text_focus_gc, m_label_text_unfocus_gc;

    FbTk::Subject m_theme_change;
};

#endif // FBWINFRAMETHEME_HH


