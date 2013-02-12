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

#include "Image.hh"
#include "StringUtil.hh"
#include "FileUtil.hh"

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


namespace {

typedef std::map<std::string, FbTk::ImageBase *> ImageMap;
typedef std::list<std::string> StringList;

ImageMap s_image_map;
StringList s_search_paths;

#ifdef HAVE_IMLIB2
FbTk::ImageImlib2 imlib2_loader;
#endif
#ifdef HAVE_XPM
FbTk::ImageXPM xpm_loader;
#endif


} // end of anonymous namespace

namespace FbTk {

PixmapWithMask *Image::load(const string &filename, int screen_num) {


    if (filename.empty())
        return NULL;

    // determine file ending
    string extension(StringUtil::toUpper(StringUtil::findExtension(filename)));

    // valid handle?
    if (s_image_map.find(extension) == s_image_map.end())
        return NULL;

    string path = locateFile(filename);
    if (!path.empty())
        return s_image_map[extension]->load(path, screen_num);

    return 0;
}

string Image::locateFile(const string &filename) {
    string path = StringUtil::expandFilename(filename);
    if (FileUtil::isRegularFile(path.c_str()))
        return path;
    string base = StringUtil::basename(filename);
    StringList::iterator it = s_search_paths.begin();
    StringList::iterator it_end = s_search_paths.end();
    for (; it != it_end; ++it) {
        path = StringUtil::expandFilename(*it) + "/" + base;
        if (FileUtil::isRegularFile(path.c_str()))
            return path;
    }
    return "";
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

} // end namespace FbTk
