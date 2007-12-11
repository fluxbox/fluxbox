// FbWinFrameTheme.hh for Fluxbox Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef FBWINFRAMETHEME_HH
#define FBWINFRAMETHEME_HH

#include "FbTk/Font.hh"
#include "FbTk/Texture.hh"
#include "FbTk/Text.hh"
#include "FbTk/Color.hh"
#include "FbTk/Theme.hh"
#include "FbTk/Subject.hh"
#include "FbTk/GContext.hh"

#include "BorderTheme.hh"
#include "IconbarTheme.hh"
#include "Shape.hh"

class FbWinFrameTheme: public FbTk::Theme {
public:
    explicit FbWinFrameTheme(int screen_num);
    ~FbWinFrameTheme();
    /**
       @name textures
    */
    //@{
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
    const FbTk::Color &buttonFocuscolor() const { return *m_button_focus_color; }
    const FbTk::Color &buttonUnfocuscolor() const { return *m_button_unfocus_color; }
    //@}
    FbTk::Font &font() { return *m_font; }

    GC buttonPicFocusGC() const { return m_button_pic_focus_gc.gc(); }
    GC buttonPicUnfocusGC() const { return m_button_pic_unfocus_gc.gc(); }

    bool fallback(FbTk::ThemeItem_base &item);
    void reconfigTheme();

    inline Cursor moveCursor() const { return m_cursor_move; }
    inline Cursor lowerLeftAngleCursor() const { return m_cursor_lower_left_angle; }
    inline Cursor lowerRightAngleCursor() const { return m_cursor_lower_right_angle; }
    inline Cursor upperLeftAngleCursor() const { return m_cursor_upper_left_angle; }
    inline Cursor upperRightAngleCursor() const { return m_cursor_upper_right_angle; }
    inline Cursor leftSideCursor() const { return m_cursor_left_side; }
    inline Cursor rightSideCursor() const { return m_cursor_right_side; }
    inline Cursor topSideCursor() const { return m_cursor_top_side; }
    inline Cursor bottomSideCursor() const { return m_cursor_bottom_side; }

    inline Shape::ShapePlace shapePlace() const { return *m_shape_place; }
    inline const BorderTheme &border() const { return m_border; }

    unsigned int titleHeight() const { return *m_title_height; }
    unsigned int bevelWidth() const { return *m_bevel_width; }
    unsigned int handleWidth() const { return *m_handle_width; }

    unsigned char focusedAlpha() const { return m_focused_alpha; }
    unsigned char unfocusedAlpha() const { return m_unfocused_alpha; }
    void setFocusedAlpha(unsigned char alpha) { m_focused_alpha = alpha; }
    void setUnfocusedAlpha(unsigned char alpha) { m_unfocused_alpha = alpha; }

    IconbarTheme &iconbarTheme() { return m_iconbar_theme; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_title_focus, m_title_unfocus;
    FbTk::ThemeItem<FbTk::Texture> m_handle_focus, m_handle_unfocus;
    FbTk::ThemeItem<FbTk::Texture> m_button_focus, m_button_unfocus, m_button_pressed;
    FbTk::ThemeItem<FbTk::Texture> m_grip_focus, m_grip_unfocus;

    FbTk::ThemeItem<FbTk::Color> m_button_focus_color, m_button_unfocus_color;
    
    FbTk::ThemeItem<FbTk::Font> m_font;
    FbTk::ThemeItem<Shape::ShapePlace> m_shape_place;

    FbTk::ThemeItem<int> m_title_height, m_bevel_width, m_handle_width;
    BorderTheme m_border;

    FbTk::GContext m_button_pic_focus_gc, m_button_pic_unfocus_gc;

    Cursor m_cursor_move;
    Cursor m_cursor_lower_left_angle;
    Cursor m_cursor_lower_right_angle;
    Cursor m_cursor_upper_left_angle;
    Cursor m_cursor_upper_right_angle;
    Cursor m_cursor_left_side;
    Cursor m_cursor_right_side;
    Cursor m_cursor_top_side;
    Cursor m_cursor_bottom_side;
    unsigned char m_focused_alpha;
    unsigned char m_unfocused_alpha;

    IconbarTheme m_iconbar_theme;
};

#endif // FBWINFRAMETHEME_HH


