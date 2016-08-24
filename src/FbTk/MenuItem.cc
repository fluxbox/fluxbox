// MenuItem.cc for FbTk - Fluxbox Toolkit
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

#include "MenuItem.hh"
#include "Menu.hh"
#include "MenuTheme.hh"
#include "App.hh"
#include "Command.hh"
#include "Image.hh"
#include "GContext.hh"
#include "PixmapWithMask.hh"
#include "StringUtil.hh"

#include <X11/keysym.h>


namespace FbTk {

MenuItem::~MenuItem() { }

void MenuItem::click(int button, int time, unsigned int mods) {
    if (m_command.get() != 0) {
        if (m_menu && m_close_on_click && (mods & ControlMask) == 0)
            m_menu->hide();
        // we need a local variable, since the command may destroy this object
        RefCount<Command<void> > tmp(m_command);
        tmp->execute();
    }
}

void MenuItem::drawLine(FbDrawable &draw,
                        const FbTk::ThemeProxy<MenuTheme> &theme, size_t n_chars,
                        int text_x, int text_y, unsigned int width,
                        size_t skip_chars) const {

    // avoid drawing an ugly dot
    if (n_chars == 0) {
        return;
    }

    const FbString& text = m_label.visual();
    const size_t n = std::min(n_chars, text.size());
    const FbTk::Font& font = theme->hiliteFont();
    int font_height = static_cast<int>(font.height());
    int height = static_cast<int>(theme->itemHeight());
    int font_top = (height - font_height)/2;
    int bevel_width = static_cast<int>(theme->bevelWidth());
    int underline_height = font_top + font.ascent() + 2;
    int bottom = height - bevel_width - 1;
    int text_w = font.textWidth(label());
    int text_pixels = font.textWidth(text.c_str()+skip_chars, n);
    int skip_pixels = 0;
    if (skip_chars > 0) {
        skip_pixels = font.textWidth(text.c_str(), skip_chars);
    }

    text_y += std::min(bottom, underline_height);

    // pay attention to the text justification
    switch(theme->hiliteFontJustify()) {
    case FbTk::LEFT:
        text_x += bevel_width + height + 1;
        break;
    case FbTk::RIGHT:
        text_x += width - (height + bevel_width + text_w);
        break;
    default: //center
        text_x += ((width + 1 - text_w) / 2);
        break;
    }

    text_x += skip_pixels;

    draw.drawLine(theme->hiliteUnderlineGC().gc(),
                  text_x, text_y, text_x + text_pixels, text_y);

}

void MenuItem::draw(FbDrawable &draw,
                    const FbTk::ThemeProxy<MenuTheme> &theme,
                    bool highlight, bool draw_foreground, bool draw_background,
                    int x, int y,
                    unsigned int width, unsigned int height) const {

    // text and submenu icon are background
    // selected pixmaps are foreground

    int h = static_cast<int>(height);
    int bevel = theme->bevelWidth();
    Display *disp = App::instance()->display();

    //
    // Icon
    //
    if (draw_background) {
        if (icon() != 0) {
            // copy pixmap, so we don't resize the original
            FbPixmap tmp_pixmap, tmp_mask;
            tmp_pixmap.copy(icon()->pixmap());
            tmp_mask.copy(icon()->mask());

            // scale pixmap to right size
            if ((h - (2*bevel)) != static_cast<int>(tmp_pixmap.height())) {
                int scale_size = h - 2*bevel;
                if (scale_size > 0) {
                    tmp_pixmap.scale(scale_size, scale_size);
                    tmp_mask.scale(scale_size, scale_size);
                }
            }

            if (tmp_pixmap.drawable() != 0) {
                GC gc = theme->frameTextGC().gc();
                int icon_x = x + bevel;
                int icon_y = y + bevel;
                // enable clip mask
                XSetClipMask(disp, gc, tmp_mask.drawable());
                XSetClipOrigin(disp, gc, icon_x, icon_y);

                if (draw.depth() == tmp_pixmap.depth()) {
                    draw.copyArea(tmp_pixmap.drawable(),
                                  gc,
                                  0, 0,
                                  icon_x, icon_y,
                                  tmp_pixmap.width(), tmp_pixmap.height());
                } else { // TODO: wrong in soon-to-be-common circumstances
                    XGCValues backup;
                    XGetGCValues(draw.display(), gc, GCForeground|GCBackground,
                                 &backup);
                    XSetForeground(draw.display(), gc,
                                   Color("black", theme->screenNum()).pixel());
                    XSetBackground(draw.display(), gc,
                                   Color("white", theme->screenNum()).pixel());
                    XCopyPlane(draw.display(), tmp_pixmap.drawable(),
                               draw.drawable(), gc,
                               0, 0, tmp_pixmap.width(), tmp_pixmap.height(),
                               icon_x, icon_y, 1);
                    XSetForeground(draw.display(), gc, backup.foreground);
                    XSetBackground(draw.display(), gc, backup.background);
                }

                // restore clip mask
                XSetClipMask(disp, gc, None);
            }
        }
    }

    if (label().logical().empty())
        return;

    // text is background
    if (draw_background) {
        const GContext &tgc =
            (highlight ? theme->hiliteTextGC() :
             (isEnabled() ? theme->frameTextGC() : theme->disableTextGC() ) );
        const Font& font = (highlight ? theme->hiliteFont() : theme->frameFont());

        //
        // Text
        //
        int text_y = y;
        int text_x = x;
        int text_w = font.textWidth(label());

        int height_offset = theme->itemHeight() - (font.height() + 2*bevel);
        text_y = y + bevel + font.ascent() + height_offset/2;

        switch(highlight ? theme->hiliteFontJustify() : theme->frameFontJustify()) {
        case FbTk::LEFT:
            text_x = x + bevel + h + 1;
            break;

        case FbTk::RIGHT:
            text_x = x +  width - (h + bevel + text_w);
            break;
        default: //center
            text_x = x + ((width + 1 - text_w) / 2);
            break;
        }

        font.drawText(draw, theme->screenNum(), tgc.gc(), label(), text_x, text_y);
    }

    GC gc = (highlight) ? theme->hiliteTextGC().gc() :
        theme->frameTextGC().gc();
    int sel_x = x;
    int sel_y = y;
    int item_pm_height = static_cast<int>(theme->itemHeight());

    if (theme->bulletPos() == FbTk::RIGHT)
        sel_x += static_cast<int>(width) - h - bevel;


    // 'draw_foreground' was used to decide if to draw the toggleitem.
    // somehow(tm) this lead to not rendering the toggleitem on bringing
    // up the menu. it would be rendered once the user highlights the item,
    // but not before. i removed the check against 'draw_foreground' and
    // thus make it an unused variable.
    if (isToggleItem()) {

        const PixmapWithMask *pm = 0;

        if (isSelected()) {
            if (highlight && theme->highlightSelectedPixmap().pixmap().drawable() != 0)
                pm = &theme->highlightSelectedPixmap();
            else
                pm = &theme->selectedPixmap();
        } else {
            if (highlight && theme->highlightUnselectedPixmap().pixmap().drawable() != 0)
                pm = &theme->highlightUnselectedPixmap();
            else
                pm = &theme->unselectedPixmap();
        }

        if (pm != 0 && pm->pixmap().drawable() != 0) {
            int selw = static_cast<int>(pm->width());
            int selh = static_cast<int>(pm->height());
            int offset_x = 0;
            int offset_y = 0;
            if (selw < item_pm_height)
                offset_x += (item_pm_height - selw) / 2;
            if (selh < item_pm_height)
                offset_y += (item_pm_height - selh) / 2;

            XSetClipMask(disp, gc, pm->mask().drawable());
            XSetClipOrigin(disp, gc, sel_x+offset_x, sel_y+offset_y);
            // copy bullet pixmap to drawable
            draw.copyArea(pm->pixmap().drawable(),
                          gc,
                          0, 0,
                          sel_x+offset_x, sel_y+offset_y,
                          selw,
                          selh);
            // disable clip mask
            XSetClipMask(disp, gc, None);
        } else if (isSelected()) {
            draw.fillRectangle(theme->hiliteGC().gc(),
                               sel_x+item_pm_height/4, sel_y+item_pm_height/4, item_pm_height/2, item_pm_height/2);
        }
    }

    //
    // Submenu (background)
    //
    if (draw_background && submenu()) {

        const PixmapWithMask *pm = 0;

        if (highlight && theme->highlightBulletPixmap().pixmap().drawable() != 0)
            pm = &theme->highlightBulletPixmap();
        else
            pm = &theme->bulletPixmap();

        if (pm && pm->pixmap().drawable() != 0) {
            int selw = static_cast<int>(pm->width());
            int selh = static_cast<int>(pm->height());
            int offset_x = 0;
            int offset_y = 0;
            if (selw < item_pm_height)
                offset_x += (item_pm_height - selw) / 2;
            if (selh < item_pm_height)
                offset_y += (item_pm_height - selh) / 2;

            XSetClipMask(disp, gc, pm->mask().drawable());
            XSetClipOrigin(disp, gc, sel_x+offset_x, sel_y+offset_y);
            // copy bullet pixmap to drawable
            draw.copyArea(pm->pixmap().drawable(),
                          gc,
                          0, 0,
                          sel_x+offset_x, sel_y+offset_y,
                          selw,
                          selh);
            // disable clip mask
            XSetClipMask(disp, gc, None);

        } else {
            int half_w = item_pm_height / 2;
            int quarter_w = item_pm_height / 4;
            switch (theme->bullet()) {
            case MenuTheme::SQUARE:
                draw.drawRectangle(gc, sel_x+quarter_w, y+quarter_w, half_w, half_w);
                break;

            case MenuTheme::TRIANGLE:
                    draw.drawTriangle(gc, ((theme->bulletPos() == FbTk::RIGHT)?
                                           FbTk::FbDrawable::RIGHT:
                                           FbTk::FbDrawable::LEFT),
                                      sel_x, sel_y,
                                      item_pm_height,
                                      item_pm_height,
                                      300); // 33% triangle
                break;

            case MenuTheme::DIAMOND:
                XPoint dia[4];

                dia[0].x = sel_x + half_w - 3;
                dia[0].y = sel_y + half_w;
                dia[1].x = 3;
                dia[1].y = -3;
                dia[2].x = 3;
                dia[2].y = 3;
                dia[3].x = -3;
                dia[3].y = 3;

                draw.fillPolygon(gc, dia, 4, Convex,
                                 CoordModePrevious);
                break;
            default:
                break;
            }
        }
    }


}

void MenuItem::setIcon(const std::string &filename, int screen_num) {
    if (filename.empty()) {
        m_icon.reset(0);
        return;
    }

    if (m_icon.get() == 0)
        m_icon.reset(new Icon);

    m_icon->filename = FbTk::StringUtil::expandFilename(filename);
    m_icon->pixmap.reset(Image::load(m_icon->filename.c_str(),
                         screen_num));
}

unsigned int MenuItem::height(const FbTk::ThemeProxy<MenuTheme> &theme) const {
    const unsigned int bevel = theme->bevelWidth();
    return std::max(theme->itemHeight(),
                    std::max(theme->frameFont().height() + 2*bevel,
                             theme->hiliteFont().height() + 2*bevel));
}

unsigned int MenuItem::width(const FbTk::ThemeProxy<MenuTheme> &theme) const {
    // textwidth + bevel width on each side of the text
    const unsigned int icon_width = height(theme);
    const unsigned int normal = 2 * (theme->bevelWidth() + icon_width) +
                                std::max(theme->frameFont().textWidth(label()),
                                         theme->hiliteFont().textWidth(label()));
    return m_icon.get() == 0 ? normal : normal + icon_width;
}

void MenuItem::updateTheme(const FbTk::ThemeProxy<MenuTheme> &theme) {
    if (m_icon.get() == 0)
        return;

    m_icon->pixmap.reset(Image::load(m_icon->filename.c_str(), theme->screenNum()));


}

void MenuItem::showSubmenu() {
    if (submenu() != 0)
        submenu()->show();
}

} // end namespace FbTk
