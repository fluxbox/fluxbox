// WorkspaceNameTheme.hh
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: WorkspaceNameTheme.hh,v 1.1 2003/08/29 00:51:55 fluxgen Exp $

#ifndef WORKSPACENAMETHEME_HH
#define WORKSPACENAMETHEME_HH

#include "ToolTheme.hh"
#include "FbTk/Theme.hh"

class WorkspaceNameTheme:public ToolTheme {
public:
    WorkspaceNameTheme(int screen_num, 
                       const std::string &name, 
                       const std::string &alt_name):ToolTheme(screen_num, name, alt_name) { }

    bool fallback(FbTk::ThemeItem_base &item) {
        if (item.name() == "toolbar.workspace.textColor") {
            return FbTk::ThemeManager::instance().loadItem(item, 
                                                           "toolbar.label.textColor", 
                                                           "Toolbar.Label.TextColor");
        } else if (item.name() == "toolbar.workspace") {

            // special case for textures since they're using .load()
            FbTk::ThemeItem<FbTk::Texture> tmp_item(*this,
                                                    "toolbar.label", "Toolbar.Label");
            tmp_item.load();
            // copy texture
            *textureTheme() = *tmp_item;
            return true;
        }

        return ToolTheme::fallback(item);
    }
};

#endif // WORKSPACENAMETHEME_HH
