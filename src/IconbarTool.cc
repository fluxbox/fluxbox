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

#include "IconbarTool.hh"

#include "fluxbox.hh"
#include "WindowCmd.hh"
#include "Screen.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"
#include "Workspace.hh"
#include "FbMenu.hh"
#include "FbTk/CommandParser.hh"
#include "WinClient.hh"
#include "FocusControl.hh"
#include "FbCommands.hh"
#include "Layer.hh"
#include "Debug.hh"

#include "FbTk/STLUtil.hh"
#include "FbTk/I18n.hh"
#include "FbTk/Menu.hh"
#include "FbTk/RadioMenuItem.hh"
#include "FbTk/BoolMenuItem.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/MenuSeparator.hh"
#include "FbTk/Util.hh"
#include "FbTk/STLUtil.hh"
#include "FbTk/Select2nd.hh"
#include "FbTk/Compose.hh"

#include <typeinfo>
#include <iterator>
#include <cstring>

using std::string;
using std::list;
using std::endl;

namespace FbTk {

template<>
void FbTk::Resource<FbTk::Container::Alignment>::setDefaultValue() {
    m_value = FbTk::Container::RELATIVE;
}

template<>
string FbTk::Resource<FbTk::Container::Alignment>::getString() const {
    if (m_value == FbTk::Container::LEFT)
        return string("Left");
    if (m_value == FbTk::Container::RIGHT)
        return string("Right");
    if (m_value == FbTk::Container::RELATIVE_SMART)
        return string("RelativeSmart");
    return string("Relative");
}

template<>
void FbTk::Resource<FbTk::Container::Alignment>::setFromString(const char *str) {
    if (strcasecmp(str, "Left") == 0)
        m_value = FbTk::Container::LEFT;
    else if (strcasecmp(str, "Right") == 0)
        m_value = FbTk::Container::RIGHT;
    else if (strcasecmp(str, "Relative") == 0)
        m_value = FbTk::Container::RELATIVE;
    else if (strcasecmp(str, "RelativeSmart") == 0)
        m_value = FbTk::Container::RELATIVE_SMART;
    else
        setDefaultValue();
}

} // end namespace FbTk

namespace {

class ToolbarModeMenuItem : public FbTk::RadioMenuItem {
public:
    ToolbarModeMenuItem(const FbTk::FbString &label, IconbarTool &handler,
                        string mode,
                        FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd), m_handler(handler), m_mode(mode) {
        setCloseOnClick(false);
    }
    bool isSelected() const { return m_handler.mode() == m_mode; }
    void click(int button, int time, unsigned int mods) {
        m_handler.setMode(m_mode);
        FbTk::RadioMenuItem::click(button, time, mods);
    }

private:
    IconbarTool &m_handler;
    string m_mode;
};

class ToolbarAlignMenuItem: public FbTk::RadioMenuItem {
public:
    ToolbarAlignMenuItem(const FbTk::FbString &label, IconbarTool &handler,
                        FbTk::Container::Alignment mode,
                        FbTk::RefCount<FbTk::Command<void> > &cmd):
        FbTk::RadioMenuItem(label, cmd), m_handler(handler), m_mode(mode) {
        setCloseOnClick(false);
    }
    bool isSelected() const { return m_handler.alignment() == m_mode; }
    void click(int button, int time, unsigned int mods) {
        m_handler.setAlignment(m_mode);
        FbTk::RadioMenuItem::click(button, time, mods);
    }

private:
    IconbarTool &m_handler;
    FbTk::Container::Alignment m_mode;
};


enum {
    L_TITLE = 0,
    L_MODE_NONE,
    L_MODE_ICONS,
    L_MODE_NO_ICONS,
    L_MODE_ICONS_WORKSPACE,
    L_MODE_NOICONS_WORKSPACE,
    L_MODE_WORKSPACE,
    L_MODE_ALL,

    L_LEFT,
    L_RELATIVE,
    L_RELATIVE_SMART,
    L_RIGHT,
};

void setupModeMenu(FbTk::Menu &menu, IconbarTool &handler) {

    using namespace FbTk;
    _FB_USES_NLS;

    static const FbString _labels[] = {
        _FB_XTEXT(Toolbar, IconbarMode, "Iconbar Mode", "Menu title - chooses which set of icons are shown in the iconbar"),
        _FB_XTEXT(Toolbar, IconbarModeNone, "None", "No icons are shown in the iconbar"),
        _FB_XTEXT(Toolbar, IconbarModeIcons, "Icons", "Iconified windows from all workspaces are shown"),
        _FB_XTEXT(Toolbar, IconbarModeNoIcons, "NoIcons", "No iconified windows from all workspaces are shown"),
        _FB_XTEXT(Toolbar, IconbarModeWorkspaceIcons, "WorkspaceIcons", "Iconified windows from this workspace are shown"),
        _FB_XTEXT(Toolbar, IconbarModeWorkspaceNoIcons, "WorkspaceNoIcons", "No iconified windows from this workspace are shown"),
        _FB_XTEXT(Toolbar, IconbarModeWorkspace, "Workspace", "Normal and iconified windows from this workspace are shown"),
        _FB_XTEXT(Toolbar, IconbarModeAllWindows, "All Windows", "All windows are shown"),

        _FB_XTEXT(Align, Left, "Left", "Align to the left"),
        _FB_XTEXT(Align, Relative, "Relative", "Align relative to the width"),
        _FB_XTEXT(Align, RelativeSmart, "Relative (Smart)", "Align relative to the width, but let elements vary according to size of title"),
        _FB_XTEXT(Align, Right, "Right", "Align to the right"),
    };

    RefCount<Command<void> > saverc_cmd(new FbCommands::SaveResources());

    menu.setLabel(_labels[L_TITLE]);
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_NONE], handler, "none", saverc_cmd));
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_ICONS], handler, "{static groups} (minimized=yes)", saverc_cmd));
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_NO_ICONS], handler, "{static groups} (minimized=no)", saverc_cmd));
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_ICONS_WORKSPACE], handler, "{static groups} (minimized=yes) (workspace)", saverc_cmd));
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_NOICONS_WORKSPACE], handler, "{static groups} (minimized=no) (workspace)", saverc_cmd));
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_WORKSPACE], handler, "{static groups} (workspace)", saverc_cmd));
    menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_ALL], handler, "{static groups}", saverc_cmd));

    menu.insertItem(new FbTk::MenuSeparator());

    menu.insertItem(new ToolbarAlignMenuItem(_labels[L_LEFT], handler, FbTk::Container::LEFT, saverc_cmd));
    menu.insertItem(new ToolbarAlignMenuItem(_labels[L_RELATIVE], handler, FbTk::Container::RELATIVE, saverc_cmd));
    menu.insertItem(new ToolbarAlignMenuItem(_labels[L_RELATIVE_SMART], handler, FbTk::Container::RELATIVE_SMART, saverc_cmd));
    menu.insertItem(new ToolbarAlignMenuItem(_labels[L_RIGHT], handler, FbTk::Container::RIGHT, saverc_cmd));

    menu.insertItem(new FbTk::MenuSeparator());

    menu.updateMenu();
}

typedef FbTk::RefCount<FbTk::Command<void> > RefCmd;

class ShowMenu: public FbTk::Command<void> {
public:
    explicit ShowMenu(FluxboxWindow &win):m_win(win) { }
    void execute() {
        FbTk::Menu::hideShownMenu();
        // get last button pos
        const XEvent &e = Fluxbox::instance()->lastEvent();
        m_win.popupMenu(e.xbutton.x_root, e.xbutton.y_root);
    }
private:
    FluxboxWindow &m_win;
};

class FocusCommand: public FbTk::Command<void> {
public:
    explicit FocusCommand(Focusable &win): m_win(win) { }
    void execute() {
        FbTk::Menu::hideShownMenu();
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

} // end anonymous namespace

std::string IconbarTool::s_iconifiedDecoration[2];

IconbarTool::IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme,
                         FbTk::ThemeProxy<IconbarTheme> &focused_theme,
                         FbTk::ThemeProxy<IconbarTheme> &unfocused_theme,
                         BScreen &screen, FbTk::Menu &menu):
    ToolbarItem(ToolbarItem::RELATIVE),
    m_screen(screen),
    m_icon_container(parent, false),
    m_theme(theme),
    m_focused_theme(focused_theme),
    m_unfocused_theme(unfocused_theme),
    m_empty_pm( screen.imageControl() ),
    m_winlist(new FocusableList(screen)),
    m_mode("none"),
    m_rc_mode(screen.resourceManager(), "{static groups} (workspace)",
              screen.name() + ".iconbar.mode", screen.altName() + ".Iconbar.Mode"),
    m_rc_alignment(screen.resourceManager(), FbTk::Container::RELATIVE,
                   screen.name() + ".iconbar.alignment", screen.altName() + ".Iconbar.Alignment"),
    m_rc_client_width(screen.resourceManager(), 128,
                   screen.name() + ".iconbar.iconWidth", screen.altName() + ".Iconbar.IconWidth"),
    m_rc_client_padding(screen.resourceManager(), 10,
                   screen.name() + ".iconbar.iconTextPadding", screen.altName() + ".Iconbar.IconTextPadding"),
    m_rc_use_pixmap(screen.resourceManager(), true,
                    screen.name() + ".iconbar.usePixmap", screen.altName() + ".Iconbar.UsePixmap"),
    m_menu(screen.menuTheme(), screen.imageControl(),
           *screen.layerManager().getLayer(ResourceLayer::MENU)),
    m_alpha(255) {

    updateIconifiedPattern();

    // setup mode menu
    setupModeMenu(m_menu, *this);
    _FB_USES_NLS;
    using namespace FbTk;
    // setup use pixmap item to reconfig iconbar and save resource on click
    MacroCommand *save_and_reconfig = new MacroCommand();
    RefCount<Command<void> > reconfig(new SimpleCommand<IconbarTool>(*this, &IconbarTool::renderTheme));
    RefCount<Command<void> > save(FbTk::CommandParser<void>::instance().parse("saverc"));
    save_and_reconfig->add(reconfig);
    save_and_reconfig->add(save);
    RefCount<Command<void> > s_and_reconfig(save_and_reconfig);
    m_menu.insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Toolbar, ShowIcons,
                    "Show Pictures", "chooses if little icons are shown next to title in the iconbar"),
                           m_rc_use_pixmap, s_and_reconfig));
    m_menu.updateMenu();
    // must be internal menu, otherwise toolbar main menu tries to delete it.
    m_menu.setInternalMenu();

    // add iconbar menu to toolbar menu
    menu.insertSubmenu(m_menu.label().logical(), &m_menu);

    // setup signals
    m_tracker.join(theme.reconfigSig(), FbTk::MemFun(*this, &IconbarTool::themeReconfigured));
    m_tracker.join(focused_theme.reconfigSig(),
            FbTk::MemFun(*this, &IconbarTool::themeReconfigured));
    m_tracker.join(unfocused_theme.reconfigSig(),
            FbTk::MemFun(*this, &IconbarTool::themeReconfigured));
    m_tracker.join(screen.reconfigureSig(),
            FbTk::MemFunIgnoreArgs(*this, &IconbarTool::updateIconifiedPattern));

    m_resizeSig_timer.setTimeout(IconButton::updateLaziness());
    m_resizeSig_timer.fireOnce(true);
    FbTk::RefCount<FbTk::Command<void> > ers(new FbTk::SimpleCommand<IconbarTool>(*this, &IconbarTool::emitResizeSig));
    m_resizeSig_timer.setCommand(ers);

    themeReconfigured();
}

IconbarTool::~IconbarTool() {
    deleteIcons();
}

void IconbarTool::move(int x, int y) {
    m_icon_container.move(x, y);
}

void IconbarTool::updateMaxSizes(unsigned int width, unsigned int height) {
    const unsigned int maxsize = (m_icon_container.orientation() & 1) ? height : width;
    m_icon_container.setMaxTotalSize(maxsize);

    if(*m_rc_alignment == FbTk::Container::LEFT || *m_rc_alignment == FbTk::Container::RIGHT) {
        *m_rc_client_width = FbTk::Util::clamp(*m_rc_client_width, 10, 400);
        m_icon_container.setMaxSizePerClient(*m_rc_client_width);
    } else {
        m_icon_container.setMaxSizePerClient(maxsize/std::max(1, m_icon_container.size()));
    }
}

void IconbarTool::resize(unsigned int width, unsigned int height) {
    m_icon_container.resize(width, height);
    updateMaxSizes(width, height);
    renderTheme();
}

void IconbarTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {

    m_icon_container.moveResize(x, y, width, height);
    updateMaxSizes(width, height);
    renderTheme();
}

void IconbarTool::show() {
    m_icon_container.show();
}

void IconbarTool::hide() {
    m_icon_container.hide();
}

void IconbarTool::setAlignment(FbTk::Container::Alignment align) {
    *m_rc_alignment = align;
    update(ALIGN, NULL);
    m_menu.reconfigure();
}

void IconbarTool::setMode(string mode) {
    if (mode == m_mode)
        return;

    *m_rc_mode = m_mode = mode;

    // lock graphics update
    m_icon_container.setUpdateLock(true);

    if (mode == "none")
        m_winlist.reset(new FocusableList(m_screen));
    else
        m_winlist.reset(new FocusableList(m_screen,
                                           mode + " (iconhidden=no)"));
    if (m_winlist.get()) {
        m_winlist->addSig().connect(
                    std::bind1st(FbTk::MemFun(*this, &IconbarTool::update), LIST_ADD)
                );
        m_winlist->removeSig().connect(
                    std::bind1st(FbTk::MemFun(*this, &IconbarTool::update), LIST_REMOVE)
                );
        m_winlist->addSig().connect(
                    std::bind1st(FbTk::MemFun(*this, &IconbarTool::update), LIST_ORDER)
                );
        m_winlist->resetSig().connect(FbTk::MemFunBind(
                        *this, &IconbarTool::update, LIST_RESET, static_cast<Focusable *>(0)
                    ));
    }
    reset();

    m_resizeSig_timer.start();

    // unlock graphics update
    m_icon_container.setUpdateLock(false);
    m_icon_container.update();
    m_icon_container.showSubwindows();

    renderTheme();

    m_menu.reconfigure();
}

void IconbarTool::emitResizeSig() {
    resizeSig().emit();
}

unsigned int IconbarTool::width() const {
    return m_icon_container.width();
}

unsigned int IconbarTool::preferredWidth() const {
    // border and paddings
    unsigned int w = 2*borderWidth() + *m_rc_client_padding * m_icons.size();

    // the buttons
    for (IconMap::const_iterator it = m_icons.begin(), end = m_icons.end(); it != end; ++it) {
        w += it->second->preferredWidth();
    }

    return w;
}

unsigned int IconbarTool::height() const {
    return m_icon_container.height();
}

unsigned int IconbarTool::borderWidth() const {
    return m_icon_container.borderWidth();
}

void IconbarTool::themeReconfigured() {
    setMode(*m_rc_mode);
}

void IconbarTool::update(UpdateReason reason, Focusable *win) {
    // ignore updates if we're shutting down
    if (m_screen.isShuttingdown()) {
        if (!m_icons.empty())
            deleteIcons();
        return;
    }

    m_icon_container.setAlignment(*m_rc_alignment);

    // lock graphic update
    m_icon_container.setUpdateLock(true);

    switch(reason) {
        case LIST_ADD: case LIST_ORDER:
            insertWindow(*win);
            break;
        case LIST_REMOVE:
            removeWindow(*win);
            break;
        case LIST_RESET:
            reset();
            break;
        case ALIGN:
            break;
    }

    m_resizeSig_timer.start();

    updateMaxSizes(width(), height());
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

void IconbarTool::updateIconifiedPattern() {
    FbTk::Resource<std::string> p(m_screen.resourceManager(), "( %t )",
                                  m_screen.name() + ".iconbar.iconifiedPattern",
                                  m_screen.altName() + ".Iconbar.IconifiedPattern");
    size_t tidx = p->find("%t");
    s_iconifiedDecoration[0].clear();
    s_iconifiedDecoration[1].clear();
    if (tidx != std::string::npos) {
        s_iconifiedDecoration[0] = p->substr(0, tidx);
        s_iconifiedDecoration[1] = p->substr(tidx+2);
    }
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
    m_tracker.join(button->titleChanged(), FbTk::MemFun(m_resizeSig_timer, &FbTk::Timer::start));
}

void IconbarTool::reset() {
    deleteIcons();
    updateList();
}

void IconbarTool::updateSizing() {
    m_icon_container.setBorderWidth(m_theme.border().width());
    m_icon_container.setBorderColor(m_theme.border().color());

    FbTk::STLUtil::forAll(m_icons, 
            FbTk::Compose(std::mem_fun(&IconButton::reconfigTheme), 
                FbTk::Select2nd<IconMap::value_type>()));

}

void IconbarTool::renderTheme(int alpha) {

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
    FbTk::STLUtil::destroyAndClearSecond(m_icons);
}

void IconbarTool::removeWindow(Focusable &win) {
    // got window die signal, lets find and remove the window
    IconMap::iterator it = m_icons.find(&win);
    if (it == m_icons.end())
        return;

    fbdbg<<"IconbarTool::"<<__FUNCTION__<<"( 0x"<<&win<<" title = "<<win.title().logical()<<") found!"<<endl;

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

    fbdbg<<"IconbarTool::addWindow(0x"<<&win<<" title = "<<win.title().logical()<<")"<<endl;

    IconButton *button = new IconButton(m_icon_container, m_focused_theme,
                                        m_unfocused_theme, win);

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

