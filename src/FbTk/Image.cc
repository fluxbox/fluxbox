// Image.cc for FbTk - Fluxbox ToolKit
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Image.cc,v 1.4 2004/01/02 22:01:08 fluxgen Exp $

#include "Image.hh"
#include "StringUtil.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XPM
#include "ImageXPM.hh"
#endif /// HAVE_XPM

#include <list>
#include <iostream>
using namespace std;

namespace FbTk {

Image::ImageMap Image::s_image_map;
Image::StringList Image::s_search_paths;

PixmapWithMask *Image::load(const std::string &filename, int screen_num) {

#ifdef HAVE_XPM
    // we must do this because static linkage with libFbTk will not init 
    // a static autoreg variable for it
    static ImageXPM xpm;
#endif // HAVE_XPM

    if (filename == "")
        return false;

    // determine file ending
    std::string extension(StringUtil::toUpper(StringUtil::findExtension(filename)));

    // valid handle?
    if (s_image_map[extension] == 0)
        return false;
    
    // load file
    PixmapWithMask *pm = s_image_map[extension]->load(filename, screen_num);
    // failed?, try different search paths
    if (pm == 0 && s_search_paths.size()) {
        // first we need to get basename of current filename
        std::string base_filename = StringUtil::basename(filename);
        std::string path = "";
        // append each search path and try to load
        StringList::iterator it = s_search_paths.begin();
        StringList::iterator it_end = s_search_paths.end();
        for (; it != it_end && pm == 0; ++it) {
            // append search path and try load it
            path = StringUtil::expandFilename(*it);
            pm = s_image_map[extension]->load(path + "/" + base_filename, screen_num);
        }

    }

    return pm;
}

bool Image::registerType(const std::string &type, ImageBase &base) {

    string ucase_type = StringUtil::toUpper(type);

    // not empty and not this base?
    if (s_image_map[ucase_type] != 0 &&
        s_image_map[ucase_type] != &base) 
        return false;
    // already registered?
    if (s_image_map[ucase_type] == &base)
        return true;

    s_image_map[ucase_type] = &base;

    return true;
}

void Image::remove(ImageBase &base) {
    // find and remove all referenses to base
    ImageMap::iterator it = s_image_map.begin();
    ImageMap::iterator it_end = s_image_map.end();
    std::list<std::string> remove_list;
    for (; it != it_end; ++it) {
        if (it->second == &base)
            remove_list.push_back(it->first);
    }

    while (!remove_list.empty()) {
        s_image_map.erase(remove_list.back());
        remove_list.pop_back();
    }
}

void Image::addSearchPath(const std::string &search_path) {
    s_search_paths.push_back(search_path);
}

void Image::removeSearchPath(const std::string &search_path) {
    s_search_paths.remove(search_path);
}

void Image::removeAllSearchPaths() {
    s_search_paths.clear();
}

}; // end namespace FbTk
