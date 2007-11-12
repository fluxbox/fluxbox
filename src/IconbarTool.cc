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

#include "fluxbox.hh"
#include "WindowCmd.hh"
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
#include "STLUtil.hh"

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

} // end namespace FbTk

namespace {

class ToolbarModeMenuItem : public FbTk::MenuItem {
public:
    ToolbarModeMenuItem(const FbTk::FbString &label, IconbarTool &handler,
                        string mode,
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
    string m_mode;
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
                    "none", saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeIcons,
                            "Icons", "Iconified windows from all workspaces are shown"),
                    handler,
                    "{static groups} (minimized=yes)", saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeNoIcons,
                        "NoIcons", "No iconified windows from all workspaces are shown"),
                    handler,
                    "{static groups} (minimized=no)", saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeWorkspaceIcons,
                            "WorkspaceIcons", "Iconified windows from this workspace are shown"),
                    handler,
                    "{static groups} (minimized=yes) (workspace)", saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeWorkspaceNoIcons,
                            "WorkspaceNoIcons", "No iconified windows from this workspace are shown"),
                    handler,
                    "{static groups} (minimized=no) (workspace)", saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeWorkspace,
                            "Workspace", "Normal and iconified windows from this workspace are shown"),
                    handler,
                    "{static groups} (workspace)", saverc_cmd));

    menu.insert(new ToolbarModeMenuItem(
                    _FB_XTEXT(Toolbar, IconbarModeAllWindows, "All Windows", "All windows are shown"),
                    handler,
                    "{static groups}", saverc_cmd));

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

typedef FbTk::RefCount<FbTk::Command> RefCmd;

class ShowMenu: public FbTk::Command {
public:
    explicit ShowMenu(FluxboxWindow &win):m_win(win) { }
    void execute() {
        // hide the menu if it's already showing for this FluxboxWindow
        if (m_win.menu().isVisible() && WindowCmd<void>::window() == &m_win) {
            m_win.screen().hideMenus();
            return;
        }
        m_win.screen().hideMenus();
        // get last button pos
        const XEvent &event = Fluxbox::instance()->lastEvent();
        int x = event.xbutton.x_root - (m_win.menu().width() / 2);
        int y = event.xbutton.y_root - (m_win.menu().height() / 2);
        m_win.showMenu(x, y);
    }
private:
    FluxboxWindow &m_win;
};

class FocusCommand: public FbTk::Command {
public:
    explicit FocusCommand(Focusable &win): m_win(win) { }
    void execute() {
        // this needs to be a local variable, as this object could be destroyed
        // if the workspace is changed.
        FluxboxWindow *fbwin = m_win.fbwindow();
        if (!fbwin)
            return;
        if (m_win.isFocused())
            fbwin->iconify();
        else {
            m_win.focus();
            fbwin->raise();
        }
    }

private:
    Focusable &m_win;
};

}; // end anonymous namespace

IconbarTool::IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme,
                         BScreen &screen, FbTk::Menu &menu):
    ToolbarItem(ToolbarItem::RELATIVE),
    m_screen(screen),
    m_icon_container(parent),
    m_theme(theme),
    m_empty_pm( screen.imageControl() ),
    m_winlist(new FocusableList(screen)),
    m_mode("none"),
    m_rc_mode(screen.resourceManager(), "{static groups} (workspace)",
              screen.name() + ".iconbar.mode", screen.altName() + ".Iconbar.Mode"),
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
    setMode(*m_rc_mode);
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

void IconbarTool::setMode(string mode) {
    if (mode == m_mode)
        return;

    *m_rc_mode = m_mode = mode;

    // lock graphics update
    m_icon_container.setUpdateLock(true);

    if (m_winlist.get()) {
        m_winlist->addSig().detach(this);
        m_winlist->removeSig().detach(this);
        m_winlist->orderSig().detach(this);
        m_winlist->resetSig().detach(this);
    }
    if (mode == "none")
        m_winlist.reset(new FocusableList(m_screen));
    else
        m_winlist.reset(new FocusableList(m_screen,
                                           mode + " (iconhidden=no)"));
    if (m_winlist.get()) {
        m_winlist->addSig().attach(this);
        m_winlist->removeSig().attach(this);
        m_winlist->orderSig().attach(this);
        m_winlist->resetSig().attach(this);
    }
    reset();

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
        if (!m_icons.empty())
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

    if (subj == &m_theme.reconfigSig()) {
        setMode(*m_rc_mode);
        return;
    }

    // lock graphic update
    m_icon_container.setUpdateLock(true);

    if (typeid(*subj) == typeid(FocusableList::FocusableListSubject)) {
        FocusableList::FocusableListSubject *fsubj =
            static_cast<FocusableList::FocusableListSubject *>(subj);
        if (subj == &m_winlist->addSig())
            insertWindow(*fsubj->win());
        else if (subj == &m_winlist->removeSig())
            removeWindow(*fsubj->win());
        else if (subj == &m_winlist->resetSig())
            reset();
        else if (subj == &m_winlist->orderSig())
            insertWindow(*fsubj->win());
    }

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

void IconbarTool::insertWindow(Focusable &win, int pos) {
    IconButton *button = 0;

    IconMap::iterator icon_it = m_icons.find(&win);
    if (icon_it != m_icons.end())
        button = icon_it->second;

    if (button)
        m_icon_container.removeItem(button);
    else
        button = makeButton(win);
    if (!button) return;

    if (pos == -2) {
        pos = 0;
        list<Focusable *>::iterator it = m_winlist->clientList().begin(),
                                    it_end = m_winlist->clientList().end();
        for (; it != it_end && *it != &win; ++it)
            pos++;
    }

    m_icon_container.insertItem(button, pos);
}

void IconbarTool::reset() {
    deleteIcons();
    updateList();
}

void IconbarTool::updateSizing() {
    m_icon_container.setBorderWidth(m_theme.border().width());

    IconMap::iterator icon_it = m_icons.begin();
    const IconMap::iterator icon_it_end = m_icons.end();
    for (; icon_it != icon_it_end; ++icon_it)
        icon_it->second->reconfigTheme();

}

void IconbarTool::renderTheme(unsigned char alpha) {

    m_alpha = alpha;
    renderTheme();
}

void IconbarTool::renderTheme() {

    // update button sizes before we get max width per client!
    updateSizing();

    // if we dont have any icons then we should render empty texture
    if (!m_theme.emptyTexture().usePixmap()) {
        m_empty_pm.reset( 0 );
        m_icon_container.setBackgroundColor(m_theme.emptyTexture().color());
    } else {
        m_empty_pm.reset(m_screen.imageControl().
                          renderImage(m_icon_container.width(),
                                      m_icon_container.height(),
                                      m_theme.emptyTexture(), orientation()));
        m_icon_container.setBackgroundPixmap(m_empty_pm);
    }

    m_icon_container.setAlpha(m_alpha);

    // update buttons
    IconMap::iterator icon_it = m_icons.begin();
    const IconMap::iterator icon_it_end = m_icons.end();
    for (; icon_it != icon_it_end; ++icon_it)
        renderButton(*icon_it->second);

}

void IconbarTool::renderButton(IconButton &button, bool clear) {

    button.setPixmap(*m_rc_use_pixmap);
    button.setTextPadding(*m_rc_client_padding);
    button.reconfigTheme();
    if (clear)
        button.clear(); // the clear also updates transparent
}

void IconbarTool::deleteIcons() {
    m_icon_container.removeAll();
    STLUtil::destroyAndClearSecond(m_icons);
}

void IconbarTool::removeWindow(Focusable &win) {
    // got window die signal, lets find and remove the window
    IconMap::iterator it = m_icons.find(&win);
    if (it == m_icons.end())
        return;
#ifdef DEBUG
    cerr<<"IconbarTool::"<<__FUNCTION__<<"( 0x"<<&win<<" title = "<<win.title()<<") found!"<<endl;
#endif // DEBUG

    // remove from list and render theme again
    IconButton *button = it->second;
    m_icons.erase(it);
    m_icon_container.removeItem(button);
    delete button;
}

IconButton *IconbarTool::makeButton(Focusable &win) {
    // we just want windows that have clients
    FluxboxWindow *fbwin = win.fbwindow();
    if (!fbwin || fbwin->clientList().empty())
        return 0;
#ifdef DEBUG
    cerr<<"IconbarTool::addWindow(0x"<<&win<<" title = "<<win.title()<<")"<<endl;
#endif // DEBUG
    IconButton *button = new IconButton(m_icon_container, m_theme, win);

    RefCmd focus_cmd(new ::FocusCommand(win));
    RefCmd menu_cmd(new ::ShowMenu(*fbwin));
    button->setOnClick(focus_cmd, 1);
    button->setOnClick(menu_cmd, 3);

    renderButton(*button, false); // update the attributes, but don't clear it
    m_icons[&win] = button;
    return button;
}

void IconbarTool::updateList() {
    list<Focusable *>::iterator it = m_winlist->clientList().begin();
    list<Focusable *>::iterator it_end = m_winlist->clientList().end();
    for (; it != it_end; ++it) {
        if ((*it)->fbwindow())
            insertWindow(**it, -1);
    }

    renderTheme();
}

void IconbarTool::setOrientation(FbTk::Orientation orient) {
    m_icon_container.setOrientation(orient);
    ToolbarItem::setOrientation(orient);
}
