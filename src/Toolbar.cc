// Toolbar.cc for Fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Toolbar.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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

// $Id$

#include "Toolbar.hh"

// tool
#include "ToolbarItem.hh"

// themes
#include "ToolbarTheme.hh"

#include "fluxbox.hh"
#include "Keys.hh"
#include "Screen.hh"
#include "WindowCmd.hh"
#include "IntResMenuItem.hh"
#include "BoolMenuItem.hh"

#ifdef XINERAMA
#include "Xinerama.hh"
#endif // XINERAMA

#include "Strut.hh"
#include "CommandParser.hh"
#include "Layer.hh"

#include "FbTk/I18n.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Transparent.hh"


// use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Shape.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#include <iterator>
#include <typeinfo>

using std::string;
using std::pair;
using std::list;

namespace FbTk {

template<>
void FbTk::Resource<Toolbar::Placement>::
setFromString(const char *strval) {
    if (strcasecmp(strval, "TopLeft")==0)
        m_value = Toolbar::TOPLEFT;
    else if (strcasecmp(strval, "BottomLeft")==0)
        m_value = Toolbar::BOTTOMLEFT;
    else if (strcasecmp(strval, "TopCenter")==0)
        m_value = Toolbar::TOPCENTER;
    else if (strcasecmp(strval, "BottomCenter")==0)
        m_value = Toolbar::BOTTOMCENTER;
    else if (strcasecmp(strval, "TopRight")==0)
        m_value = Toolbar::TOPRIGHT;
    else if (strcasecmp(strval, "BottomRight")==0)
        m_value = Toolbar::BOTTOMRIGHT;
    else if (strcasecmp(strval, "LeftTop") == 0)
        m_value = Toolbar::LEFTTOP;
    else if (strcasecmp(strval, "LeftCenter") == 0)
        m_value = Toolbar::LEFTCENTER;
    else if (strcasecmp(strval, "LeftBottom") == 0)
        m_value = Toolbar::LEFTBOTTOM;
    else if (strcasecmp(strval, "RightTop") == 0)
        m_value = Toolbar::RIGHTTOP;
    else if (strcasecmp(strval, "RightCenter") == 0)
        m_value = Toolbar::RIGHTCENTER;
    else if (strcasecmp(strval, "RightBottom") == 0)
        m_value = Toolbar::RIGHTBOTTOM;
    else
        setDefaultValue();
}

template<>
string FbTk::Resource<Toolbar::Placement>::
getString() const {
    switch (m_value) {
    case Toolbar::TOPLEFT:
        return string("TopLeft");
        break;
    case Toolbar::BOTTOMLEFT:
        return string("BottomLeft");
        break;
    case Toolbar::TOPCENTER:
        return string("TopCenter");
        break;
    case Toolbar::BOTTOMCENTER:
        return string("BottomCenter");
        break;
    case Toolbar::TOPRIGHT:
        return string("TopRight");
        break;
    case Toolbar::BOTTOMRIGHT:
        return string("BottomRight");
        break;
    case Toolbar::LEFTTOP:
        return string("LeftTop");
        break;
    case Toolbar::LEFTCENTER:
        return string("LeftCenter");
        break;
    case Toolbar::LEFTBOTTOM:
        return string("LeftBottom");
        break;
    case Toolbar::RIGHTTOP:
        return string("RightTop");
        break;
    case Toolbar::RIGHTCENTER:
        return string("RightCenter");
        break;
    case Toolbar::RIGHTBOTTOM:
        return string("RightBottom");
        break;
    }
    //default string
    return string("BottomCenter");
}
} // end namespace FbTk

namespace {
class SetToolbarPlacementCmd: public FbTk::Command {
public:
    SetToolbarPlacementCmd(Toolbar &tbar, Toolbar::Placement place):m_tbar(tbar), m_place(place) { }
    void execute() {
        m_tbar.setPlacement(m_place);
        m_tbar.reconfigure();
        Fluxbox::instance()->save_rc();
    }
private:
    Toolbar &m_tbar;
    Toolbar::Placement m_place;
};

}; // end anonymous

// toolbar frame
Toolbar::Frame::Frame(FbTk::EventHandler &evh, int screen_num):
    window(screen_num, // screen (parent)
           0, 0, // pos
           10, 10, // size
           // event mask
           ButtonPressMask | ButtonReleaseMask | ExposureMask |
           EnterWindowMask | LeaveWindowMask | SubstructureNotifyMask,

           true) // override redirect
{

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // add windows to eventmanager
    evm.add(evh, window);

}

Toolbar::Frame::~Frame() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    // remove windows from eventmanager
    evm.remove(window);
}

Toolbar::Toolbar(BScreen &scrn, FbTk::XLayer &layer, size_t width):
    m_hidden(false),
    frame(*this, scrn.screenNumber()),
    m_window_pm(0),
    m_screen(scrn),
    m_layeritem(frame.window, layer),
    m_layermenu(scrn.menuTheme(),
                scrn.imageControl(),
                *scrn.layerManager().getLayer(Layer::MENU),
                this,
                true),
    m_placementmenu(scrn.menuTheme(),
                    scrn.imageControl(),
                    *scrn.layerManager().getLayer(Layer::MENU)),
    m_toolbarmenu(scrn.menuTheme(),
                  scrn.imageControl(),
                  *scrn.layerManager().getLayer(Layer::MENU)),
    m_theme(scrn.screenNumber()),
    m_tool_factory(scrn),
    m_strut(0),
    // lock rcmanager here
    m_rc_auto_hide(scrn.resourceManager().lock(), false,
                   scrn.name() + ".toolbar.autoHide", scrn.altName() + ".Toolbar.AutoHide"),
    m_rc_maximize_over(scrn.resourceManager(), false,
                       scrn.name() + ".toolbar.maxOver", scrn.altName() + ".Toolbar.MaxOver"),
    m_rc_visible(scrn.resourceManager(), true, scrn.name() + ".toolbar.visible", scrn.altName() + ".Toolbar.Visible"),
    m_rc_width_percent(scrn.resourceManager(), 65,
                       scrn.name() + ".toolbar.widthPercent", scrn.altName() + ".Toolbar.WidthPercent"),
    m_rc_alpha(scrn.resourceManager(), 255,
                       scrn.name() + ".toolbar.alpha", scrn.altName() + ".Toolbar.Alpha"),
    m_rc_layernum(scrn.resourceManager(), Layer(Layer::DOCK),
                  scrn.name() + ".toolbar.layer", scrn.altName() + ".Toolbar.Layer"),
    m_rc_on_head(scrn.resourceManager(), 0,
                 scrn.name() + ".toolbar.onhead", scrn.altName() + ".Toolbar.onHead"),
    m_rc_placement(scrn.resourceManager(), Toolbar::BOTTOMCENTER,
                   scrn.name() + ".toolbar.placement", scrn.altName() + ".Toolbar.Placement"),
    m_rc_height(scrn.resourceManager(), 0, scrn.name() + ".toolbar.height", scrn.altName() + ".Toolbar.Height"),
    m_rc_tools(scrn.resourceManager(), "workspacename, prevworkspace, nextworkspace, iconbar, systemtray, prevwindow, nextwindow, clock",
               scrn.name() + ".toolbar.tools", scrn.altName() + ".Toolbar.Tools"),
    m_shape(new Shape(frame.window, 0)),
    m_resize_lock(false) {
    _FB_USES_NLS;
    // we need to get notified when the theme is reloaded
    m_theme.reconfigSig().attach(this);
    // listen to screen size changes
    screen().resizeSig().attach(this);
    screen().reconfigureSig().attach(this); // get this on antialias change

    moveToLayer((*m_rc_layernum).getNum());

    m_layermenu.setLabel(_FB_XTEXT(Toolbar, Layer, "Toolbar Layer", "Title of toolbar layer menu"));
    m_placementmenu.setLabel(_FB_XTEXT(Toolbar, Placement, "Toolbar Placement", "Title of toolbar placement menu"));

    m_layermenu.setInternalMenu();
    m_placementmenu.setInternalMenu();
    m_toolbarmenu.setInternalMenu();
    setupMenus();
    // add menu to screen
    screen().addConfigMenu(_FB_XTEXT(Toolbar, Toolbar, "Toolbar", "title of toolbar menu item"), menu());

    // geometry settings
    frame.width = width;
    frame.height = 10;
    frame.bevel_w = 1;
    frame.grab_x = frame.grab_y = 0;

    // setup hide timer
    m_hide_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay());
    FbTk::RefCount<FbTk::Command> toggle_hidden(new FbTk::SimpleCommand<Toolbar>(*this, &Toolbar::toggleHidden));
    m_hide_timer.setCommand(toggle_hidden);
    m_hide_timer.fireOnce(true);


    // show all windows
    frame.window.showSubwindows();
    //    frame.window.show();

    scrn.resourceManager().unlock();
    // setup to listen to child events
    FbTk::EventManager::instance()->addParent(*this, window());
    Fluxbox::instance()->keys()->registerWindow(window().window(), *this,
                                                Keys::ON_TOOLBAR);
    // get everything together
    reconfigure();
    // this gets done by the screen later as it loads

}

Toolbar::~Toolbar() {
    Fluxbox::instance()->keys()->unregisterWindow(window().window());
    FbTk::EventManager::instance()->remove(window());
    // remove menu items before we delete tools so we dont end up
    // with dangling pointers to old submenu items (internal menus)
    // from the tools
    screen().removeConfigMenu(menu());

    menu().removeAll();

    deleteItems();
    clearStrut();

    if (m_window_pm)
        screen().imageControl().removeImage(m_window_pm);
}

void Toolbar::clearStrut() {
    if (m_strut) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

void Toolbar::updateStrut() {

    bool had_strut = m_strut ? true : false;
    clearStrut();
    // we should request space if we're in autohide mode or
    // if the user dont want to request space for toolbar.
    if (doAutoHide() || *m_rc_maximize_over || ! *m_rc_visible) {
        if (had_strut)
            screen().updateAvailableWorkspaceArea();
        return;
    }

    // request area on screen
    int top = 0, bottom = 0, left = 0, right = 0;
    switch (placement()) {
    case TOPLEFT:
    case TOPCENTER:
    case TOPRIGHT:
        top = height() + 2 * theme().border().width();
        break;
    case BOTTOMLEFT:
    case BOTTOMCENTER:
    case BOTTOMRIGHT:
        bottom = height() + 2 * theme().border().width();
        break;
    case RIGHTTOP:
    case RIGHTCENTER:
    case RIGHTBOTTOM:
        right = width() + 2 * theme().border().width();
        break;
    case LEFTTOP:
    case LEFTCENTER:
    case LEFTBOTTOM:
        left = width() + 2 * theme().border().width();
        break;
    };
    m_strut = screen().requestStrut(getOnHead(), left, right, top, bottom);
    screen().updateAvailableWorkspaceArea();
}

bool Toolbar::isVertical() const {
    return (placement() == RIGHTCENTER ||
            placement() == RIGHTTOP ||
            placement() == RIGHTBOTTOM ||
            placement() == LEFTCENTER ||
            placement() == LEFTTOP ||
            placement() == LEFTBOTTOM);
}


void Toolbar::raise() {
    m_layeritem.raise();
}

void Toolbar::lower() {
    m_layeritem.lower();
}

void Toolbar::reconfigure() {
    // wait until after windows are drawn to show toolbar at startup
    // otherwise, it looks ugly
    if (!Fluxbox::instance()->isStartup())
        updateVisibleState();

    if (!doAutoHide() && isHidden())
        toggleHidden();

    m_tool_factory.updateThemes();

    // parse resource tools and determine if we need to rebuild toolbar

    bool need_update = false;
    // parse and transform to lower case
    list<string> tools;
    FbTk::StringUtil::stringtok(tools, *m_rc_tools, ", ");
    transform(tools.begin(),
              tools.end(),
              tools.begin(),
              FbTk::StringUtil::toLower);

    if (!tools.empty() && tools.size() == m_tools.size()) {
        StringList::const_iterator tool_it = tools.begin();
        StringList::const_iterator current_tool_it = m_tools.begin();
        StringList::const_iterator tool_it_end = tools.end();
        for (; tool_it != tool_it_end; ++tool_it, ++current_tool_it) {
            if (*current_tool_it != *tool_it)
                break;
        }
        // did we find anything that wasn't in the right place or new item?
        if (tool_it != tool_it_end)
            need_update = true;
    } else // sizes does not match so we update
        need_update = true;

    if (need_update) {

        // destroy tools and rebuild them
        deleteItems();
        // they will be readded later
        menu().removeAll();
        setupMenus(true); // rebuild menu but skip rebuild of placement menu

        m_tools = tools; // copy values

        if (!m_tools.empty()) {
            // make lower case
            transform(m_tools.begin(), m_tools.end(),
                      m_tools.begin(),
                      FbTk::StringUtil::toLower);

            // create items
            StringList::const_iterator item_it = m_tools.begin();
            StringList::const_iterator item_it_end = m_tools.end();
            for (; item_it != item_it_end; ++item_it) {
                ToolbarItem *item = m_tool_factory.create(*item_it, frame.window, *this);
                if (item == 0)
                    continue;
                m_item_list.push_back(item);
                item->resizeSig().attach(this);

            }
            // show all items
            frame.window.showSubwindows();
        }

    } else { // just update the menu
        menu().reconfigure();
    }

    if (doAutoHide())
        m_hide_timer.start();

    frame.bevel_w = theme().bevelWidth();
    // destroy shape if the theme wasn't specified with one,
    // or create one
    if (theme().shape() == false && m_shape.get())
        m_shape.reset(0);
    else if (theme().shape() && m_shape.get() == 0) {
        m_shape.reset(new Shape(frame.window, 0));
    }

    // recalibrate size
    setPlacement(placement());

    if (isHidden()) {
        frame.window.moveResize(frame.x_hidden, frame.y_hidden,
                                frame.width, frame.height);
    } else {
        frame.window.moveResize(frame.x, frame.y,
                                frame.width, frame.height);
    }

    // render frame window
    Pixmap tmp = m_window_pm;
    if (!theme().toolbar().usePixmap()) {
        m_window_pm = 0;
        frame.window.setBackgroundColor(theme().toolbar().color());
    } else {
        FbTk::Orientation orient = FbTk::ROT0;
        Toolbar::Placement where = *m_rc_placement;
        if (where == LEFTCENTER || where == LEFTTOP || where == LEFTBOTTOM)
            orient = FbTk::ROT270;
        if (where == RIGHTCENTER || where == RIGHTTOP || where == RIGHTBOTTOM)
            orient = FbTk::ROT90;

        m_window_pm = screen().imageControl().renderImage(
                          frame.window.width(), frame.window.height(),
                          theme().toolbar(), orient);
        frame.window.setBackgroundPixmap(m_window_pm);
    }
    if (tmp)
        screen().imageControl().removeImage(tmp);

    frame.window.setBorderColor(theme().border().color());
    frame.window.setBorderWidth(theme().border().width());

    bool have_composite = FbTk::Transparent::haveComposite();
    // have_composite could have changed, so we need to change both
    if (have_composite) {
        frame.window.setOpaque(alpha());
        frame.window.setAlpha(255);
    } else {
        frame.window.setOpaque(255);
        frame.window.setAlpha(alpha());
    }
    frame.window.clear();

    if (theme().shape() && m_shape.get())
        m_shape->update();

    ItemList::iterator item_it = m_item_list.begin();
    ItemList::iterator item_it_end = m_item_list.end();
    for (; item_it != item_it_end; ++item_it) {
        (*item_it)->updateSizing();
    }

    rearrangeItems();

    for (item_it = m_item_list.begin(); item_it != item_it_end; ++item_it) {
        (*item_it)->renderTheme(alpha());
    }


    // we're done with all resizing and stuff now we can request a new
    // area to be reserved on screen
    updateStrut();

}



void Toolbar::buttonPressEvent(XButtonEvent &be) {
    WindowCmd<void>::setWindow(0);
    if (Fluxbox::instance()->keys()->doAction(be.type, be.state, be.button,
                                              Keys::ON_TOOLBAR, be.time))
        return;
    if (be.button == 1)
        raise();
    if (be.button != 3)
        return;

    screen().hideMenus();

    if (! menu().isVisible()) {

        int head = screen().getHead(be.x_root, be.y_root);
        int borderw = menu().fbwindow().borderWidth();
        pair<int, int> m = screen().clampToHead(head,
                                                be.x_root - (menu().width() / 2),
                                                be.y_root - (menu().titleWindow().height() / 2),
                                                menu().width() + 2*borderw,
                                                menu().height() + 2*borderw);

        menu().setScreen(screen().getHeadX(head),
                         screen().getHeadY(head),
                         screen().getHeadWidth(head),
                         screen().getHeadHeight(head));
        menu().move(m.first, m.second);
        menu().show();
        menu().grabInputFocus();
    } else
        menu().hide();

}

void Toolbar::enterNotifyEvent(XCrossingEvent &ce) {
    WindowCmd<void>::setWindow(0);
    Fluxbox::instance()->keys()->doAction(ce.type, ce.state, 0,
                                          Keys::ON_TOOLBAR);
    if (! doAutoHide()) {
        if (isHidden())
            toggleHidden();
        return;
    }

    if (isHidden()) {
        if (! m_hide_timer.isTiming())
            m_hide_timer.start();
    } else {
        if (m_hide_timer.isTiming())
            m_hide_timer.stop();
    }
}

void Toolbar::leaveNotifyEvent(XCrossingEvent &event) {
    // still inside?
    if (event.x_root > x() && event.x_root <= (int)(x() + width()) &&
        event.y_root > y() && event.y_root <= (int)(y() + height()))
        return;

    Fluxbox::instance()->keys()->doAction(event.type, event.state, 0,
                                          Keys::ON_TOOLBAR);
    if (! doAutoHide())
        return;

    if (isHidden()) {
        if (m_hide_timer.isTiming())
            m_hide_timer.stop();
    } else if (! menu().isVisible() && ! m_hide_timer.isTiming())
        m_hide_timer.start();

}


void Toolbar::exposeEvent(XExposeEvent &ee) {
    if (ee.window == frame.window) {
        frame.window.clearArea(ee.x, ee.y,
                               ee.width, ee.height);
    }
}


void Toolbar::handleEvent(XEvent &event) {
    /* Commented out by Simon 16jun04, since it causes LOTS of rearrangeItems
       particularly on startup. This was needed to resize when tool changes its own
       size, but it has too many side effects. Use the resizeSig in ToolbarItem instead.

    if (event.type == ConfigureNotify &&
        event.xconfigure.window != window().window()) {
        rearrangeItems();
    }
*/
}

void Toolbar::update(FbTk::Subject *subj) {

    // either screen reconfigured, theme was reloaded
    // or a tool resized itself

    if (typeid(*subj) == typeid(ToolbarItem::ToolbarItemSubject))
        rearrangeItems();
    else
        reconfigure();

}

void Toolbar::setPlacement(Toolbar::Placement where) {
    // disable vertical toolbar

    *m_rc_placement = where;
    int head_x = 0,
        head_y = 0,
        head_w = screen().width(),
        head_h = screen().height();

    if (screen().hasXinerama()) {
        int head = *m_rc_on_head;
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    }

    int bevel_width = theme().bevelWidth();
    int border_width = theme().border().width();

    frame.width = (head_w - 2*border_width) * (*m_rc_width_percent) / 100;
    //!! TODO: change this
    // max height of each toolbar items font...
    unsigned int max_height = m_tool_factory.maxFontHeight() + 2;

    if (theme().height() > 0)
        max_height = theme().height();

    if (*m_rc_height > 0 && *m_rc_height < 100)
        max_height = *m_rc_height;

    frame.height = max_height;

    frame.height += (frame.bevel_w * 2);

    // should we flipp sizes?
    if (isVertical()) {
        frame.width = frame.height;
        frame.height = head_h * (*m_rc_width_percent) / 100;

    } // else horizontal toolbar


    // So we get at least one pixel visible in hidden mode
    if (bevel_width <= border_width)
        bevel_width = border_width + 1;

    FbTk::Orientation orient = FbTk::ROT0;

    switch (where) {
    case TOPLEFT:
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;

    case BOTTOMLEFT:
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = head_x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;

    case TOPCENTER:
        frame.x = head_x + (head_w - frame.width) / 2 - border_width;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;
    case TOPRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + bevel_width - border_width - frame.height;
        if (m_shape.get())
            m_shape->setPlaces(Shape::BOTTOMRIGHT | Shape::BOTTOMLEFT);
        break;

    case BOTTOMRIGHT:
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;

    case BOTTOMCENTER: // default is BOTTOMCENTER
        frame.x = head_x + (head_w - frame.width) / 2 - border_width;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x;
        frame.y_hidden = head_y + head_h - bevel_width - border_width;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::TOPLEFT);
        break;
    case LEFTCENTER:
        orient = FbTk::ROT270;
        frame.x = head_x;
        frame.y = head_y + (head_h - frame.height)/2 - border_width;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case LEFTTOP:
        orient = FbTk::ROT270;
        frame.x = head_x;
        frame.y = head_y;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case LEFTBOTTOM:
        orient = FbTk::ROT270;
        frame.x = head_x;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x - frame.width + bevel_width + border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPRIGHT | Shape::BOTTOMRIGHT);
        break;
    case RIGHTCENTER:
        orient = FbTk::ROT90;
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + (head_h - frame.height)/2 - border_width;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    case RIGHTTOP:
        orient = FbTk::ROT90;
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    case RIGHTBOTTOM:
        orient = FbTk::ROT90;
        frame.x = head_x + head_w - frame.width - border_width*2;
        frame.y = head_y + head_h - frame.height - border_width*2;
        frame.x_hidden = frame.x + frame.width - bevel_width - border_width;
        frame.y_hidden = frame.y;
        if (m_shape.get())
            m_shape->setPlaces(Shape::TOPLEFT | Shape::BOTTOMLEFT);
        break;
    }

    ItemList::iterator item_it = m_item_list.begin();
    ItemList::iterator item_it_end = m_item_list.end();
    for (; item_it != item_it_end; ++item_it)
        (*item_it)->setOrientation(orient);
}

void Toolbar::updateVisibleState() {
    *m_rc_visible ? frame.window.show() : frame.window.hide();
}

void Toolbar::toggleHidden() {

    // toggle hidden
    m_hidden = ! m_hidden;
    if (isHidden())
        frame.window.move(frame.x_hidden, frame.y_hidden);
    else {
        frame.window.move(frame.x, frame.y);
        ItemList::iterator item_it = m_item_list.begin();
        ItemList::iterator item_it_end = m_item_list.end();
        for ( ; item_it != item_it_end; ++item_it)
            (*item_it)->parentMoved();
    }

}

void Toolbar::moveToLayer(int layernum) {
    m_layeritem.moveToLayer(layernum);
    *m_rc_layernum = layernum;
}

void Toolbar::setupMenus(bool skip_new_placement) {
    _FB_USES_NLS;
    using namespace FbTk;

    typedef RefCount<Command> RefCommand;
    typedef SimpleCommand<Toolbar> ToolbarCommand;

    menu().setLabel(_FB_XTEXT(Toolbar, Toolbar,
                              "Toolbar", "Title of Toolbar menu"));

    RefCommand reconfig_toolbar(new ToolbarCommand(*this, &Toolbar::reconfigure));
    RefCommand save_resources(CommandParser::instance().parseLine("saverc"));
    MacroCommand *toolbar_menuitem_macro = new MacroCommand();
    toolbar_menuitem_macro->add(reconfig_toolbar);
    toolbar_menuitem_macro->add(save_resources);
    RefCommand reconfig_toolbar_and_save_resource(toolbar_menuitem_macro);

    MacroCommand *visible_macro = new MacroCommand();
    RefCommand toggle_visible(new ToolbarCommand(*this, &Toolbar::updateVisibleState));
    visible_macro->add(toggle_visible);
    visible_macro->add(reconfig_toolbar);
    visible_macro->add(save_resources);
    RefCommand toggle_visible_cmd(visible_macro);
    menu().insert(new BoolMenuItem(_FB_XTEXT(Common, Visible,
                                             "Visible", "Whether this item is visible"),
                                   *m_rc_visible, toggle_visible_cmd));

    menu().insert(new BoolMenuItem(_FB_XTEXT(Common, AutoHide,
                                             "Auto hide", "Toggle auto hide of toolbar"),
                                   *m_rc_auto_hide,
                                   reconfig_toolbar_and_save_resource));

    MenuItem *toolbar_menuitem =
        new IntResMenuItem< FbTk::Resource<int> >(_FB_XTEXT(Toolbar, WidthPercent,
                                     "Toolbar width percent",
                                     "Percentage of screen width taken by toolbar"),
                           m_rc_width_percent,
                           0, 100, menu()); // min/max value


    toolbar_menuitem->setCommand(reconfig_toolbar_and_save_resource);
    menu().insert(toolbar_menuitem);

    menu().insert(new BoolMenuItem(_FB_XTEXT(Common, MaximizeOver,
                                             "Maximize Over",
                                             "Maximize over this thing when maximizing"),
                                   *m_rc_maximize_over,
                                   reconfig_toolbar_and_save_resource));
    menu().insert(_FB_XTEXT(Menu, Layer, "Layer...", "Title of Layer menu"), &layerMenu());
#ifdef XINERAMA
    if (screen().hasXinerama()) {
        menu().insert(_FB_XTEXT(Menu, OnHead, "On Head...", "Title of On Head menu"),
                      new XineramaHeadMenu<Toolbar>(screen().menuTheme(),
                                                    screen(),
                                                    screen().imageControl(),
                                                    *screen().layerManager().getLayer(::Layer::MENU),
                                                    *this,
                                                    _FB_XTEXT(Toolbar, OnHead, "Toolbar on Head",
                                                              "Title of toolbar on head menu")));
    }
#endif // XINERAMA


    // menu is 3 wide, 5 down
    if (!skip_new_placement) {
        typedef pair<FbTk::FbString, Toolbar::Placement> PlacementP;
        typedef list<PlacementP> Placements;
        Placements place_menu;

        place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), Toolbar::TOPLEFT));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), Toolbar::LEFTTOP));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), Toolbar::LEFTCENTER));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), Toolbar::LEFTBOTTOM));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), Toolbar::BOTTOMLEFT));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), Toolbar::TOPCENTER));
        place_menu.push_back(PlacementP("", Toolbar::TOPLEFT));
        place_menu.push_back(PlacementP("", Toolbar::TOPLEFT));
        place_menu.push_back(PlacementP("", Toolbar::TOPLEFT));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), Toolbar::BOTTOMCENTER));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, TopRight, "Top Right", "Top Right"), Toolbar::TOPRIGHT));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightTop, "Right Top", "Right Top"), Toolbar::RIGHTTOP));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), Toolbar::RIGHTCENTER));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), Toolbar::RIGHTBOTTOM));
        place_menu.push_back(PlacementP(_FB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), Toolbar::BOTTOMRIGHT));


        placementMenu().setMinimumSublevels(3);
        // create items in sub menu
        for (size_t i=0; i<15; ++i) {
            FbTk::FbString &str = place_menu.front().first;
            Toolbar::Placement placement = place_menu.front().second;

            if (str == "") {
                placementMenu().insert("");
                placementMenu().setItemEnabled(i, false);
            } else {
                RefCommand setplace(new SetToolbarPlacementCmd(*this, placement));
                placementMenu().insert(str, setplace);

            }
            place_menu.pop_front();
        }
    }

    menu().insert(_FB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu"), &placementMenu());
    placementMenu().updateMenu();


    // this saves resources and clears the slit window to update alpha value
    FbTk::MenuItem *alpha_menuitem =
        new IntResMenuItem< FbTk::Resource<int> >(_FB_XTEXT(Common, Alpha, "Alpha", "Transparency level"),
                           m_rc_alpha,
                           0, 255, menu());
    // setup command for alpha value
    MacroCommand *alpha_macrocmd = new MacroCommand();
    RefCount<Command> alpha_cmd(new SimpleCommand<Toolbar>(*this, &Toolbar::updateAlpha));
    alpha_macrocmd->add(save_resources);
    alpha_macrocmd->add(alpha_cmd);
    RefCount<Command> set_alpha_cmd(alpha_macrocmd);
    alpha_menuitem->setCommand(set_alpha_cmd);

    menu().insert(alpha_menuitem);
    menu().updateMenu();
}

void Toolbar::saveOnHead(int head) {
    m_rc_on_head = head;
    reconfigure();
}

/*
 * Place items next to each other, with a bevel width between,
 * above and below each item. BUT, if there is no bevel width, then
 * borders should be merged for evenness.
 */

void Toolbar::rearrangeItems() {
    if (m_resize_lock || screen().isShuttingdown() ||
        m_item_list.empty())
        return;

    FbTk::Orientation orient = FbTk::ROT0;
    switch (placement()) {
    case LEFTTOP:
    case LEFTCENTER:
    case LEFTBOTTOM:
        orient = FbTk::ROT270;
        break;
    case RIGHTTOP:
    case RIGHTCENTER:
    case RIGHTBOTTOM:
        orient = FbTk::ROT90;
        break;
    default:
        orient = FbTk::ROT0;
    }

    // lock this
    m_resize_lock = true;
    // calculate size for fixed items
    ItemList::iterator item_it = m_item_list.begin();
    ItemList::iterator item_it_end = m_item_list.end();
    int bevel_width = theme().bevelWidth();
    int fixed_width = bevel_width; // combined size of all fixed items
    int relative_items = 0;
    int last_bw = 0; // we show the largest border of adjoining items
    bool first = true;

    unsigned int width = this->width(), height = this->height();
    unsigned int tmpw, tmph;
    FbTk::translateSize(orient, width, height);

    for (; item_it != item_it_end; ++item_it) {
        if (!(*item_it)->active())
            continue;

        int borderW = (*item_it)->borderWidth();

        if (bevel_width > 0) {
            // the bevel and border are fixed whether relative or not
            fixed_width += bevel_width + 2*borderW;
        } else {
            if (!first) {
                if (borderW > last_bw)
                    fixed_width += borderW;
                else
                    fixed_width += last_bw;
            } else {
                first = false;
            }
        }

        last_bw = borderW;

        tmpw = (*item_it)->width();
        tmph = (*item_it)->height();
        FbTk::translateSize(orient, tmpw, tmph);

        if ((*item_it)->type() == ToolbarItem::FIXED) {
            fixed_width += tmpw;
        } else if ((*item_it)->type() == ToolbarItem::SQUARE) {
            fixed_width += height;
            if (bevel_width)
                fixed_width -= 2*(borderW + bevel_width);
        } else {
            relative_items++;
        }
    }

    // calculate what's going to be left over to the relative sized items
    int relative_width = 0;
    int rounding_error = 0;
    if (relative_items == 0)
        relative_width = 0;
    else { // size left after fixed items / number of relative items
        relative_width = (width - fixed_width) / relative_items;
        rounding_error = width - fixed_width - relative_items * relative_width;
    }

    // now move and resize the items
    // borderWidth added back on straight away
    int next_x = -m_item_list.front()->borderWidth(); // list isn't empty
    if (bevel_width != 0)
        next_x = 0;

    last_bw = 0;
    for (item_it = m_item_list.begin(); item_it != item_it_end; ++item_it) {
        int borderW = (*item_it)->borderWidth();
        if (!(*item_it)->active()) {
            (*item_it)->hide();
            // make sure it still gets told the toolbar height
            tmpw = 1; tmph = height - 2*(bevel_width+borderW);
            if (tmph >= (1<<30)) tmph = 1;
            FbTk::translateSize(orient, tmpw, tmph);
            (*item_it)->resize(tmpw, tmph);  // width of 0 changes to 1 anyway
            continue;
        }
        int offset = bevel_width;
        int size_offset = 2*(borderW + bevel_width);

        if (bevel_width == 0) {
            offset = -borderW;
            size_offset = 0;
            if (borderW > last_bw)
                next_x += borderW;
            else
                next_x += last_bw;
        }
        last_bw = borderW;

        int tmpx = next_x + offset,
            tmpy = offset;

        if ((*item_it)->type() == ToolbarItem::RELATIVE) {
            int extra = 0;
            if (rounding_error != 0) { // distribute rounding error over all relatives
                extra = 1;
                --rounding_error;
            }
            tmpw = extra + relative_width;
            tmph = height - size_offset;
        } else if ((*item_it)->type() == ToolbarItem::SQUARE) {
            tmpw = tmph = height - size_offset;
        } else { // fixed size
            unsigned int itemw = (*item_it)->width(), itemh = (*item_it)->height();
            FbTk::translateSize(orient, itemw, itemh);
            tmpw = itemw;
            tmph = height - size_offset;
        }
        if (tmpw >= (1<<30)) tmpw = 1;
        if (tmph >= (1<<30)) tmph = 1;
        next_x += tmpw + bevel_width;
        if (bevel_width != 0)
            next_x += 2*borderW;

        FbTk::translateCoords(orient, tmpx, tmpy, width, height);
        FbTk::translatePosition(orient, tmpx, tmpy, tmpw, tmph, borderW);
        FbTk::translateSize(orient, tmpw, tmph);
        (*item_it)->moveResize(tmpx, tmpy, tmpw, tmph);
        (*item_it)->show();

    }
    // unlock
    m_resize_lock = false;
    frame.window.clear();
}

void Toolbar::deleteItems() {
    while (!m_item_list.empty()) {
        delete m_item_list.back();
        m_item_list.pop_back();
    }
    m_tools.clear();
}

void Toolbar::updateAlpha() {
    // called when the alpha resource is changed
    if (FbTk::Transparent::haveComposite()) {
        frame.window.setOpaque(*m_rc_alpha);
    } else {
        frame.window.setAlpha(*m_rc_alpha);
        frame.window.updateBackground(false);
        frame.window.clear();

        ItemList::iterator item_it = m_item_list.begin();
        ItemList::iterator item_it_end = m_item_list.end();
        for (item_it = m_item_list.begin(); item_it != item_it_end; ++item_it) {
            (*item_it)->renderTheme(alpha());
        }
    }
}
