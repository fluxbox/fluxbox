// LayerMenu.hh for Fluxbox - fluxbox toolkit
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

// $Id: LayerMenu.hh,v 1.2 2003/04/16 13:43:41 rathnor Exp $

#ifndef LAYERMENU_HH
#define LAYERMENU_HH

#include "MenuItem.hh"
#include "FbMenu.hh"
#include "FbCommands.hh"
#include "RefCount.hh"
#include "SimpleCommand.hh"

class Fluxbox;

// provides a generic way for giving an object a layer menu

// this class holds the layermenu items
template <typename ItemType> 
class LayerMenuItem : public FbTk::MenuItem {
public:
    LayerMenuItem(const char *label, ItemType *object, int layernum,
                  FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label,cmd), m_object(object), m_layernum(layernum) {}
    LayerMenuItem(const char *label, ItemType *object, int layernum):
        FbTk::MenuItem(label), m_object(object), m_layernum(layernum) {}

    bool isEnabled() const { return m_object->getLayerItem().getLayerNum() != m_layernum; } ;
    void click(int button, int time) {
        m_object->moveToLayer(m_layernum);
        FbTk::MenuItem::click(button, time);
    }
    
private:
    ItemType *m_object;
    int m_layernum;
};


/* create a layer menu inside from the given menu */
template <typename ItemType>
class LayerMenu : public FbMenu {
public:
    LayerMenu(FbTk::MenuTheme &tm, int screen_num, FbTk::ImageControl &imgctrl,
              FbTk::XLayer &layer, ItemType *item, bool save_rc);

private:
    ItemType *m_object;
};


template <typename ItemType>
LayerMenu<ItemType>::LayerMenu(FbTk::MenuTheme &tm, int screen_num, FbTk::ImageControl &imgctrl,
                               FbTk::XLayer &layer, ItemType *item, bool save_rc):
    FbMenu(tm, screen_num, imgctrl, layer), 
    m_object(item) 
{
    
    Fluxbox *fluxbox = Fluxbox::instance();
    
    struct {
        int set;
        int base;
        const char *default_str;
                int layernum;
    } layer_menuitems[]  = {
        //TODO: nls
        {0, 0, "Above Dock", fluxbox->getAboveDockLayer()},
        {0, 0, "Dock", fluxbox->getDockLayer()},
        {0, 0, "Top", fluxbox->getTopLayer()},
        {0, 0, "Normal", fluxbox->getNormalLayer()},
        {0, 0, "Bottom", fluxbox->getBottomLayer()},
        {0, 0, "Desktop", fluxbox->getDesktopLayer()},
    };
    
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbTk::SimpleCommand<Fluxbox>(
                                     *Fluxbox::instance(), 
                                     &Fluxbox::save_rc));

    for (size_t i=0; i < 6; ++i) {
        // TODO: fetch nls string
        if (save_rc) {    
            insert(new LayerMenuItem<ItemType>(
                       layer_menuitems[i].default_str, 
                       m_object, layer_menuitems[i].layernum, saverc_cmd));
        } else {
            insert(new LayerMenuItem<ItemType>(
                       layer_menuitems[i].default_str, 
                       m_object, layer_menuitems[i].layernum));               
        }
    }
    update();
}

#endif // LAYERMENU_HH
