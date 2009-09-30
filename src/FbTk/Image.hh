// Image.hh for FbTk - Fluxbox ToolKit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef FBTK_IMAGE_HH
#define FBTK_IMAGE_HH

#include <string>
#include <list>
#include <map>

namespace FbTk {

class ImageBase;
class PixmapWithMask;

/// loads images
namespace Image {

    /// @return an instance of PixmapWithMask on success, 0 on failure
    PixmapWithMask *load(const std::string &filename, int screen_num);
    /// for register file type and imagebase
    /// @return false on failure
    bool registerType(const std::string &type, ImageBase &base);
    /// removes a imagebase class from register
    /// @return false on failure
    void remove(ImageBase &base);
    /// adds a path to search images from
    void addSearchPath(const std::string &search_path);
    /// removes a path to search images from
    void removeSearchPath(const std::string &search_path);
    /// adds a path to search images from
    void removeAllSearchPaths();
    /// locates an image in the search path
    std::string locateFile(const std::string &filename);
}

/// common interface for all image classes
class ImageBase {
public:
    virtual ~ImageBase() { Image::remove(*this); }
    virtual PixmapWithMask *load(const std::string &name, int screen_num) const = 0;
};

} // end namespace FbTk

#endif // IMAGE_HH

