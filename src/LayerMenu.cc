#include "LayerMenu.hh"

#include "FbCommands.hh"
#include "Layer.hh"

#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/I18n.hh"

LayerMenu::LayerMenu(MenuTheme &tm, FbTk::ImageControl &imgctrl,
                     FbTk::XLayer &layer, LayerObject *object, bool save_rc):
    ToggleMenu(tm, imgctrl, layer) {
    _FB_USES_NLS;

    
    struct {
        int set;
        int base;
        const char *default_str;
        int layernum;
    } layer_menuitems[]  = {
        //TODO: nls
        {0, 0, _FBTEXT(Layer, AboveDock, "Above Dock", "Layer above dock"), Layer::ABOVE_DOCK},
        {0, 0, _FBTEXT(Layer, Dock, "Dock", "Layer dock"), Layer::DOCK},
        {0, 0, _FBTEXT(Layer, Top, "Top", "Layer top"), Layer::TOP},
        {0, 0, _FBTEXT(Layer, Normal, "Normal", "Layer normal"), Layer::NORMAL},
        {0, 0, _FBTEXT(Layer, Bottom, "Bottom", "Layer bottom"), Layer::BOTTOM},
        {0, 0, _FBTEXT(Layer, Desktop, "Desktop", "Layer desktop"), Layer::DESKTOP},
    };
    
    FbTk::RefCount<FbTk::Command> saverc_cmd(new FbCommands::SaveResources());

    for (size_t i=0; i < 6; ++i) {
        // TODO: fetch nls string
        if (save_rc) {    
            insert(new LayerMenuItem(layer_menuitems[i].default_str, 
                                     object, layer_menuitems[i].layernum, saverc_cmd));
        } else {
            insert(new LayerMenuItem(layer_menuitems[i].default_str, 
                                     object, layer_menuitems[i].layernum));               
        }
    }
    updateMenu();
}
