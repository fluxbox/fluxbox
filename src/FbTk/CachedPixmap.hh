// CachedPixmap.hh
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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


#ifndef FBTK_CACHED_PIXMAP_H
#define FBTK_CACHED_PIXMAP_H

#include <X11/Xlib.h>

namespace FbTk {

class ImageControl;

/// holds cached pixmap and releases it from cache when it dies
class CachedPixmap {
public:
    /// @param ctrl the image cache control
    explicit CachedPixmap( FbTk::ImageControl& ctrl );
    /**
     * @param ctrl cache control
     * @param pm pixmap to store
     */
    CachedPixmap( FbTk::ImageControl& ctrl,
                  Pixmap pm );
    ~CachedPixmap();

    operator Pixmap() const { 
        return m_pixmap;
    }

    /**
     * Sets new pixmap and releases the old pixmap from cache
     * @param pm the new pixmap to set
     */
    void reset( Pixmap pm );

    /// @return pixmap
    Pixmap operator *() const {
        return m_pixmap;
    }

public:
    /// releases pixmap from cache
    void destroy();

    Pixmap m_pixmap; //< cached pixmap
    FbTk::ImageControl &m_ctrl; //< cache control
};

} // namespace CachedPixmap

#endif // FBTK_CACHED_PIXMAP
