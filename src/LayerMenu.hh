// LayerMenu.hh for Fluxbox - fluxbox toolkit
// Copyright (c) 2003-2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                     and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id$

#ifndef LAYERMENU_HH
#define LAYERMENU_HH

#include "MenuItem.hh"
#include "ToggleMenu.hh"
#include "RefCount.hh"
#include "SimpleCommand.hh"
#include "I18n.hh"

#include "fluxbox.hh"

// provides a generic way for giving an object a layer menu

/// this class holds the layermenu items
template <typename ItemType> 
class LayerMenuItem : public FbTk::MenuItem {
public:
    LayerMenuItem(const char *label, ItemType *object, int layernum,
                  FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label,cmd), m_object(object), m_layernum(layernum) {}
    LayerMenuItem(const char *label, ItemType *object, int layernum):
        FbTk::MenuItem(label), m_object(object), m_layernum(layernum) {}

    bool isEnabled() const { return m_object->layerItem().getLayerNum() != m_layernum; }
    void click(int button, int time) {
        m_object->moveToLayer(m_layernum);
        FbTk::MenuItem::click(button, time);
    }
    
private:
    ItemType *m_object;
    int m_layernum;
};


/// Create a layer menu inside from the given menu 
template <typename ItemType>
class LayerMenu : public ToggleMenu {
public:
    LayerMenu(MenuTheme &tm, FbTk::ImageControl &imgctrl,
              FbTk::XLayer &layer, ItemType *item, bool save_rc);


private:
    ItemType *m_object;
};


template <typename ItemType>
LayerMenu<ItemType>::LayerMenu(MenuTheme &tm, FbTk::ImageControl &imgctrl,
                               FbTk::XLayer &layer, ItemType *item, bool save_rc):
    ToggleMenu(tm, imgctrl, layer), 
    m_object(item) 
{
    _FB_USES_NLS;

    Fluxbox *fluxbox = Fluxbox::instance();
    
    struct {
        int set;
        int base;
        const char *default_str;
        int layernum;
    } layer_menuitems[]  = {
        //TODO: nls
        {0, 0, _FBTEXT(Layer, AboveDock, "Above Dock", "Layer above dock"), fluxbox->getAboveDockLayer()},
        {0, 0, _FBTEXT(Layer, Dock, "Dock", "Layer dock"), fluxbox->getDockLayer()},
        {0, 0, _FBTEXT(Layer, Top, "Top", "Layer top"), fluxbox->getTopLayer()},
        {0, 0, _FBTEXT(Layer, Normal, "Normal", "Layer normal"), fluxbox->getNormalLayer()},
        {0, 0, _FBTEXT(Layer, Bottom, "Bottom", "Layer bottom"), fluxbox->getBottomLayer()},
        {0, 0, _FBTEXT(Layer, Desktop, "Desktop", "Layer desktop"), fluxbox->getDesktopLayer()},
    };
    
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(
                                             *Fluxbox::instance(), 
                                             &Fluxbox::save_rc));

    for (size_t i=0; i < 6; ++i) {
        // TODO: fetch nls string
        if (save_rc) {    
            insert(new LayerMenuItem<ItemType>(layer_menuitems[i].default_str, 
                                               m_object, layer_menuitems[i].layernum, saverc_cmd));
        } else {
            insert(new LayerMenuItem<ItemType>(layer_menuitems[i].default_str, 
                                               m_object, layer_menuitems[i].layernum));               
        }
    }
    updateMenu();
}

#endif // LAYERMENU_HH
