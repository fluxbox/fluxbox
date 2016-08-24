// LayerMenu.cc
// Copyright (c) 2005 - 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

#include "LayerMenu.hh"

#include "FbCommands.hh"
#include "Layer.hh"

#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/I18n.hh"

LayerMenu::LayerMenu(FbTk::ThemeProxy<FbTk::MenuTheme> &tm,
                     FbTk::ImageControl &imgctrl,
                     FbTk::Layer &layer, LayerObject *object, bool save_rc):
    ToggleMenu(tm, imgctrl, layer) {
    _FB_USES_NLS;


    struct {
        int set;
        int base;
        FbTk::FbString default_str;
        int layernum;
    } layer_menuitems[]  = {
        //TODO: nls
        {0, 0, _FB_XTEXT(Layer, AboveDock, "Above Dock", "Layer above dock"), ResourceLayer::ABOVE_DOCK},
        {0, 0, _FB_XTEXT(Layer, Dock, "Dock", "Layer dock"), ResourceLayer::DOCK},
        {0, 0, _FB_XTEXT(Layer, Top, "Top", "Layer top"), ResourceLayer::TOP},
        {0, 0, _FB_XTEXT(Layer, Normal, "Normal", "Layer normal"), ResourceLayer::NORMAL},
        {0, 0, _FB_XTEXT(Layer, Bottom, "Bottom", "Layer bottom"), ResourceLayer::BOTTOM},
        {0, 0, _FB_XTEXT(Layer, Desktop, "Desktop", "Layer desktop"), ResourceLayer::DESKTOP},
    };

    FbTk::RefCount<FbTk::Command<void> > saverc_cmd(new FbCommands::SaveResources());

    for (size_t i=0; i < 6; ++i) {
        // TODO: fetch nls string
        if (save_rc) {
            insertItem(new LayerMenuItem(layer_menuitems[i].default_str,
                                     object, layer_menuitems[i].layernum, saverc_cmd));
        } else {
            insertItem(new LayerMenuItem(layer_menuitems[i].default_str,
                                     object, layer_menuitems[i].layernum));
        }
    }
    updateMenu();
}

// update which items appear disabled whenever we show the menu
void LayerMenu::show() {
    frameWindow().updateBackground(false);
    clearWindow();
    FbTk::Menu::show();
}
