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

#ifndef LAYERMENU_HH
#define LAYERMENU_HH


#include "ToggleMenu.hh"

#include "FbTk/RadioMenuItem.hh"

class LayerObject {
public:
    virtual void moveToLayer(int layer_number) = 0;
    virtual int layerNumber() const = 0;
    virtual ~LayerObject() { }
};


/// this class holds the layermenu items
class LayerMenuItem : public FbTk::RadioMenuItem {
public:
    LayerMenuItem(const FbTk::FbString &label, LayerObject *object,
                  int layernum, FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd), m_object(object), m_layernum(layernum) {}

    LayerMenuItem(const FbTk::FbString &label, LayerObject *object,
                  int layernum):
        FbTk::RadioMenuItem(label), m_object(object), m_layernum(layernum) {}

    bool isSelected() const { return m_object->layerNumber() == m_layernum; }
    void click(int button, int time, unsigned int mods) {
        m_object->moveToLayer(m_layernum);
        FbTk::RadioMenuItem::click(button, time, mods);
    }
    
private:
    LayerObject *m_object;
    int m_layernum;
};


/// Create a layer menu inside from the given menu 
class LayerMenu : public ToggleMenu {
public:
    LayerMenu(FbTk::ThemeProxy<FbTk::MenuTheme> &tm,
              FbTk::ImageControl &imgctrl,
              FbTk::Layer &layer, LayerObject *item, bool save_rc);
    void show();
};

#endif // LAYERMENU_HH
