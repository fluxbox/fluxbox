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

#ifndef FBWINFRAMETHEME_HH
#define FBWINFRAMETHEME_HH

#include "FbTk/Font.hh"
#include "FbTk/Texture.hh"
#include "FbTk/Color.hh"
#include "FbTk/Theme.hh"
#include "FbTk/BorderTheme.hh"
#include "FbTk/GContext.hh"
#include "FbTk/Shape.hh"

#include "IconbarTheme.hh"
#include <string>

class FbWinFrameTheme: public FbTk::Theme,
                       public FbTk::ThemeProxy<FbWinFrameTheme> {
public:
    explicit FbWinFrameTheme(int screen_num, const std::string &extra,
                             const std::string &altextra);
    ~FbWinFrameTheme();
    /**
       @name textures
    */
    //@{
    const FbTk::Texture &titleTexture() const { return *m_title; }
    const FbTk::Texture &handleTexture() const { return *m_handle; }
    const FbTk::Texture &buttonTexture() const { return *m_button; }
    const FbTk::Texture &buttonPressedTexture() const { return *m_button_pressed; }
    const FbTk::Texture &gripTexture() const { return *m_grip; }
    //@}

    const FbTk::Color &buttonColor() const { return *m_button_color; }
    FbTk::Font &font() { return *m_font; }
    GC buttonPicGC() const { return m_button_pic_gc.gc(); }

    bool fallback(FbTk::ThemeItem_base &item);
    void reconfigTheme();

    Cursor moveCursor() const { return m_cursor_move; }
    Cursor lowerLeftAngleCursor() const { return m_cursor_lower_left_angle; }
    Cursor lowerRightAngleCursor() const { return m_cursor_lower_right_angle; }
    Cursor upperLeftAngleCursor() const { return m_cursor_upper_left_angle; }
    Cursor upperRightAngleCursor() const { return m_cursor_upper_right_angle; }
    Cursor leftSideCursor() const { return m_cursor_left_side; }
    Cursor rightSideCursor() const { return m_cursor_right_side; }
    Cursor topSideCursor() const { return m_cursor_top_side; }
    Cursor bottomSideCursor() const { return m_cursor_bottom_side; }

    FbTk::Shape::ShapePlace shapePlace() const { return *m_shape_place; }
    const FbTk::BorderTheme &border() const { return m_border; }

    unsigned int titleHeight() const { return *m_title_height; }
    unsigned int bevelWidth() const { return *m_bevel_width; }
    unsigned int handleWidth() const { return *m_handle_width; }

    int alpha() const { return m_alpha; }
    void setAlpha(int alpha) { m_alpha = alpha; }

    IconbarTheme &iconbarTheme() { return m_iconbar_theme; }

    virtual FbTk::Signal<> &reconfigSig() { return FbTk::Theme::reconfigSig(); }

    virtual FbWinFrameTheme &operator *() { return *this; }
    virtual const FbWinFrameTheme &operator *() const { return *this; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_title, m_handle, m_button,
                                   m_button_pressed, m_grip;

    FbTk::ThemeItem<FbTk::Color> m_button_color;
    FbTk::ThemeItem<FbTk::Font> m_font;
    FbTk::ThemeItem<FbTk::Shape::ShapePlace> m_shape_place;

    FbTk::ThemeItem<int> m_title_height, m_bevel_width, m_handle_width;
    FbTk::BorderTheme m_border;

    FbTk::GContext m_button_pic_gc;

    Cursor m_cursor_move;
    Cursor m_cursor_lower_left_angle;
    Cursor m_cursor_lower_right_angle;
    Cursor m_cursor_upper_left_angle;
    Cursor m_cursor_upper_right_angle;
    Cursor m_cursor_left_side;
    Cursor m_cursor_right_side;
    Cursor m_cursor_top_side;
    Cursor m_cursor_bottom_side;
    int m_alpha;

    IconbarTheme m_iconbar_theme;
};

#endif // FBWINFRAMETHEME_HH


