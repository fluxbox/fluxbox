// ImageImlib2.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2004 Mathias Gumz <akira at fluxbox dot org>
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

// $Id:  $

#include "ImageImlib2.hh"

#include "App.hh"
#include "PixmapWithMask.hh"

#include <X11/xpm.h>

#include <Imlib2.h>
#include <map>

namespace {

typedef std::map<int, Imlib_Context> ScreenImlibContextContainer;
typedef ScreenImlibContextContainer::iterator ScreenImlibContext;

ScreenImlibContextContainer contexts;

}; // anon namespace


namespace FbTk {

ImageImlib2::ImageImlib2() {

    // lets have a 2mb cache inside imlib, holds
    // uncompressed images
    imlib_set_cache_size(2048 * 1024);

    // TODO: this are the potential candidates,
    //       choose only sane ones. open for discussion
    char* format_list[] = {
        "PNG",                               // pngloader
        "JPEG", "JPG", "JFI", "JFIF",        // jpegloader
//        "TIFF", "TIF",                       // tiffloader
        "PNM", "PPM", "PGM", "PBM", "PAM",   // pnmloader
//        "TGA",                               // tgaloader
//        "IFF", "ILBM", "LBM",                // lbmloader
//        "GIF",                               // gifloader
//        "ARGB", "AGR",                       // argbloader
//        "BMP",                               // bmploader
//        "BZ2",                               // bzloader
//        "GZ",                                // gzloader
        NULL
    };

    char** format = NULL;
    for(format = format_list; *format != NULL; format++) {
        Image::registerType(*format, *this);
    }
}

ImageImlib2::~ImageImlib2() {
    ScreenImlibContext it = contexts.begin();
    ScreenImlibContext it_end = contexts.end();
    for (; it != it_end; it++) {
        imlib_context_free(it->second);
    }
    contexts.clear();
}

PixmapWithMask *ImageImlib2::load(const std::string &filename, int screen_num) const {

    Display *dpy = FbTk::App::instance()->display();
    
    // init imlib2 if needed, the settings for each screen may differ
    ScreenImlibContext screen_context = contexts.find(screen_num);
    if (screen_context == contexts.end()) {

        Imlib_Context new_context = imlib_context_new();
        imlib_context_push(new_context);

        imlib_context_set_display(dpy);
        imlib_context_set_drawable(RootWindow(dpy, screen_num));
        imlib_context_set_colormap(DefaultColormap(dpy, screen_num));
        imlib_context_set_visual(DefaultVisual(dpy, screen_num));

        imlib_context_pop();

        contexts[screen_num] = new_context;
        screen_context = contexts.find(screen_num);
    }

    if (screen_context == contexts.end()) {
#ifdef DEBUG
        cerr << "ImageImlib2::load: error, couldnt find a valid Imlib_Context.\n";
#endif // DEBUG
        return 0;
    }

    // now load the stuff
    Imlib_Context context = screen_context->second;
    imlib_context_push(context);
    Imlib_Image image = imlib_load_image_immediately(filename.c_str());
    if (image) { // loading was ok
        imlib_context_set_image(image);

        Pixmap pm = 0, mask = 0;
        imlib_render_pixmaps_for_whole_image(&pm, &mask);
        
        // pm and mask belong to imlib, so we have to copy them
        FbPixmap fbpm;
        FbPixmap fbmask;

        fbpm.copy(pm);
        fbmask.copy(mask);

        // mark pm and mask as freeable in imlib
        imlib_free_image();
        imlib_free_pixmap_and_mask(pm);

        imlib_context_pop();

        PixmapWithMask* result = new PixmapWithMask();
        result->pixmap() = fbpm;
        result->mask() = fbmask;
        return result;
    }

    // loading failure
    imlib_context_pop();
    return 0;
}

} // end namespace FbTk
