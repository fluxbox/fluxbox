// GContext.cc for FbTk - fluxbox toolkit
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

// $Id: GContext.cc,v 1.3 2003/09/11 19:57:38 fluxgen Exp $

#include "GContext.hh"

#include "App.hh"
#include "FbDrawable.hh"
#include "FbPixmap.hh"
#include "Color.hh"
#include "Font.hh"

namespace FbTk {

GContext::GContext(const FbTk::FbDrawable &drawable): 
    m_gc(XCreateGC(FbTk::App::instance()->display(),
                   drawable.drawable(),
                   0, 0)) {
    setGraphicsExposure(false);
}

GContext::GContext(Drawable drawable):
    m_gc(XCreateGC(FbTk::App::instance()->display(),
                   drawable,
                   0, 0)) {
    setGraphicsExposure(false);
}

GContext::~GContext() {
    if (m_gc)
        XFreeGC(FbTk::App::instance()->display(), m_gc);
}

void GContext::setForeground(const FbTk::Color &color) {
    setForeground(color.pixel());
}

void GContext::setForeground(long pixel_value) {
    XSetForeground(FbTk::App::instance()->display(), m_gc,
                   pixel_value);
}

void GContext::setBackground(const FbTk::Color &color) {
    setBackground(color.pixel());
}

void GContext::setBackground(long pixel_value) {
    XSetBackground(FbTk::App::instance()->display(), m_gc,
                   pixel_value);
}

/// not implemented!
void GContext::setFont(const FbTk::Font &font) {
    //!! TODO
}

void GContext::setFont(int fid) {
    XSetFont(FbTk::App::instance()->display(), m_gc, fid);
}
void GContext::setClipMask(const FbTk::FbPixmap &mask) {
    XSetClipMask(FbTk::App::instance()->display(), m_gc,
                 mask.drawable());
}

void GContext::setClipOrigin(int x, int y) {
    XSetClipOrigin(FbTk::App::instance()->display(), m_gc,
                   x, y);
}

void GContext::setGraphicsExposure(bool flag) {
    XSetGraphicsExposures(FbTk::App::instance()->display(), m_gc, 
                          flag);
}

void GContext::setFunction(int func) {
    XSetFunction(FbTk::App::instance()->display(), m_gc,
                 func);
}

void GContext::setSubwindowMode(int mode) {
    XSetSubwindowMode(FbTk::App::instance()->display(), m_gc,
                      mode);
}


} // end namespace FbTk
