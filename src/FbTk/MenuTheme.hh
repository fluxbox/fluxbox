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

// $Id$

#ifndef FBTK_MENUTHEME_HH
#define FBTK_MENUTHEME_HH

#include "Theme.hh"
#include "Color.hh"
#include "Font.hh"
#include "Shape.hh"
#include "Texture.hh"
#include "Text.hh"
#include "Subject.hh"
#include "PixmapWithMask.hh"
#include "GContext.hh"

namespace FbTk {

class MenuTheme:public Theme {
public:
    //!! TODO
    // this isn't actually used with a theme item
    // see setMenuMode() for more info
    enum MenuMode {CLICK_OPEN, DELAY_OPEN};

    enum BulletType { EMPTY, SQUARE, TRIANGLE, DIAMOND};
    MenuTheme(int screen_num);
    virtual ~MenuTheme();

    void reconfigTheme();

    bool fallback(ThemeItem_base &item);

    /**
       @name text colors
    */
    ///@{
    inline const Color &titleTextColor() const { return *t_text; }
    inline const Color &frameTextColor() const { return *f_text; }
    inline const Color &frameUnderlineColor() const { return *u_text; }
    inline const Color &highlightTextColor() const { return *h_text; }
    inline const Color &disableTextColor() const { return *d_text; }
    ///@}
    /**
       @name textures
    */
    ///@{
    inline const Texture &titleTexture() const { return *title; }
    inline const Texture &frameTexture() const { return *frame; }
    inline const Texture &hiliteTexture() const { return *hilite; }
    ///@}

    inline const PixmapWithMask &bulletPixmap() const { return *m_bullet_pixmap; }
    inline const PixmapWithMask &selectedPixmap() const { return *m_selected_pixmap; }
    inline const PixmapWithMask &unselectedPixmap() const { return *m_unselected_pixmap; }

    inline const PixmapWithMask &highlightBulletPixmap() const { return *m_hl_bullet_pixmap; }
    inline const PixmapWithMask &highlightSelectedPixmap() const { return *m_hl_selected_pixmap; }
    inline const PixmapWithMask &highlightUnselectedPixmap() const { return *m_hl_unselected_pixmap; }
    /**
       @name fonts
    */
    ///@{
    inline const Font &titleFont() const { return *titlefont; }
    inline Font &titleFont() { return *titlefont; }
    inline const Font &frameFont() const { return *framefont; }
    inline Font &frameFont() { return *framefont; }
    ///@}

    inline Justify frameFontJustify() const { return *framefont_justify; }
    inline Justify titleFontJustify() const { return *titlefont_justify; }
	
    /**
       @name graphic contexts
    */
    ///@{
    inline const GContext &titleTextGC() const { return t_text_gc; }
    inline const GContext &frameTextGC() const { return f_text_gc; }
    inline const GContext &frameUnderlineGC() const { return u_text_gc; }
    inline const GContext &hiliteTextGC() const { return h_text_gc; }
    inline const GContext &disableTextGC() const { return d_text_gc; }
    inline const GContext &hiliteGC() const { return hilite_gc; }
    inline GContext &titleTextGC() { return t_text_gc; }
    inline GContext &frameTextGC() { return f_text_gc; }
    inline GContext &frameUnderlineGC() { return u_text_gc; }
    inline GContext &hiliteTextGC() { return h_text_gc; }
    inline GContext &disableTextGC() { return d_text_gc; }
    inline GContext &hiliteGC() { return hilite_gc; }
    ///@}
    inline BulletType bullet() const { return *m_bullet; }
    inline Justify bulletPos() const { return *bullet_pos; }

    inline unsigned int titleHeight() const { return m_real_title_height; }
    inline unsigned int itemHeight() const { return m_real_item_height; }
    inline unsigned int borderWidth() const { return *m_border_width; }
    inline unsigned int bevelWidth() const { return *m_bevel_width; }

    inline unsigned char alpha() const { return m_alpha; }
    inline void setAlpha(unsigned char alpha) { m_alpha = alpha; }
    // this isn't actually a theme item
    // but we'll let it be here for now, until there's a better way to
    // get resources into menu
    inline void setMenuMode(MenuMode mode) { m_menumode = mode; }
    inline MenuMode menuMode() const { return m_menumode; }
    inline void setDelayOpen(int msec) { m_delayopen = msec; }
    inline void setDelayClose(int msec) { m_delayclose = msec; }
    inline int delayOpen() const { return m_delayopen; }
    inline int delayClose() const { return m_delayclose; }
    
    inline const Color &borderColor() const { return *m_border_color; }
    inline Shape::ShapePlace shapePlaces() const { return *m_shapeplace; }

    // special override
    inline void setSelectedPixmap(Pixmap pm, bool is_imagecached) {
        m_selected_pixmap->pixmap() = pm; 
        if (is_imagecached)
            m_selected_pixmap->pixmap().dontFree();
    }

    inline void setHighlightSelectedPixmap(Pixmap pm, bool is_imagecached) {
        m_hl_selected_pixmap->pixmap() = pm; 
        if (is_imagecached)
            m_hl_selected_pixmap->pixmap().dontFree();
    }

private:
    ThemeItem<Color> t_text, f_text, h_text, d_text, u_text;
    ThemeItem<Texture> title, frame, hilite;
    ThemeItem<Font> titlefont, framefont;
    ThemeItem<Justify> framefont_justify, titlefont_justify;
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

    unsigned char m_alpha;
    MenuMode m_menumode;
    unsigned int m_delayopen; ///< in msec
    unsigned int m_delayclose; ///< in msec
    unsigned int m_real_title_height; ///< the calculated item height (from font and menu.titleHeight)
    unsigned int m_real_item_height; ///< the calculated item height (from font and menu.itemHeight)
};

} // end namespace FbTk

#endif // FBTK_MENUTHEME_HH
