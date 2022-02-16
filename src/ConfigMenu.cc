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

#include "ConfigMenu.hh"
#include "MenuCreator.hh"
#include "Screen.hh"
#include "fluxbox.hh"

#include "FocusModelMenuItem.hh"
#include "ScreenPlacement.hh"
#include "FbMenu.hh"
#include "ToggleMenu.hh"
#include "FbTk/Menu.hh"
#include "FbTk/BoolMenuItem.hh"
#include "FbTk/IntMenuItem.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/RadioMenuItem.hh"

#include "FbTk/MacroCommand.hh"
#include "FbTk/CommandParser.hh"
#include "FbTk/SimpleCommand.hh"

#include "FbTk/Transparent.hh"
#include "FbTk/Resource.hh"
#include "FbTk/I18n.hh"

namespace {

class TabPlacementMenuItem: public FbTk::RadioMenuItem {
public:
    TabPlacementMenuItem(const FbTk::FbString & label, BScreen &screen,
                         FbWinFrame::TabPlacement place,
                         FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd),
        m_screen(screen),
        m_place(place) {
        setCloseOnClick(false);
    }

    bool isSelected() const { return m_screen.getTabPlacement() == m_place; }
    void click(int button, int time, unsigned int mods) {
        m_screen.saveTabPlacement(m_place);
        FbTk::RadioMenuItem::click(button, time, mods);
    }

private:
    BScreen &m_screen;
    FbWinFrame::TabPlacement m_place;
};


typedef FbTk::RefCount<FbTk::Command<void> > _Cmd;


// NOTE: might also be placed into MenuCreator; for now it ends up here
// because it's just used here
FbMenu *createToggleMenu(const std::string &label, BScreen& screen) {
    FbTk::Layer* layer = screen.layerManager().getLayer(ResourceLayer::MENU);
    FbMenu *menu = new ToggleMenu(screen.menuTheme(), screen.imageControl(), *layer);
    if (!label.empty())
        menu->setLabel(label);

    return menu;
}

void setupAlphaMenu(FbTk::Menu& parent, ConfigMenu::SetupHelper& sh, _Cmd& save_reconf) {
#ifdef HAVE_XRENDER
    _FB_USES_NLS;

    enum {
        L_ALPHA = 0,
        L_PSEUDO_TRANS,
        L_FOCUS_ALPHA,
        L_UNFOCUS_ALPHA,
        L_MENU_ALPHA,
    };

    static const FbTk::FbString _labels[] = {
        _FB_XTEXT(Configmenu, Transparency, "Transparency", "Menu containing various transparency options"),
        _FB_XTEXT(Configmenu, ForcePseudoTrans, "Force Pseudo-Transparency", "When composite is available, still use old pseudo-transparency"),
        _FB_XTEXT(Configmenu, FocusedAlpha, "Focused Window Alpha", "Transparency level of the focused window"),
        _FB_XTEXT(Configmenu, UnfocusedAlpha, "Unfocused Window Alpha", "Transparency level of unfocused windows"),
        _FB_XTEXT(Configmenu, MenuAlpha, "Menu Alpha", "Transparency level of menu")
    };


    FbTk::FbString label = _labels[L_ALPHA];
    FbTk::Menu* menu = MenuCreator::createMenu(label, sh.screen);

    if (FbTk::Transparent::haveComposite(true)) {
        static FbTk::SimpleAccessor<bool> s_pseudo = Fluxbox::instance()->getPseudoTrans();
        menu->insertItem(new FbTk::BoolMenuItem(_labels[L_PSEUDO_TRANS], s_pseudo, save_reconf));
    }

    // in order to save system resources, don't save or reconfigure alpha
    // settings until after the user is done changing them
    _Cmd delayed_save_and_reconf(new FbTk::DelayedCmd(save_reconf));

    FbTk::MenuItem *focused_alpha_item =
        new FbTk::IntMenuItem(_labels[L_FOCUS_ALPHA], sh.resource.focused_alpha, 0, 255, *menu);
    focused_alpha_item->setCommand(delayed_save_and_reconf);
    menu->insertItem(focused_alpha_item);

    FbTk::MenuItem *unfocused_alpha_item =
        new FbTk::IntMenuItem(_labels[L_UNFOCUS_ALPHA], sh.resource.unfocused_alpha, 0, 255, *menu);
    unfocused_alpha_item->setCommand(delayed_save_and_reconf);
    menu->insertItem(unfocused_alpha_item);

    FbTk::MenuItem *menu_alpha_item =
        new FbTk::IntMenuItem(_labels[L_MENU_ALPHA], sh.resource.menu_alpha, 0, 255, *menu);
    menu_alpha_item->setCommand(delayed_save_and_reconf);
    menu->insertItem(menu_alpha_item);

    menu->updateMenu();
    parent.insertSubmenu(label, menu);
#endif
}


#define _BOOLITEM(m,a, b, c, d, e, f) (m).insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(a, b, c, d), e, f))

void setupFocusMenu(FbTk::Menu& menu, ConfigMenu::SetupHelper& sh, _Cmd& save_rc, _Cmd& save_reconf) {

    _FB_USES_NLS;

    // we don't set this to internal menu so will
    // be deleted toghether with the parent
    FbTk::FbString label = _FB_XTEXT(Configmenu, FocusModel, "Focus Model", "Method used to give focus to windows");
    FbMenu* fm = MenuCreator::createMenu(label, sh.screen);

#define _FOCUSITEM(a, b, c, d, e) \
    fm->insertItem(new FocusModelMenuItem(_FB_XTEXT(a, b, c, d), sh.screen.focusControl(), \
                                              e, save_reconf))

    _FOCUSITEM(Configmenu, ClickFocus,
               "Click To Focus", "Click to focus",
               FocusControl::CLICKFOCUS);
    _FOCUSITEM(Configmenu, MouseFocus,
               "Mouse Focus (Keyboard Friendly)",
               "Mouse Focus (Keyboard Friendly)",
               FocusControl::MOUSEFOCUS);
    _FOCUSITEM(Configmenu, StrictMouseFocus,
               "Mouse Focus (Strict)",
               "Mouse Focus (Strict)",
               FocusControl::STRICTMOUSEFOCUS);
#undef _FOCUSITEM

    fm->insertItem(new FbTk::MenuSeparator());
    fm->insertItem(new TabFocusModelMenuItem(_FB_XTEXT(Configmenu,
        ClickTabFocus, "ClickTabFocus", "Click tab to focus windows"),
        sh.screen.focusControl(), FocusControl::CLICKTABFOCUS, save_reconf));
    fm->insertItem(new TabFocusModelMenuItem(_FB_XTEXT(Configmenu,
        MouseTabFocus, "MouseTabFocus", "Hover over tab to focus windows"),
        sh.screen.focusControl(), FocusControl::MOUSETABFOCUS, save_reconf));
    fm->insertItem(new FbTk::MenuSeparator());

    try {
        fm->insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Configmenu, FocusNew,
            "Focus New Windows", "Focus newly created windows"),
            sh.rm.getResource<bool>(sh.screen.name() + ".focusNewWindows"),
            save_rc));
    } catch (FbTk::ResourceException & e) {
        std::cerr<<e.what()<<std::endl;
    }

#ifdef XINERAMA
    try {
        fm->insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Configmenu, FocusSameHead,
            "Keep Head", "Only revert focus on same head"),
            sh.rm.getResource<bool>(sh.screen.name() + ".focusSameHead"),
            save_rc));
    } catch (FbTk::ResourceException e) {
        std::cerr << e.what() << std::endl;
    }
#endif // XINERAMA

    _BOOLITEM(*fm, Configmenu, AutoRaise,
              "Auto Raise", "Auto Raise windows on sloppy",
              sh.resource.auto_raise, save_rc);
    _BOOLITEM(*fm, Configmenu, ClickRaises,
              "Click Raises", "Click Raises",
              sh.resource.click_raises, save_rc);

    fm->updateMenu();
    menu.insertSubmenu(label, fm);
}


void setupMaximizeMenu(FbTk::Menu& menu, ConfigMenu::SetupHelper& sh, _Cmd& save_rc) {

    _FB_USES_NLS;

    FbTk::FbString label = _FB_XTEXT(Configmenu, MaxMenu,
            "Maximize Options", "heading for maximization options");
    FbTk::Menu* mm = MenuCreator::createMenu(label, sh.screen);

    _BOOLITEM(*mm, Configmenu, FullMax,
              "Full Maximization", "Maximise over slit, toolbar, etc",
              sh.resource.full_max, save_rc);
    _BOOLITEM(*mm, Configmenu, MaxIgnoreInc,
              "Ignore Resize Increment",
              "Maximizing Ignores Resize Increment (e.g. xterm)",
              sh.resource.max_ignore_inc, save_rc);
    _BOOLITEM(*mm, Configmenu, MaxDisableMove,
              "Disable Moving", "Don't Allow Moving While Maximized",
              sh.resource.max_disable_move, save_rc);
    _BOOLITEM(*mm, Configmenu, MaxDisableResize,
              "Disable Resizing", "Don't Allow Resizing While Maximized",
              sh.resource.max_disable_resize, save_rc);

    mm->updateMenu();
    menu.insertSubmenu(label, mm);
}

void setupTabMenu(FbTk::Menu& parent, ConfigMenu::SetupHelper& sh, _Cmd& save_reconf, _Cmd& save_reconftabs) {

    _FB_USES_NLS;

    FbTk::FbString label = _FB_XTEXT(Configmenu, TabMenu, "Tab Options", "heading for tab-related options");
    // TODO: main-category is 'Menu'?? should be 'ConfigMenu'???
    FbTk::FbString p_label = _FB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu");
    FbTk::Menu* menu = MenuCreator::createMenu(label, sh.screen);
    FbTk::Menu* p_menu = createToggleMenu(p_label, sh.screen);

    menu->insertSubmenu(p_label, p_menu);

    menu->insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Configmenu, TabsInTitlebar,
              "Tabs in Titlebar", "Tabs in Titlebar"),
              sh.resource.default_internal_tabs, save_reconftabs));
    menu->insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, MaximizeOver,
              "Maximize Over", "Maximize over this thing when maximizing"),
              sh.resource.max_over_tabs, save_reconf));
    menu->insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Toolbar, ShowIcons,
              "Show Pictures", "chooses if little icons are shown next to title in the iconbar"),
              sh.resource.tabs_use_pixmap, save_reconf));

    FbTk::MenuItem *tab_width_item =
        new FbTk::IntMenuItem(_FB_XTEXT(Configmenu, ExternalTabWidth,
                                       "External Tab Width",
                                       "Width of external-style tabs"),
                               sh.resource.tab_width, 10, 3000, /* silly number */
                               *menu);
    tab_width_item->setCommand(save_reconftabs);
    menu->insertItem(tab_width_item);

    // menu is 3 wide, 5 down
    struct PlacementP {
         const FbTk::FbString label;
         FbWinFrame::TabPlacement placement;
    };
    static const PlacementP place_menu[] = {

        { _FB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), FbWinFrame::TOPLEFT},
        { _FB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), FbWinFrame::LEFTTOP},
        { _FB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), FbWinFrame::LEFT},
        { _FB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), FbWinFrame::LEFTBOTTOM},
        { _FB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), FbWinFrame::BOTTOMLEFT},
        { _FB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), FbWinFrame::TOP},
        { "", FbWinFrame::TOPLEFT},
        { "", FbWinFrame::TOPLEFT},
        { "", FbWinFrame::TOPLEFT},
        { _FB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), FbWinFrame::BOTTOM},
        { _FB_XTEXT(Align, TopRight, "Top Right", "Top Right"), FbWinFrame::TOPRIGHT},
        { _FB_XTEXT(Align, RightTop, "Right Top", "Right Top"), FbWinFrame::RIGHTTOP},
        { _FB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), FbWinFrame::RIGHT},
        { _FB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), FbWinFrame::RIGHTBOTTOM},
        { _FB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), FbWinFrame::BOTTOMRIGHT}
    };

    p_menu->setMinimumColumns(3);
    // create items in sub menu
    for (size_t i=0; i< sizeof(place_menu)/sizeof(PlacementP); ++i) {
        const PlacementP& p = place_menu[i];
        if (p.label == "") {
            p_menu->insert(p.label);
            p_menu->setItemEnabled(i, false);
        } else
            p_menu->insertItem(new TabPlacementMenuItem(p.label, 
                sh.screen, p.placement, save_reconftabs));
    }
    p_menu->updateMenu();
    menu->updateMenu();
    parent.insertSubmenu(label, menu);
}

}


void ConfigMenu::setup(FbTk::Menu& menu, ConfigMenu::SetupHelper& sh) {

    _FB_USES_NLS;

    FbTk::MacroCommand *s_a_reconf_macro = new FbTk::MacroCommand();
    FbTk::MacroCommand *s_a_reconftabs_macro = new FbTk::MacroCommand();
    _Cmd saverc_cmd(new FbTk::SimpleCommand<Fluxbox>( *Fluxbox::instance(), &Fluxbox::save_rc));
    _Cmd reconf_cmd(FbTk::CommandParser<void>::instance().parse("reconfigure"));
    _Cmd reconftabs_cmd(new FbTk::SimpleCommand<BScreen>(sh.screen, &BScreen::reconfigureTabs));
    s_a_reconf_macro->add(saverc_cmd);
    s_a_reconf_macro->add(reconf_cmd);
    s_a_reconftabs_macro->add(saverc_cmd);
    s_a_reconftabs_macro->add(reconftabs_cmd);

    _Cmd save_and_reconfigure(s_a_reconf_macro);
    _Cmd save_and_reconftabs(s_a_reconftabs_macro);

    setupFocusMenu(menu, sh, saverc_cmd, save_and_reconfigure);
    setupMaximizeMenu(menu, sh, saverc_cmd);
    setupTabMenu(menu, sh, save_and_reconfigure, save_and_reconftabs);

#ifdef HAVE_XRENDER
    if (FbTk::Transparent::haveRender() ||
        FbTk::Transparent::haveComposite()) {
        setupAlphaMenu(menu, sh, save_and_reconfigure);
    }
#endif // HAVE_XRENDER

    _BOOLITEM(menu, Configmenu, OpaqueMove,
              "Opaque Window Moving",
              "Window Moving with whole window visible (as opposed to outline moving)",
              sh.resource.opaque_move, saverc_cmd);
    _BOOLITEM(menu, Configmenu, OpaqueResize,
              "Opaque Window Resizing",
              "Window Resizing with whole window visible (as opposed to outline resizing)",
              sh.resource.opaque_resize, saverc_cmd);
    _BOOLITEM(menu, Configmenu, WorkspaceWarping,
              "Workspace Warping",
              "Workspace Warping - dragging windows to the edge and onto the next workspace",
              sh.resource.workspace_warping, saverc_cmd);

#undef _BOOLITEM

    menu.updateMenu();
}
