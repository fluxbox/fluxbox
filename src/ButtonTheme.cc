#include "ButtonTheme.hh"
#include "FbTk/App.hh"

//!! TODO: still missing *.pressed.picColor
ButtonTheme::ButtonTheme(int screen_num, 
                         const std::string &name, 
                         const std::string &alt_name):
    ToolTheme(screen_num, name, alt_name),
    m_pic_color(*this, name + ".picColor", alt_name + ".PicColor"),
    m_pressed_texture(*this, name + ".pressed", alt_name + ".Pressed"),
    m_gc(RootWindow(FbTk::App::instance()->display(), screen_num)) {

}

bool ButtonTheme::fallback(FbTk::ThemeItem_base &item) {

    if (item.name().find(".borderWidth") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderWidth", "BorderWidth");
    }

    if (item.name().find(".borderColor") != std::string::npos) {
        return FbTk::ThemeManager::instance().loadItem(item, "borderColor", "BorderColor");
    }

    return ToolTheme::fallback(item);
}

void ButtonTheme::reconfigTheme() {
    m_gc.setForeground(*m_pic_color);
}



