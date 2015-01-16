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

#include <list>

namespace FbTk {
class AutoReloadHelper;
class Menu;
}

class FbMenu;
class FluxboxWindow;
class BScreen;

namespace MenuCreator {

    FbMenu* createMenu(const std::string& label, BScreen& screen);
    FbMenu* createMenu(const std::string& label, int screen_num);
    FbMenu* createMenuType(const std::string &label, int screen_num);
    bool createFromFile(const std::string &filename,
                        FbTk::Menu &inject_into,
                        FbTk::AutoReloadHelper *reloader = NULL,
                        bool begin = true);
    bool createWindowMenuItem(const std::string &type, const std::string &label, 
                                     FbTk::Menu &inject_into);
};

#endif // MENUCREATOR_HH
