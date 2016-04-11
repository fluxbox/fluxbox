// MenuTheme.hh for FbTk
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBTK_MENUTHEME_HH
#define FBTK_MENUTHEME_HH

#include "Theme.hh"
#include "Color.hh"
#include "Font.hh"
#include "Shape.hh"
#include "Texture.hh"
#include "PixmapWithMask.hh"
#include "GContext.hh"

namespace FbTk {

class MenuTheme: public Theme, public ThemeProxy<MenuTheme> {
public:
    enum BulletType { EMPTY, SQUARE, TRIANGLE, DIAMOND};
    MenuTheme(int screen_num);
    virtual ~MenuTheme();

    void reconfigTheme();

    bool fallback(ThemeItem_base &item);

    /**
       @name text colors
    */
    ///@{
    const Color &titleTextColor() const { return *t_text; }
    const Color &frameTextColor() const { return *f_text; }
    const Color &frameUnderlineColor() const { return *u_text; }
    const Color &highlightTextColor() const { return *h_text; }
    const Color &disableTextColor() const { return *d_text; }
    ///@}
    /**
       @name textures
    */
    ///@{
    const Texture &titleTexture() const { return *title; }
    const Texture &frameTexture() const { return *frame; }
    const Texture &hiliteTexture() const { return *hilite; }
    ///@}

    const PixmapWithMask &bulletPixmap() const { return *m_bullet_pixmap; }
    const PixmapWithMask &selectedPixmap() const { return *m_selected_pixmap; }
    const PixmapWithMask &unselectedPixmap() const { return *m_unselected_pixmap; }

    const PixmapWithMask &highlightBulletPixmap() const { return *m_hl_bullet_pixmap; }
    const PixmapWithMask &highlightSelectedPixmap() const { return *m_hl_selected_pixmap; }
    const PixmapWithMask &highlightUnselectedPixmap() const { return *m_hl_unselected_pixmap; }
    /**
       @name fonts
    */
    ///@{
    const Font &titleFont() const { return *titlefont; }
    Font &titleFont() { return *titlefont; }
    const Font &frameFont() const { return *framefont; }
    Font &frameFont() { return *framefont; }
    const Font &hiliteFont() const { return *hilitefont; }
    Font &hiliteFont() { return *hilitefont; }
    ///@}

    Justify frameFontJustify() const { return *framefont_justify; }
    Justify hiliteFontJustify() const { return *hilitefont_justify; }
    Justify titleFontJustify() const { return *titlefont_justify; }

    /**
       @name graphic contexts
    */
    ///@{
    const GContext &titleTextGC() const { return t_text_gc; }
    const GContext &frameTextGC() const { return f_text_gc; }
    const GContext &hiliteUnderlineGC() const { return u_text_gc; }
    const GContext &hiliteTextGC() const { return h_text_gc; }
    const GContext &disableTextGC() const { return d_text_gc; }
    const GContext &hiliteGC() const { return hilite_gc; }
    GContext &titleTextGC() { return t_text_gc; }
    GContext &frameTextGC() { return f_text_gc; }
    GContext &hiliteUnderlineGC() { return u_text_gc; }
    GContext &hiliteTextGC() { return h_text_gc; }
    GContext &disableTextGC() { return d_text_gc; }
    GContext &hiliteGC() { return hilite_gc; }
    ///@}
    BulletType bullet() const { return *m_bullet; }
    Justify bulletPos() const { return *bullet_pos; }

    unsigned int titleHeight(bool fontConstrained = false) const {
        return fontConstrained ? m_real_title_height : *m_title_height;
    }
    unsigned int itemHeight() const { return m_real_item_height; }
    unsigned int borderWidth() const { return *m_border_width; }
    unsigned int bevelWidth() const { return *m_bevel_width; }

    unsigned char alpha() const { return m_alpha; }
    void setAlpha(int alpha) { m_alpha = alpha; }
    // this isn't actually a theme item
    // but we'll let it be here for now, until there's a better way to
    // get resources into menu
    void setDelay(int msec) { m_delay = msec; }
    int getDelay() const { return m_delay; }

    const Color &borderColor() const { return *m_border_color; }
    Shape::ShapePlace shapePlaces() const { return *m_shapeplace; }

    // special override
    void setSelectedPixmap(Pixmap pm, bool is_imagecached) {
        m_selected_pixmap->pixmap() = pm;
        if (is_imagecached)
            m_selected_pixmap->pixmap().dontFree();
    }

    void setHighlightSelectedPixmap(Pixmap pm, bool is_imagecached) {
        m_hl_selected_pixmap->pixmap() = pm;
        if (is_imagecached)
            m_hl_selected_pixmap->pixmap().dontFree();
    }

    virtual Signal<> &reconfigSig() { return Theme::reconfigSig(); }

    virtual MenuTheme &operator *() { return *this; }
    virtual const MenuTheme &operator *() const { return *this; }

private:
    ThemeItem<Color> t_text, f_text, h_text, d_text, u_text;
    ThemeItem<Texture> title, frame, hilite;
    ThemeItem<Font> titlefont, framefont, hilitefont;
    ThemeItem<Justify> framefont_justify, hilitefont_justify, titlefont_justify;
    ThemeItem<Justify> bullet_pos;
    ThemeItem<BulletType> m_bullet;
    ThemeItem<Shape::ShapePlace> m_shapeplace;
    ThemeItem<unsigned int> m_title_height, m_item_height;
    ThemeItem<unsigned int> m_border_width;
    ThemeItem<unsigned int> m_bevel_width;
    ThemeItem<Color> m_border_color;
    ThemeItem<PixmapWithMask> m_bullet_pixmap, m_selected_pixmap, m_unselected_pixmap;
    ThemeItem<PixmapWithMask> m_hl_bullet_pixmap, m_hl_selected_pixmap, m_hl_unselected_pixmap;

    Display *m_display;
    GContext t_text_gc, f_text_gc, u_text_gc, h_text_gc, d_text_gc, hilite_gc;

    int m_alpha;
    unsigned int m_delay; ///< in msec
    unsigned int m_real_title_height; ///< the calculated item height (from font and menu.titleHeight)
    unsigned int m_real_item_height; ///< the calculated item height (from font and menu.itemHeight)
};

} // end namespace FbTk

#endif // FBTK_MENUTHEME_HH
