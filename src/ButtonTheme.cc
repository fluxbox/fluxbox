#include "ButtonTheme.hh"
#include "FbTk/App.hh"

//!! TODO: still missing *.pressed.picColor
ButtonTheme::ButtonTheme(int screen_num, 
                         const std::string &name, 
                         const std::string &alt_name,
                         const std::string &extra_fallback,
                         const std::string &extra_fallback_alt):
    ToolTheme(screen_num, name, alt_name),
    m_pic_color(*this, name + ".picColor", alt_name + ".PicColor"),
    m_pressed_texture(*this, name + ".pressed", alt_name + ".Pressed"),
    m_gc(RootWindow(FbTk::App::instance()->display(), screen_num)),
    m_scale(*this, name + ".scale", alt_name + ".Scale"),
    m_name(name),
    m_fallbackname(extra_fallback), m_altfallbackname(extra_fallback_alt) {

}

bool ButtonTheme::fallback(FbTk::ThemeItem_base &item) {

/* Don't fallback these for theme backwards compatibility
    if (item.name().find(".borderWidth") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    }

    if (item.name().find(".borderColor") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    }
*/
    if (item.name() == name()) {
        // default to the toolbar label style
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       m_fallbackname,
                                                       m_altfallbackname);

    } else if (item.name().find(".picColor") != std::string::npos) {
        // if we've fallen back to alternate name, and it doesn't have a picColor, 
        // try its text color instead
        return FbTk::ThemeManager::instance().loadItem(item, 
                                                       m_fallbackname + ".picColor",
                                                       m_altfallbackname + ".picColor") ||
            FbTk::ThemeManager::instance().loadItem(item, 
                                                    m_fallbackname + ".textColor",
                                                    m_altfallbackname + ".TextColor");
    } else if (item.name().find(".pressed") != std::string::npos) {
        // copy texture
        *m_pressed_texture = texture();
        // invert the bevel if it has one!
        unsigned long type = m_pressed_texture->type();
        unsigned long bevels = (FbTk::Texture::SUNKEN | FbTk::Texture::RAISED);
        if ((type & bevels) != 0) {
            type ^= bevels;
            m_pressed_texture->setType(type);
        }
        return true;
    }

    return ToolTheme::fallback(item);
}

void ButtonTheme::reconfigTheme() {
    m_gc.setForeground(*m_pic_color);
}



