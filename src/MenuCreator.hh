// MenuCreator.hh for Fluxbox
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#ifndef MENUCREATOR_HH
#define MENUCREATOR_HH

#include "FbTk/FbString.hh"

#include <string>
#include <list>

namespace FbTk {
class Menu;
}

class FluxboxWindow;

class MenuCreator {
public:
    static FbTk::Menu *createMenu(const std::string &label, int screen_num);
    static FbTk::Menu *createFromFile(const std::string &filename, int screen_num, 
                                      bool require_begin);
    static FbTk::Menu *createMenuType(const std::string &label, int screen_num);
    static bool createFromFile(const std::string &filename, FbTk::Menu &inject_into,
                               bool require_begin);
    static bool createWindowMenuFromFile(const std::string &filename, FbTk::Menu &inject_into, 
                                         bool require_begin);
    static bool createWindowMenuItem(const std::string &type, const std::string &label, 
                                     FbTk::Menu &inject_into);

    /**
     * Encoding-related helpers (encoding, aka codeset)
     */

    // Files are guaranteed to be "balanced", unlike user-created [encoding] tags.
    static void startFile();
    static void endFile();

    static void startEncoding(const std::string &encoding);
    static void endEncoding();

private:
    // stack of encodings
    static std::list<std::string> encoding_stack;
    // stack of ints, representing stack size as each file is entered
    // (a file should never end more encodings than it starts)
    static std::list<size_t> stacksize_stack;

    static FbTk::StringConvertor m_stringconvertor;

};

#endif // MENUCREATOR_HH
