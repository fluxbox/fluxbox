#ifndef ICONBARTHEME_HH
#define ICONBARTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/Texture.hh"

#include "TextTheme.hh"
#include "BorderTheme.hh"

class IconbarTheme:public FbTk::Theme {
public:
    IconbarTheme(int screen_num, const std::string &name, const std::string &altname);
    virtual ~IconbarTheme();

    void reconfigTheme();

    void setAntialias(bool antialias);

    const TextTheme &focusedText() const { return m_focused_text; }
    const TextTheme &unfocusedText() const { return m_unfocused_text; }

    const BorderTheme &focusedBorder() const { return m_focused_border; }
    const BorderTheme &unfocusedBorder() const { return m_unfocused_border; }
    const BorderTheme &border() const { return m_border; }
    
    const FbTk::Texture &focusedTexture() const { return *m_focused_texture; }
    const FbTk::Texture &unfocusedTexture() const { return *m_unfocused_texture; }
    const FbTk::Texture &emptyTexture() const { return *m_empty_texture; }

private:
    FbTk::ThemeItem<FbTk::Texture> m_focused_texture, m_unfocused_texture, m_empty_texture;
    BorderTheme m_focused_border, m_unfocused_border, m_border;
    TextTheme m_focused_text, m_unfocused_text;
};

#endif  // ICONBARTHEME_HH
