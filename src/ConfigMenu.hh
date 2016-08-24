#ifndef CONFIG_MENU_HH
#define CONFIG_MENU_HH

// ConfigMenu.cc for Fluxbox Window Manager
// Copyright (c) 2015 - Mathias Gumz <akira@fluxbox.org>
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


class BScreen;
struct ScreenResource;

namespace FbTk{
    class Menu;
    class ResourceManager;
}

class ConfigMenu {
public:

    // makes the setup() function-signature shorter
    struct SetupHelper {
        SetupHelper(BScreen& _s, FbTk::ResourceManager& _rm, ScreenResource& _r) :
            screen(_s), rm(_rm), resource(_r) { }
        BScreen& screen;
        FbTk::ResourceManager& rm;
        ScreenResource& resource;
    };

    static void setup(FbTk::Menu& menu, SetupHelper& sh);
};

#endif
