// FocusableTheme.hh
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef FOCUSABLETHEME_HH
#define FOCUSABLETHEME_HH

#include "Focusable.hh"
#include "FbTk/MemFun.hh"
#include "FbTk/Theme.hh"

template <typename BaseTheme>
class FocusableTheme: public FbTk::ThemeProxy<BaseTheme> {
public:
    FocusableTheme(Focusable &win, FbTk::ThemeProxy<BaseTheme> &focused,
                   FbTk::ThemeProxy<BaseTheme> &unfocused):
        m_win(win), m_focused_theme(focused), m_unfocused_theme(unfocused) {

        m_signals.join(m_win.focusSig(),
                FbTk::MemFunIgnoreArgs(m_reconfig_sig, &FbTk::Signal<>::emit));
        m_signals.join(m_win.attentionSig(),
                FbTk::MemFunIgnoreArgs(m_reconfig_sig, &FbTk::Signal<>::emit));
        m_signals.join(m_focused_theme.reconfigSig(),
                FbTk::MemFun(m_reconfig_sig, &FbTk::Signal<>::emit));
        m_signals.join(m_unfocused_theme.reconfigSig(),
                FbTk::MemFun(m_reconfig_sig, &FbTk::Signal<>::emit));
    }

    Focusable &win() { return m_win; }
    const Focusable &win() const { return m_win; }

    FbTk::ThemeProxy<BaseTheme> &focusedTheme() { return m_focused_theme; }
    const FbTk::ThemeProxy<BaseTheme> &focusedTheme() const { return m_focused_theme; }

    FbTk::ThemeProxy<BaseTheme> &unfocusedTheme() { return m_unfocused_theme; }
    const FbTk::ThemeProxy<BaseTheme> &unfocusedTheme() const { return m_unfocused_theme; }

    FbTk::Signal<> &reconfigSig() { return m_reconfig_sig; }

    virtual BaseTheme &operator *() {
        return (m_win.isFocused() || m_win.getAttentionState()) ?
               *m_focused_theme : *m_unfocused_theme;
    }
    virtual const BaseTheme &operator *() const {
        return (m_win.isFocused() || m_win.getAttentionState()) ?
               *m_focused_theme : *m_unfocused_theme;
    }

private:
    Focusable &m_win;
    FbTk::ThemeProxy<BaseTheme> &m_focused_theme, &m_unfocused_theme;
    FbTk::Signal<> m_reconfig_sig;
    FbTk::SignalTracker m_signals;
};

#endif // FOCUSABLETHEME_HH
