// IconbarTool.hh
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef ICONBARTOOL_HH
#define ICONBARTOOL_HH

#include "ToolbarItem.hh"
#include "FbMenu.hh"

#include "FbTk/Container.hh"
#include "FbTk/CachedPixmap.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Timer.hh"

#include <map>

class IconbarTheme;
class BScreen;
class IconButton;
class Focusable;
class FocusableList;

class IconbarTool: public ToolbarItem {
public:
    typedef std::map<Focusable *, IconButton *> IconMap;

    IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme,
                FbTk::ThemeProxy<IconbarTheme> &focused_theme,
                FbTk::ThemeProxy<IconbarTheme> &unfocused_theme,
                BScreen &screen, FbTk::Menu &menu);
    ~IconbarTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void show();
    void hide();
    void setAlignment(FbTk::Container::Alignment a);
    void setMode(std::string mode);
    void parentMoved() { m_icon_container.parentMoved(); }

    unsigned int width() const;
    unsigned int preferredWidth() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    std::string mode() const { return *m_rc_mode; }

    void setOrientation(FbTk::Orientation orient);
    FbTk::Container::Alignment alignment() const { return m_icon_container.alignment(); }
    static std::string &iconifiedPrefix() { return s_iconifiedDecoration[0]; }
    static std::string &iconifiedSuffix() { return s_iconifiedDecoration[1]; }

    const BScreen &screen() const { return m_screen; }
private:
    enum UpdateReason { LIST_ORDER, LIST_ADD, LIST_REMOVE, LIST_RESET, ALIGN };

    void updateSizing();

    /// render single button, and probably apply changes (clear)
    /// @param button the button to render
    /// @param clear if the window should be cleared first
    void renderButton(IconButton &button, bool clear = true);
    /// render all buttons
    void renderTheme();
    void renderTheme(int alpha);
    /// destroy all icons
    void deleteIcons();
    /// add or move a single window
    void insertWindow(Focusable &win, int pos = -2);
    /// remove a single window
    void removeWindow(Focusable &win);
    /// make a button for the window
    IconButton *makeButton(Focusable &win);
    /// remove all windows and add again
    void reset();
    /// add icons to the list
    void updateList();

    void updateMaxSizes(unsigned int width, unsigned int height);
    /// called when the list emits a signal
    void update(UpdateReason reason, Focusable *win);

    void updateIconifiedPattern();

    void themeReconfigured();

    FbTk::Timer m_resizeSig_timer;
    void emitResizeSig();

    BScreen &m_screen;
    FbTk::Container m_icon_container;
    IconbarTheme &m_theme;
    FbTk::ThemeProxy<IconbarTheme> &m_focused_theme, &m_unfocused_theme;
    FbTk::CachedPixmap m_empty_pm; ///< pixmap for empty container

    FbTk::SignalTracker m_tracker;

    std::unique_ptr<FocusableList> m_winlist;
    IconMap m_icons;
    std::string m_mode;
    FbTk::Resource<std::string> m_rc_mode;
    FbTk::Resource<FbTk::Container::Alignment> m_rc_alignment; ///< alignment of buttons
    FbTk::Resource<int> m_rc_client_width; ///< size of client button in LEFT/RIGHT mode
    FbTk::Resource<unsigned int> m_rc_client_padding; ///< padding of the text
    FbTk::Resource<bool> m_rc_use_pixmap; ///< if iconbar should use win pixmap or not
    FbMenu m_menu;
    int m_alpha;
    static std::string s_iconifiedDecoration[2];
};

#endif // ICONBARTOOL_HH
