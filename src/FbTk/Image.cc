// Image.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "Image.hh"
#include "StringUtil.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_XPM
#include "ImageXPM.hh"
#endif // HAVE_XPM

#ifdef HAVE_IMLIB2
#include "ImageImlib2.hh"
#endif // HAVE_IMLIB2

#include <list>
#include <set>

using std::string;
using std::list;
using std::set;

namespace FbTk {

Image::ImageMap Image::s_image_map;
Image::StringList Image::s_search_paths;


void Image::init() {

// create imagehandlers for their extensions
#ifdef HAVE_XPM
    new ImageXPM();
#endif // HAVE_XPM
#ifdef HAVE_IMLIB2
    new ImageImlib2();
#endif // HAVE_IMLIB2
}

void Image::shutdown() {

    set<ImageBase*> handlers;

    // one imagehandler could be registered
    // for more than one type
    ImageMap::iterator it = s_image_map.begin();
    ImageMap::iterator it_end = s_image_map.end();
    for (; it != it_end; it++) {
        if (it->second)
            handlers.insert(it->second);
    }

    // free the unique handlers
    set<ImageBase*>::iterator handler_it = handlers.begin();
    set<ImageBase*>::iterator handler_it_end = handlers.end();
    for(; handler_it != handler_it_end; handler_it++) {
        delete (*handler_it);
    }

    s_image_map.clear();
}

PixmapWithMask *Image::load(const string &filename, int screen_num) {


    if (filename == "")
        return false;

    // determine file ending
    string extension(StringUtil::toUpper(StringUtil::findExtension(filename)));

    // valid handle?
    if (s_image_map.find(extension) == s_image_map.end())
        return false;

    // load file
    PixmapWithMask *pm = s_image_map[extension]->load(filename, screen_num);
    // failed?, try different search paths
    if (pm == 0 && s_search_paths.size()) {
        // first we need to get basename of current filename
        string base_filename = StringUtil::basename(filename);
        string path = "";
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

bool Image::registerType(const string &type, ImageBase &base) {

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
    list<string> remove_list;
    for (; it != it_end; ++it) {
        if (it->second == &base)
            remove_list.push_back(it->first);
    }

    while (!remove_list.empty()) {
        s_image_map.erase(remove_list.back());
        remove_list.pop_back();
    }
}

void Image::addSearchPath(const string &search_path) {
    s_search_paths.push_back(search_path);
}

void Image::removeSearchPath(const string &search_path) {
    s_search_paths.remove(search_path);
}

void Image::removeAllSearchPaths() {
    s_search_paths.clear();
}

}; // end namespace FbTk
