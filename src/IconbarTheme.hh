#ifndef ICONBARTHEME_HH
#define ICONBARTHEME_HH

#include "FbTk/Theme.hh"
#include "FbTk/Texture.hh"
#include "TextTheme.hh"

class IconbarTheme:public FbTk::Theme {
public:
    IconbarTheme(int screen_num, const std::string &name, const std::string &altname);
    virtual ~IconbarTheme();

    void reconfigTheme();
    const TextTheme &focusedText() const { return m_focused_text; }
    const TextTheme &unfocusedText() const { return m_unfocused_text; }

    const FbTk::Texture &focusedTexture() const { return *m_focused_texture; }
    const FbTk::Texture &unfocusedTexture() const { return *m_unfocused_texture; }
private:
    FbTk::ThemeItem<FbTk::Texture> m_focused_texture, m_unfocused_texture;
    TextTheme m_focused_text, m_unfocused_text;
};

#endif  // ICONBARTHEME_HH
