// IconbarTool.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "IconbarTool.hh"

#include "Screen.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"
#include "Workspace.hh"
#include "FbMenu.hh"
#include "BoolMenuItem.hh"
#include "CommandParser.hh"
#include "WinClient.hh"
#include "FocusControl.hh"
#include "FbCommands.hh"
#include "Layer.hh"

#include "FbTk/I18n.hh"
#include "FbTk/Menu.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/MenuSeparator.hh"

#include <typeinfo>
#include <string>
#include <iterator>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;
using std::list;

#ifdef DEBUG
using std::cerr;
using std::endl;
#endif // DEBUG

namespace FbTk {

template<>
void FbTk::Resource<IconbarTool::Mode>::setFromString(const char *strval) {
    if (strcasecmp(strval, "None") == 0)
        m_value = IconbarTool::NONE;
    else if (strcasecmp(strval, "Icons") == 0)
        m_value = IconbarTool::ICONS;
    else if (strcasecmp(strval, "NoIcons") == 0)
        m_value = IconbarTool::NOICONS;
    else if (strcasecmp(strval, "WorkspaceIcons") == 0)
        m_value = IconbarTool::WORKSPACEICONS;
    else if (strcasecmp(strval, "WorkspaceNoIcons") == 0)
        m_value = IconbarTool::WORKSPACENOICONS;
    else if (strcasecmp(strval, "Workspace") == 0)
        m_value = IconbarTool::WORKSPACE;
    else if (strcasecmp(strval, "AllWindows") == 0)
        m_value = IconbarTool::ALLWINDOWS;
    else
        setDefaultValue();
}

template<>
void FbTk::Resource<IconbarTool::WheelMode>::setDefaultValue() {
    m_value = IconbarTool::SCREEN;
}


template<>
void FbTk::Resource<IconbarTool::WheelMode>::setFromString(const char* strval) {
    if (strncasecmp(strval, "off", strlen("off")) == 0)
        m_value = IconbarTool::OFF;
    else if (strncasecmp(strval, "on", strlen("on")) == 0)
        m_value = IconbarTool::ON;
    else if (strncasecmp(strval, "screen", strlen("screen")) == 0)
        m_value = IconbarTool::SCREEN;
    else
        setDefaultValue();
}


template<>
string FbTk::Resource<IconbarTool::WheelMode>::getString() const {
    switch(m_value) {
    case IconbarTool::ON:
        return string("On");
        break;
    case IconbarTool::SCREEN:
        return string("Screen");
        break;
    case IconbarTool::OFF:
    default:
        return string("Off");
    };
}

template<>
void FbTk::Resource<Container::Alignment>::setDefaultValue() {
    m_value = Container::RELATIVE;
}

template<>
string FbTk::Resource<Container::Alignment>::getString() const {
    switch (m_value) {
    case Container::LEFT:
        return string("Left");
    case Container::RIGHT:
        return string("Right");
    case Container::RELATIVE:
        return string("Relative");
    }
    return string("Left");
}

template<>
void FbTk::Resource<Container::Alignment>::setFromString(const char *str) {
    if (strcasecmp(str, "Left") == 0)
        m_value = Container::LEFT;
    else if (strcasecmp(str, "Right") == 0)
        m_value = Container::RIGHT;
    else if (strcasecmp(str, "Relative") == 0)
        m_value = Container::RELATIVE;
    else
        setDefaultValue();
}

template<>
string FbTk::Resource<IconbarTool::Mode>::getString() const {

    switch (m_value) {
    case IconbarTool::NONE:
        return string("None");
        break;
    case IconbarTool::ICONS:
        return string("Icons");
        break;
    case IconbarTool::NOICONS:
        return string("NoIcons");
        break;
    case IconbarTool::WORKSPACEICONS:
        return string("WorkspaceIcons");
        break;
    case IconbarTool::WORKSPACENOICONS:
        return string("WorkspaceNoIcons");
        break;
    case IconbarTool::WORKSPACE:
        return string("Workspace");
        break;
    case IconbarTool::ALLWINDOWS:
        return string("AllWindows");
        break;
    }
    // default string
    return string("Icons");
}
} // end namespace FbTk

namespace {

class ToolbarModeMenuItem : public FbTk::MenuItem {
public:
    ToolbarModeMenuItem(const FbTk::FbString &label, IconbarTool &handler,
                        IconbarTool::Mode mode,
                        FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_handler(handler), m_mode(mode) {
    }
    bool isEnabled() const { return m_handler.mode() != m_mode; }
    void click(int button, int time) {
        m_handler.setMode(m_mode);
        FbTk::MenuItem::click(button, time);
    }

private:
    IconbarTool &m_handler;
    IconbarTool::Mode m_mode;
};

class ToolbarAlignMenuItem: public FbTk::MenuItem {
public:
    ToolbarAlignMenuItem(const FbTk::FbString &label, IconbarTool &handler,
                        Container::Alignment mode,
                        FbTk::RefCount<FbTk::Command> &cmd):
        FbTk::MenuItem(label, cmd), m_handler(handler), m_mode(mode) {
    }
    bool isEnabled() const { return m_handler.alignment() != m_mode; }
    void click(int button, int time) {
        m_handler.setAlignment(m_mode);
        FbTk::MenuItem::click(button, time);
    }

private:
    IconbarTool &m_handler;
    Container::Alignment m_mode;
};

void setupModeMenu(FbTk::Menu &menu, IconbarTool &handler) {
    using namespace FbTk;
    _FB_USES_NLS;

    menu.setLabel(_FB_XTEXT(Toolbar, IconbarMode, "Iconbar Mode", "Menu title - chooses which set of icons are shown in the iconbar"));

    RefCount<Command> saverc_cmd(new FbCommands::SaveResources());


    menu.insert(new ToolbarModeMenuItem(_FB_XTEXT(Toolbar, IconbarModeNone,
                                                "None", "No icons are shown in the iconbar"),
                    handler,
                    IconbarTool::NONE, saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeIcons,
                            "Icons", "Iconified windows from all workspaces are shown"),
                    handler,
                    IconbarTool::ICONS, saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeNoIcons,
                        "NoIcons", "No iconified windows from all workspaces are shown"),
                    handler,
                    IconbarTool::NOICONS, saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeWorkspaceIcons,
                            "WorkspaceIcons", "Iconified windows from this workspace are shown"),
                    handler,
                    IconbarTool::WORKSPACEICONS, saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeWorkspaceNoIcons,
                            "WorkspaceNoIcons", "No iconified windows from this workspace are shown"),
                    handler,
                    IconbarTool::WORKSPACENOICONS, saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeWorkspace,
                            "Workspace", "Normal and iconified windows from this workspace are shown"),
                    handler,
                    IconbarTool::WORKSPACE, saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeAllWindows, "All Windows", "All windows are shown"),
                    handler,
                    IconbarTool::ALLWINDOWS, saverc_cmd));

    menu.insert(new FbTk::MenuSeparator());

    menu.insert(new ToolbarAlignMenuItem(
                    _FB_XTEXT(Align, Left, "Left", "Align to the left"),
                    handler,
                    Container::LEFT, saverc_cmd));

    menu.insert(new ToolbarAlignMenuItem(
                    _FB_XTEXT(Align, Relative, "Relative", "Align relative to the width"),
                    handler,
                    Container::RELATIVE, saverc_cmd));

    menu.insert(new ToolbarAlignMenuItem(
                    _FB_XTEXT(Align, Right, "Right", "Align to the right"),
                    handler,
                    Container::RIGHT, saverc_cmd));

    menu.insert(new FbTk::MenuSeparator());

    menu.updateMenu();
}

inline bool checkAddWindow(IconbarTool::Mode mode, const FluxboxWindow &win) {
    bool ret_val = false;
    // just add the icons that are on the this workspace
    switch (mode) {
    case IconbarTool::NONE:
        break;
    case IconbarTool::ICONS:
        if (win.isIconic())
            ret_val = true;
        break;
    case IconbarTool::NOICONS:
        if (!win.isIconic())
            ret_val = true;
        break;
    case IconbarTool::WORKSPACEICONS:
        if(win.workspaceNumber() == win.screen().currentWorkspaceID() &&
           win.isIconic())
            ret_val = true;
        break;
    case IconbarTool::WORKSPACENOICONS:
        if (win.isIconic())
            break;
    case IconbarTool::WORKSPACE:
        if (win.workspaceNumber() == win.screen().currentWorkspaceID())
            ret_val = true;
        break;
    case IconbarTool::ALLWINDOWS:
        ret_val = true;
        break;
    }

    if (win.isIconHidden())
        ret_val = false;

    return ret_val;
}

void removeDuplicate(const IconbarTool::IconList &iconlist, list<FluxboxWindow *> &windowlist) {
    IconbarTool::IconList::const_iterator icon_it = iconlist.begin();
    IconbarTool::IconList::const_iterator icon_it_end = iconlist.end();
    list<FluxboxWindow *>::iterator remove_it = windowlist.end();
    for (; icon_it != icon_it_end; ++icon_it)
        remove_it = remove(windowlist.begin(), remove_it, &(*icon_it)->win());

    // remove already existing windows
    windowlist.erase(remove_it, windowlist.end());
}

}; // end anonymous namespace

IconbarTool::IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, BScreen &screen,
                         FbTk::Menu &menu):
    ToolbarItem(ToolbarItem::RELATIVE),
    m_screen(screen),
    m_icon_container(parent),
    m_theme(theme),
    m_focused_pm( screen.imageControl() ),
    m_unfocused_pm( screen.imageControl() ),
    m_focused_err_pm( screen.imageControl() ),
    m_unfocused_err_pm( screen.imageControl() ),
    m_empty_pm( screen.imageControl() ),
    m_rc_mode(screen.resourceManager(), WORKSPACE,
              screen.name() + ".iconbar.mode", screen.altName() + ".Iconbar.Mode"),
    m_wheel_mode(screen.resourceManager(), OFF,
                 screen.name() + ".iconbar.wheelMode",
                 screen.name() + ".iconbar.WheelMode"),
    m_rc_alignment(screen.resourceManager(), Container::LEFT,
                   screen.name() + ".iconbar.alignment", screen.altName() + ".Iconbar.Alignment"),
    m_rc_client_width(screen.resourceManager(), 70,
                   screen.name() + ".iconbar.iconWidth", screen.altName() + ".Iconbar.IconWidth"),
    m_rc_client_padding(screen.resourceManager(), 10,
                   screen.name() + ".iconbar.iconTextPadding", screen.altName() + ".Iconbar.IconTextPadding"),
    m_rc_use_pixmap(screen.resourceManager(), true,
                    screen.name() + ".iconbar.usePixmap", screen.altName() + ".Iconbar.UsePixmap"),
    m_menu(screen.menuTheme(), screen.imageControl(),
           *screen.layerManager().getLayer(Layer::MENU)) {

    // setup mode menu
    setupModeMenu(m_menu, *this);
    _FB_USES_NLS;
    using namespace FbTk;
    // setup use pixmap item to reconfig iconbar and save resource on click
    MacroCommand *save_and_reconfig = new MacroCommand();
    RefCount<Command> reconfig(new SimpleCommand<IconbarTool>(*this, &IconbarTool::renderTheme));
    RefCount<Command> save(CommandParser::instance().parseLine("saverc"));
    save_and_reconfig->add(reconfig);
    save_and_reconfig->add(save);
    RefCount<Command> s_and_reconfig(save_and_reconfig);
    m_menu.insert(new BoolMenuItem(_FB_XTEXT(Toolbar, ShowIcons,
                    "Show Pictures", "chooses if little icons are shown next to title in the iconbar"),
	                *m_rc_use_pixmap, s_and_reconfig));
    m_menu.updateMenu();
    // must be internal menu, otherwise toolbar main menu tries to delete it.
    m_menu.setInternalMenu();

    // add iconbar menu to toolbar menu
    menu.insert(m_menu.label(), &m_menu);

    // setup signals
    theme.reconfigSig().attach(this);
    screen.clientListSig().attach(this);
    screen.iconListSig().attach(this);
    screen.currentWorkspaceSig().attach(this);
    // setup focus timer

    FbTk::RefCount<FbTk::Command> timer_cmd(new FbTk::SimpleCommand<IconbarTool>(*this, &IconbarTool::timedRender));
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 1; // so it updates next event round
    m_focus_timer.setCommand(timer_cmd);
    m_focus_timer.setTimeout(to);
    m_focus_timer.fireOnce(true);

}

IconbarTool::~IconbarTool() {
    deleteIcons();
}

void IconbarTool::move(int x, int y) {
    m_icon_container.move(x, y);
}

void IconbarTool::resize(unsigned int width, unsigned int height) {
    m_icon_container.resize(width, height);
    renderTheme();
}

void IconbarTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {

    m_icon_container.moveResize(x, y, width, height);
    renderTheme();
}

void IconbarTool::show() {
    m_icon_container.show();
}

void IconbarTool::hide() {
    m_icon_container.hide();
}

void IconbarTool::setAlignment(Container::Alignment align) {
    *m_rc_alignment = align;
    update(0);
    m_menu.reconfigure();
}

void IconbarTool::setMode(Mode mode) {
    if (mode == *m_rc_mode)
        return;

    *m_rc_mode = mode;

    // lock graphics update
    m_icon_container.setUpdateLock(true);

    deleteIcons();

    // update mode
    if (mode != NONE)
        updateList();

    // unlock graphics update
    m_icon_container.setUpdateLock(false);
    m_icon_container.update();
    m_icon_container.showSubwindows();

    renderTheme();

    m_menu.reconfigure();
}

unsigned int IconbarTool::width() const {
    return m_icon_container.width();
}

unsigned int IconbarTool::height() const {
    return m_icon_container.height();
}

unsigned int IconbarTool::borderWidth() const {
    return m_icon_container.borderWidth();
}

void IconbarTool::update(FbTk::Subject *subj) {
    // ignore updates if we're shutting down
    if (m_screen.isShuttingdown()) {
        m_screen.clientListSig().detach(this);
        m_screen.iconListSig().detach(this);
        m_screen.currentWorkspaceSig().detach(this);
        if (!m_icon_list.empty())
            deleteIcons();
        return;
    }

    m_icon_container.setAlignment(*m_rc_alignment);
    // clamp to normal values
    if (*m_rc_client_width < 1)
        *m_rc_client_width = 10;
    else if (*m_rc_client_width > 400)
        *m_rc_client_width = 400;

    m_icon_container.setMaxSizePerClient(*m_rc_client_width);


    if (mode() == NONE) {
        if (subj != 0 && typeid(*subj) == typeid(IconbarTheme))
            renderTheme();
        return;
    }

    // handle window signal
    if (subj != 0 && typeid(*subj) == typeid(FluxboxWindow::WinSubject)) {
        // we handle everything except die signal here
        FluxboxWindow::WinSubject *winsubj = static_cast<FluxboxWindow::WinSubject *>(subj);
        if (subj == &(winsubj->win().focusSig())) {
            // start focus timer, so we can update without flicker
            m_focus_timer.start();

            return;
        } else if (subj == &(winsubj->win().workspaceSig())) {
            // we can ignore this signal if we're in ALLWINDOWS mode
            // unless the window was focused and has nothing to revert to
            if (mode() == ALLWINDOWS || mode() == ICONS || mode() == NOICONS) {
                m_focus_timer.start();
                return;
            }

            // workspace changed for this window, and if it's not on current workspace we remove it
            if (m_screen.currentWorkspaceID() != winsubj->win().workspaceNumber()) {
                removeWindow(winsubj->win());
                renderTheme();
            }
            return;
        } else if (subj == &(winsubj->win().dieSig())) { // die sig
            removeWindow(winsubj->win());
            renderTheme();
            return; // we don't need to update the entire list
        } else if (subj == &(winsubj->win().stateSig())) {
            if (!checkAddWindow(mode(), winsubj->win())) {
                removeWindow(winsubj->win());
                renderTheme();
            }

            return;

        } else if (subj == &(winsubj->win().attentionSig())) {
            // render with titlebar focus, on attention
            IconButton *button = findButton(winsubj->win());
            if (button) {
                renderButton(*button, true,
                             winsubj->win().getAttentionState());
            }
            return;
        } else {
            // signal not handled
            return;
        }
    }

    bool remove_all = false; // if we should readd all windows

    if (subj != 0 && typeid(*subj) == typeid(BScreen::ScreenSubject) &&
        mode() != ALLWINDOWS && mode() != ICONS && mode() != NOICONS) {
        BScreen::ScreenSubject *screen_subj = static_cast<BScreen::ScreenSubject *>(subj);
        // current workspace sig
        if (&m_screen.currentWorkspaceSig() == screen_subj ) {
            remove_all = true; // remove and readd all windows
        }

    }

    // lock graphic update
    m_icon_container.setUpdateLock(true);

    if (remove_all)
        deleteIcons();

    // ok, we got some signal that we need to update our iconbar container
    updateList();

    // unlock container and update graphics
    m_icon_container.setUpdateLock(false);
    m_icon_container.update();
    m_icon_container.showSubwindows();

    // another renderTheme we hopefully shouldn't need? These renders
    // should be done individually above

    // nope, we still need it (or at least I'm not bothering to fix it yet)
    // a new IconButton doesn't get resized properly until the
    // m_icon_container.update() above; then, it never runs drawText() again,
    // so text can end up behind program icons
    renderTheme();
}

IconButton *IconbarTool::findButton(FluxboxWindow &win) {

    IconList::iterator icon_it = m_icon_list.begin();
    IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if (&(*icon_it)->win() == &win)
            return *icon_it;
    }

    return 0;
}
/*
void IconbarTool::renderWindow(FluxboxWindow &win) {
    IconButton *button = findButton(win);
    if (button == 0)
        return;
    renderButton(*button);
}
*/
void IconbarTool::updateSizing() {
    m_icon_container.setBorderWidth(m_theme.border().width());

    IconList::iterator icon_it = m_icon_list.begin();
    const IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        if ((*icon_it)->win().isFocused())
            (*icon_it)->setBorderWidth(m_theme.focusedBorder().width());
        else // unfocused
            (*icon_it)->setBorderWidth(m_theme.unfocusedBorder().width());
    }

}

void IconbarTool::renderTheme(unsigned char alpha) {

    m_alpha = alpha;
    renderTheme();
}

void IconbarTool::renderTheme() {

    // update button sizes before we get max width per client!
    updateSizing();

    unsigned int icon_width = 0, icon_height = 0;
    unsigned int icon_width_off=0, icon_height_off=0;

    if (orientation() == FbTk::ROT0 || orientation() == FbTk::ROT180) {
        icon_width = m_icon_container.maxWidthPerClient();
        icon_height = m_icon_container.height();
        icon_width_off = 1;
    } else {
        icon_width = m_icon_container.width();
        icon_height = m_icon_container.maxWidthPerClient();
        icon_height_off = 1;
    }

    if (!m_theme.focusedTexture().usePixmap()) {
        m_focused_pm.reset( 0 );
        m_focused_err_pm.reset( 0 );
    } else {
        m_focused_pm.reset( m_screen.imageControl().
                            renderImage(icon_width, icon_height,
                                        m_theme.focusedTexture(), orientation()) );
        m_focused_err_pm.reset( m_screen.imageControl().
                                renderImage(icon_width + icon_width_off,
                                            icon_height + icon_height_off,
                                            m_theme.focusedTexture(), orientation()) );
    }

    if (!m_theme.unfocusedTexture().usePixmap()) {
        m_unfocused_pm.reset( 0 );
        m_unfocused_err_pm.reset( 0 );
    } else {
        m_unfocused_pm.reset( m_screen.imageControl().
                              renderImage(icon_width, icon_height,
                                          m_theme.unfocusedTexture(), orientation()) );
        m_unfocused_err_pm.reset( m_screen.imageControl().
                                  renderImage(icon_width+icon_width_off,
                                              icon_height+icon_height_off,
                                              m_theme.unfocusedTexture(), orientation()) );
    }

    // if we dont have any icons then we should render empty texture
    if (!m_theme.emptyTexture().usePixmap()) {
        m_empty_pm.reset( 0 );
        m_icon_container.setBackgroundColor(m_theme.emptyTexture().color());
    } else {
        m_empty_pm.reset( m_screen.imageControl().
                          renderImage(m_icon_container.width(), m_icon_container.height(),
                                      m_theme.emptyTexture(), orientation()) );
        m_icon_container.setBackgroundPixmap(m_empty_pm);
    }

    // set to zero so its consistent and not ugly
    m_icon_container.setBorderWidth(m_theme.border().width());
    m_icon_container.setBorderColor(m_theme.border().color());
    m_icon_container.setAlpha(m_alpha);

    // update buttons
    IconList::iterator icon_it = m_icon_list.begin();
    const IconList::iterator icon_it_end = m_icon_list.end();
    for (; icon_it != icon_it_end; ++icon_it) {
        renderButton(*(*icon_it));
    }
}

void IconbarTool::renderButton(IconButton &button, bool clear,
                               int focusOption) {

    button.setPixmap(*m_rc_use_pixmap);
    button.setAlpha(m_alpha);
    button.setTextPadding(*m_rc_client_padding);

    // The last button is always the regular width
    bool wider_button = false;
    if (!m_icon_container.empty()) {
        if (button.orientation() == FbTk::ROT0 || button.orientation() == FbTk::ROT180)
            wider_button = button.width() != m_icon_container.back()->width();
        else
            wider_button = button.height() != m_icon_container.back()->height();
//            wider_button = (button.width() != m_icon_container.maxWidthPerClient() || // height to cover both orients

//                        button.height() != m_icon_container.back()->height());
    }

    if (focusOption == 1 ||
        (focusOption == -1 &&
         button.win().isFocused())) {

        // focused texture
        if (button.win().isFocused())
            m_icon_container.setSelected(m_icon_container.find(&button));

        button.setGC(m_theme.focusedText().textGC());
        button.setFont(m_theme.focusedText().font());
        button.setJustify(m_theme.focusedText().justify());
        button.setBorderWidth(m_theme.focusedBorder().width());
        button.setBorderColor(m_theme.focusedBorder().color());

        if (!wider_button && m_focused_pm != 0)
            button.setBackgroundPixmap(m_focused_pm);
        else if (wider_button && m_focused_err_pm != 0)
            button.setBackgroundPixmap(m_focused_err_pm);
        else
            button.setBackgroundColor(m_theme.focusedTexture().color());

    } else { // unfocused
        if (m_icon_container.selected() == &button)
            m_icon_container.setSelected(-1);

        button.setGC(m_theme.unfocusedText().textGC());
        button.setFont(m_theme.unfocusedText().font());
        button.setJustify(m_theme.unfocusedText().justify());
        button.setBorderWidth(m_theme.unfocusedBorder().width());
        button.setBorderColor(m_theme.unfocusedBorder().color());

        if (!wider_button && m_unfocused_pm != 0)
            button.setBackgroundPixmap(m_unfocused_pm);
        else if (wider_button && m_unfocused_err_pm != 0)
            button.setBackgroundPixmap(m_unfocused_err_pm);
        else
            button.setBackgroundColor(m_theme.unfocusedTexture().color());
    }

    if (clear)
        button.clear(); // the clear also updates transparent
}

void IconbarTool::deleteIcons() {
    m_icon_container.removeAll();
    while (!m_icon_list.empty()) {
        delete m_icon_list.back();
        m_icon_list.pop_back();
    }
}

void IconbarTool::removeWindow(FluxboxWindow &win) {
    // got window die signal, lets find and remove the window
    IconList::iterator it = m_icon_list.begin();
    IconList::iterator it_end = m_icon_list.end();
    for (; it != it_end; ++it) {
        if (&(*it)->win() == &win)
            break;
    }
    // did we find it?
    if (it == m_icon_list.end()) {
        return;
    }
#ifdef DEBUG
    cerr<<"IconbarTool::"<<__FUNCTION__<<"( 0x"<<&win<<" title = "<<win.title()<<") found!"<<endl;
#endif // DEBUG
    // detach from all signals
    win.focusSig().detach(this);
    win.dieSig().detach(this);
    win.workspaceSig().detach(this);
    win.stateSig().detach(this);
    win.titleSig().detach(this);


    // remove from list and render theme again
    IconButton *button = *it;

    m_icon_container.removeItem(m_icon_container.find(*it));
    m_icon_list.erase(it);

    delete button;

}

void IconbarTool::addWindow(FluxboxWindow &win) {
    // we just want windows that has clients
    if (win.clientList().empty() || win.isIconHidden() )
        return;
#ifdef DEBUG
    cerr<<"IconbarTool::addWindow(0x"<<&win<<" title = "<<win.title()<<")"<<endl;
#endif // DEBUG
    IconButton *button = new IconButton(*this,
                                        m_icon_container,
                                        m_theme.focusedText().font(),
                                        win);


    renderButton(*button, false); // update the attributes, but don't clear it
    m_icon_container.insertItem(button);
    m_icon_list.push_back(button);

    // dont forget to detach signal in removeWindow
    win.focusSig().attach(this);
    win.dieSig().attach(this);
    win.workspaceSig().attach(this);
    win.stateSig().attach(this);
    win.attentionSig().attach(this);
}

void IconbarTool::updateList() {
    list<WinClient *> ordered_list =
        m_screen.focusControl().creationOrderList();
    list<WinClient *>::iterator it = ordered_list.begin();
    list<WinClient *>::iterator it_end = ordered_list.end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow() && checkAddWindow(mode(), *(*it)->fbwindow()) &&
            !checkDuplicate(*(*it)->fbwindow()))
            addWindow(*(*it)->fbwindow());
    }

    renderTheme();
}

bool IconbarTool::checkDuplicate(FluxboxWindow &win) {
    IconList::iterator it = m_icon_list.begin();
    IconList::iterator it_end = m_icon_list.end();
    for (; it != it_end; ++it) {
        if (&win == &(*it)->win())
            return true;
    }
    return false;
}

void IconbarTool::timedRender() {
    WinClient *client = FocusControl::focusedWindow();
    IconButton *current_button = static_cast<IconButton *>(m_icon_container.selected());

    if (client == 0 || client->fbwindow() == 0) {
        if (current_button != 0)
            renderButton(*current_button);
        return;
    }

    IconButton *button = findButton(*client->fbwindow());
    // if old window is the same as the new focused window then ignore this render
    // else render old client and new client
    if (button == current_button)
        return;
    if (button != 0)
        renderButton(*button);
    if (current_button != 0)
        renderButton(*current_button);
}

void IconbarTool::setOrientation(FbTk::Orientation orient) {
    m_icon_container.setOrientation(orient);
    ToolbarItem::setOrientation(orient);
}
