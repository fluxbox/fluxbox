#ifndef BUTTONTHEME_HH
#define BUTTONTHEME_HH

#include "ToolTheme.hh"

#include "FbTk/GContext.hh"

class ButtonTheme: public ToolTheme {
public:
    ButtonTheme(int screen_num, 
                const std::string &name, const std::string &alt_name, 
                const std::string &extra_fallback,
                const std::string &extra_fallback_alt);
    virtual ~ButtonTheme() { }

    bool fallback(FbTk::ThemeItem_base &item);
    void reconfigTheme();

    inline const FbTk::Texture &pressed() const { return *m_pressed_texture; }
    inline GC gc() const { return m_gc.gc(); }
    inline int scale() const { return *m_scale; } // scale factor for inside objects
    inline const std::string &name() { return m_name; }

private:
    FbTk::ThemeItem<FbTk::Color> m_pic_color;
    FbTk::ThemeItem<FbTk::Texture> m_pressed_texture;    
    FbTk::GContext m_gc;
    FbTk::ThemeItem<int> m_scale;
    const std::string m_name;
    const std::string m_fallbackname;
    const std::string m_altfallbackname;
};

#endif // BUTTONTHEME_HH
