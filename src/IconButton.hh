// IconButton.hh
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

#ifndef ICONBUTTON_HH
#define ICONBUTTON_HH

#include "FocusableTheme.hh"

#include "FbTk/CachedPixmap.hh"
#include "FbTk/FbPixmap.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/Timer.hh"
#include "FbTk/Signal.hh"

class IconbarTheme;
class IconbarTool;

namespace FbTk {
template <class T> class ThemeProxy;
}

class IconButton: public FbTk::TextButton {
public:
    IconButton(const FbTk::FbWindow &parent,
               FbTk::ThemeProxy<IconbarTheme> &focused_theme,
               FbTk::ThemeProxy<IconbarTheme> &unfocused_theme,
               Focusable &window, IconbarTool *iconbar);
    virtual ~IconButton();

    void exposeEvent(XExposeEvent &event);
    void enterNotifyEvent(XCrossingEvent &ce);
    void leaveNotifyEvent(XCrossingEvent &ce);
    void buttonPressEvent(XButtonEvent &event);
    void motionNotifyEvent(XMotionEvent &event);
    void buttonReleaseEvent(XButtonEvent &event);
    void clear();
    void clearArea(int x, int y,
                   unsigned int width, unsigned int height,
                   bool exposure = false);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void resize(unsigned int width, unsigned int height);

    void reconfigTheme();

    void setPixmap(bool use);

    Focusable &win() { return m_win; }
    const Focusable &win() const { return m_win; }

    bool setOrientation(FbTk::Orientation orient);

    virtual unsigned int preferredWidth() const;
    void showTooltip();

    const FbTk::Signal<> &titleChanged() { return m_title_changed; }

    static unsigned int updateLaziness() { return 100 * FbTk::FbTime::IN_MILLISECONDS; }

protected:
    void drawText(int x, int y, FbTk::FbDrawable *drawable_override);

    /**
       A drag gesture must not also trigger the click.
       @return false while a drag is in progress
     */
    bool acceptClick(XButtonEvent &event);
private:
    void reconfigAndClear();
    void setupWindow();

    /// Refresh all pixmaps and windows
    /// @param setup Wether to setup window again.
    void refreshEverything(bool setup);
    /// Called when client title changed.
    void clientTitleChanged();

    Focusable &m_win;
    FbTk::FbWindow m_icon_window;
    FbTk::FbPixmap m_icon_pixmap;
    FbTk::FbPixmap m_icon_mask;
    bool m_use_pixmap;
    IconbarTool *m_iconbar; ///< the iconbar this button belongs to
    bool m_dragging; ///< true while a drag gesture is in progress
    unsigned int m_press_button; ///< the button of the last press event
    int m_press_x, m_press_y; ///< root coordinates of the button press
    /// whether or not this instance has the tooltip attention 
    /// i.e if it got enter notify
    bool m_has_tooltip;
    FocusableTheme<IconbarTheme> m_theme;
    // cached pixmaps
    FbTk::CachedPixmap m_pm;
    FbTk::SignalTracker m_signals;
    FbTk::Signal<> m_title_changed;
    FbTk::Timer m_title_update_timer;
};

#endif // ICONBUTTON_HH
