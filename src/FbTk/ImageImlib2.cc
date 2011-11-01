// ImageImlib2.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2004 - 2006 Mathias Gumz <akira at fluxbox dot org>
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

#include "ImageImlib2.hh"

#include "App.hh"
#include "PixmapWithMask.hh"

#include <Imlib2.h>
#include <map>
#include <iostream>

namespace {

class ScreenImlibContextContainer : public std::map<int, Imlib_Context> {
public:
    ~ScreenImlibContextContainer() {

        std::map<int, Imlib_Context>::iterator it = this->begin();
        std::map<int, Imlib_Context>::iterator it_end = this->end();
        for (; it != it_end; ++it) {
            imlib_context_free(it->second);
        }

        imlib_flush_loaders();
    }
};
typedef ScreenImlibContextContainer::iterator ScreenImlibContext;

ScreenImlibContextContainer contexts;
} // anon namespace


namespace FbTk {

ImageImlib2::ImageImlib2() {

    // TODO: this are the potential candidates,
    //       choose only sane ones. open for discussion
    static const char* format_list[] = {
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

    const char** format = NULL;
    for(format = format_list; *format != NULL; format++) {
        Image::registerType(*format, *this);
    }
}

PixmapWithMask *ImageImlib2::load(const std::string &filename, int screen_num) const {

    Display *dpy = FbTk::App::instance()->display();
    
    // init imlib2 if needed, the settings for each screen may differ
    ScreenImlibContext screen_context = contexts.find(screen_num);
    if (screen_context == contexts.end()) {

        Imlib_Context new_context = imlib_context_new();
        imlib_context_push(new_context);

        imlib_context_set_display(dpy);
        imlib_context_set_visual(DefaultVisual(dpy, screen_num));
        imlib_context_set_colormap(DefaultColormap(dpy, screen_num));
        imlib_context_set_drawable(RootWindow(dpy, screen_num));

        // lets have a 2mb cache inside imlib, holds
        // uncompressed images
        imlib_set_cache_size(2048 * 1024);

        imlib_context_pop();

        contexts[screen_num] = new_context;
        screen_context = contexts.find(screen_num);
    }
        

    if (screen_context == contexts.end()) {
#ifdef DEBUG
        std::cerr << "ImageImlib2::load: error, couldnt find a valid Imlib_Context.\n";
#endif // DEBUG
        return 0;
    }

    // now load the stuff
    Imlib_Context context = screen_context->second;
    imlib_context_push(context); 

    Imlib_Image image = 0;
    Imlib_Load_Error err;

    // TODO: contact imlib2-authors:
    //
    // we use this error-loading + get_data_for_reading_only because
    // it doesnt memleak imlib2,
    // 
    //     imlib_load_image_immediately
    //     imlib_load_image_without_cache
    //     imlib_load_image_immediately_without_cache
    // 
    // seem to memleak or trouble imlib2 when something fails. the
    // imlib_load_image_with_error_return checks for existence etc before
    // actually doing anything, i ll analyze this further (mathias)
    image = imlib_load_image_with_error_return(filename.c_str(), &err);
    if (image) { // loading was ok
        
        imlib_context_set_image(image);
        Pixmap pm = 0, mask = 0;

        // force loading/creation of the image
        imlib_image_get_data_for_reading_only();

        // and render it to the pixmaps
        imlib_render_pixmaps_for_whole_image(&pm, &mask);
        
        // pm and mask belong to imlib2, 
        // so we have to copy them
        PixmapWithMask* result = new PixmapWithMask();
        result->pixmap().copy(pm, 0, 0);
        result->mask().copy(mask, 0, 0);

        // mark pm and mask as freeable in imlib
        imlib_free_image_and_decache();
        imlib_free_pixmap_and_mask(pm);

        imlib_context_pop();

        return result;
    } else // loading failure
        imlib_context_pop();

    return 0;
}

} // end namespace FbTk
