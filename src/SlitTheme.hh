// SlitTheme.hh for fluxbox
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: SlitTheme.hh,v 1.1 2003/06/20 01:25:26 fluxgen Exp $

#ifndef SLITTHEME_HH
#define SLITTHEME_HH

#include "Theme.hh"
#include "Slit.hh"

class SlitTheme:public FbTk::Theme {
public:
    explicit SlitTheme(Slit &slit):FbTk::Theme(slit.screen().screenNumber()), 
                          m_slit(slit),
                          m_texture(*this, "slit", "Slit") { 
        // default texture type
        m_texture->setType(FbTk::Texture::SOLID);
    }
    void reconfigTheme() {
        m_slit.reconfigure();
    }
    const FbTk::Texture &texture() const { return *m_texture; }
private:
    Slit &m_slit;
    FbTk::ThemeItem<FbTk::Texture> m_texture;
};

#endif // SLITTHEME_HH
