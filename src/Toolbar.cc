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

#include "Toolbar.hh"

#include "ToolbarItem.hh"
#include "ToolbarTheme.hh"

#include "fluxbox.hh"
#include "Keys.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "SystemTray.hh"
#include "WindowCmd.hh"

#include "Strut.hh"
#include "Layer.hh"

#include "FbTk/CommandParser.hh"
#include "FbTk/I18n.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/TextUtils.hh"
#include "FbTk/MacroCommand.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/BoolMenuItem.hh"
#include "FbTk/IntMenuItem.hh"
#include "FbTk/Shape.hh"
#include "FbTk/MemFun.hh"
#include "FbTk/STLUtil.hh"
#include "FbTk/Util.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#include <iterator>
#include <typeinfo>
#include <functional>
#include <algorithm>

using std::string;
using std::pair;
using std::list;

using FbTk::STLUtil::forAll;

using namespace std::placeholders;

namespace {

const struct { 
    Toolbar::Placement placement;
    const char* str;
    FbTk::Orientation orient;
    unsigned int shape;
} _values[] = {
    { /* unused */ },
    { Toolbar::TOPLEFT,      "TopLeft",      FbTk::ROT0,   FbTk::Shape::BOTTOMRIGHT                            },
    { Toolbar::TOPCENTER,    "TopCenter",    FbTk::ROT0,   FbTk::Shape::BOTTOMRIGHT | FbTk::Shape::BOTTOMLEFT  },
    { Toolbar::TOPRIGHT,     "TopRight",     FbTk::ROT0,   FbTk::Shape::BOTTOMLEFT                             },
    { Toolbar::BOTTOMLEFT,   "BottomLeft",   FbTk::ROT0,   FbTk::Shape::TOPRIGHT                               },
    { Toolbar::BOTTOMCENTER, "BottomCenter", FbTk::ROT0,   FbTk::Shape::TOPRIGHT    | FbTk::Shape::TOPLEFT     },
    { Toolbar::BOTTOMRIGHT,  "BottomRight",  FbTk::ROT0,   FbTk::Shape::TOPLEFT                                },
    { Toolbar::LEFTBOTTOM,   "LeftBottom",   FbTk::ROT270, FbTk::Shape::TOPRIGHT                               },
    { Toolbar::LEFTCENTER,   "LeftCenter",   FbTk::ROT270, FbTk::Shape::TOPRIGHT    | FbTk::Shape::BOTTOMRIGHT },
    { Toolbar::LEFTTOP,      "LeftTop",      FbTk::ROT270, FbTk::Shape::BOTTOMRIGHT                            },
    { Toolbar::RIGHTBOTTOM,  "RightBottom",  FbTk::ROT90,  FbTk::Shape::TOPLEFT                                },
    { Toolbar::RIGHTCENTER,  "RightCenter",  FbTk::ROT90,  FbTk::Shape::TOPLEFT     | FbTk::Shape::BOTTOMLEFT  },
    { Toolbar::RIGHTTOP,     "RightTop",     FbTk::ROT90,  FbTk::Shape::BOTTOMLEFT                             },
};

}

namespace FbTk {

template<>
string FbTk::Resource<Toolbar::Placement>::
getString() const {

    size_t i = (m_value == FbTk::Util::clamp(m_value, Toolbar::TOPLEFT, Toolbar::RIGHTTOP)
                ? m_value
                : Toolbar::DEFAULT);
    return _values[i].str;
}

template<>
void FbTk::Resource<Toolbar::Placement>::
setFromString(const char *strval) {
    size_t i;
    for (i = 1; i < sizeof(_values)/sizeof(_values[0]); ++i) {
        if (strcasecmp(strval, _values[i].str) == 0) {
            m_value = _values[i].placement;
            return;
        }
    }
    setDefaultValue();
}

} // end namespace FbTk

namespace {

class PlaceToolbarMenuItem: public FbTk::RadioMenuItem {
public:
    PlaceToolbarMenuItem(const FbTk::FbString &label, Toolbar &toolbar,
        Toolbar::Placement place):
        FbTk::RadioMenuItem(label), m_toolbar(toolbar), m_place(place) {
        setCloseOnClick(false);
    }
    bool isSelected() const { return m_toolbar.placement() == m_place; }
    void click(int button, int time, unsigned int mods) {
        m_toolbar.setPlacement(m_place);
        m_toolbar.reconfigure();
        m_toolbar.placementMenu().reconfigure();
        Fluxbox::instance()->save_rc();
    }
private:
    Toolbar &m_toolbar;
    Toolbar::Placement m_place;
};

} // end anonymous

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
    evm.add(evh, window);

}

Toolbar::Frame::~Frame() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(window);
}

Toolbar::Toolbar(BScreen &scrn, FbTk::Layer &layer, size_t width):
    m_hidden(false),
    frame(*this, scrn.screenNumber()),
    m_window_pm(0),
    m_screen(scrn),
    m_layeritem(frame.window, layer),
    m_layermenu(scrn.menuTheme(),
                scrn.imageControl(),
                *scrn.layerManager().getLayer(ResourceLayer::MENU),
                this,
                true),
    m_placementmenu(scrn.menuTheme(),
                    scrn.imageControl(),
                    *scrn.layerManager().getLayer(ResourceLayer::MENU)),
    m_toolbarmenu(scrn.menuTheme(),
                  scrn.imageControl(),
                  *scrn.layerManager().getLayer(ResourceLayer::MENU)),
#ifdef XINERAMA
    m_xineramaheadmenu(0),
#endif // XINERAMA
    m_theme(scrn.screenNumber()),
    m_tool_factory(scrn),
    m_strut(0),
    // lock rcmanager here
    m_rc_auto_hide(scrn.resourceManager().lock(), false,
                   scrn.name() + ".toolbar.autoHide", scrn.altName() + ".Toolbar.AutoHide"),
    m_rc_auto_raise(scrn.resourceManager().lock(), false,
                   scrn.name() + ".toolbar.autoRaise", scrn.altName() + ".Toolbar.AutoRaise"),
    m_rc_maximize_over(scrn.resourceManager(), false,
                       scrn.name() + ".toolbar.maxOver", scrn.altName() + ".Toolbar.MaxOver"),
    m_rc_visible(scrn.resourceManager(), true, scrn.name() + ".toolbar.visible", scrn.altName() + ".Toolbar.Visible"),
    m_rc_width_percent(scrn.resourceManager(), 100,
                       scrn.name() + ".toolbar.widthPercent", scrn.altName() + ".Toolbar.WidthPercent"),
    m_rc_alpha(scrn.resourceManager(), 255,
                       scrn.name() + ".toolbar.alpha", scrn.altName() + ".Toolbar.Alpha"),
    m_rc_layernum(scrn.resourceManager(), ResourceLayer(ResourceLayer::DOCK),
                  scrn.name() + ".toolbar.layer", scrn.altName() + ".Toolbar.Layer"),
    m_rc_on_head(scrn.resourceManager(), 1,
                 scrn.name() + ".toolbar.onhead", scrn.altName() + ".Toolbar.onHead"),
    m_rc_placement(scrn.resourceManager(), Toolbar::BOTTOMCENTER,
                   scrn.name() + ".toolbar.placement", scrn.altName() + ".Toolbar.Placement"),
    m_rc_height(scrn.resourceManager(), 0, scrn.name() + ".toolbar.height", scrn.altName() + ".Toolbar.Height"),
    m_rc_tools(scrn.resourceManager(), "prevworkspace, workspacename, nextworkspace, iconbar, systemtray, clock",
               scrn.name() + ".toolbar.tools", scrn.altName() + ".Toolbar.Tools"),
    m_shape(new FbTk::Shape(frame.window, 0)),
    m_resize_lock(false) {
    _FB_USES_NLS;

    frame.window.setWindowRole("fluxbox-toolbar");

    // get this on antialias change
    m_signal_tracker.join(screen().reconfigureSig(),
            FbTk::MemFunIgnoreArgs(*this, &Toolbar::reconfigure));

    // we need to get notified when the theme is reloaded
    m_signal_tracker.join(m_theme.reconfigSig(), FbTk::MemFun(*this, &Toolbar::reconfigure));

    // listen to screen size changes
    m_signal_tracker.join(screen().resizeSig(),
                          FbTk::MemFun(*this, &Toolbar::screenChanged));


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
    m_hide_timer.setTimeout(Fluxbox::instance()->getAutoRaiseDelay() * FbTk::FbTime::IN_MILLISECONDS);
    FbTk::RefCount<FbTk::Command<void> > ucs(new FbTk::SimpleCommand<Toolbar>(*this, &Toolbar::updateCrossingState));
    m_hide_timer.setCommand(ucs);
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

    Keys* keys = Fluxbox::instance()->keys();
    if (keys)
        keys->unregisterWindow(window().window());

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
    int w = static_cast<int>(width());
    int h = static_cast<int>(height());
    int bw = theme()->border().width();
    int top = 0, bottom = 0, left = 0, right = 0;
    switch (placement()) {
    case TOPLEFT:
    case TOPCENTER:
    case TOPRIGHT:
        top = h + 2 * bw;
        break;
    case BOTTOMLEFT:
    case BOTTOMCENTER:
    case BOTTOMRIGHT:
        bottom = h + 2 * bw;
        break;
    case RIGHTTOP:
    case RIGHTCENTER:
    case RIGHTBOTTOM:
        right = w + 2 * bw;
        break;
    case LEFTTOP:
    case LEFTCENTER:
    case LEFTBOTTOM:
        left = w + 2 * bw;
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

void Toolbar::screenChanged(BScreen &screen) {
    reconfigure();
}

void Toolbar::relayout() {
    forAll(m_item_list, std::mem_fn(&ToolbarItem::updateSizing));
    rearrangeItems();
    forAll(m_item_list, std::bind(std::mem_fn(&ToolbarItem::renderTheme), _1, alpha()));
}

void Toolbar::reconfigure() {

    updateVisibleState();

    if (doAutoHide() && !isHidden() && !m_hide_timer.isTiming())
        m_hide_timer.start();
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
        screen().clearToolButtonMap();
        // they will be readded later
        menu().removeAll();
        setupMenus(true); // rebuild menu but skip rebuild of placement menu

        m_tools = tools; // copy values

        if (!m_tools.empty()) {

            // create items
            StringList::const_iterator item_it = m_tools.begin();
            StringList::const_iterator item_it_end = m_tools.end();
            for (; item_it != item_it_end; ++item_it) {
                ToolbarItem *item = m_tool_factory.create(*item_it, frame.window, *this);
                if (item == 0)
                    continue;
                m_item_list.push_back(item);
                m_signal_tracker.join(item->resizeSig(),
                        FbTk::MemFun(*this, &Toolbar::rearrangeItems));
            }
            // show all items
            frame.window.showSubwindows();
        }

    } else { // just update the menu
        menu().reconfigure();
    }

    frame.bevel_w = theme()->bevelWidth();
    // destroy shape if the theme wasn't specified with one,
    // or create one
    if (theme()->shape() == false && m_shape.get())
        m_shape.reset(0);
    else if (theme()->shape() && m_shape.get() == 0) {
        m_shape.reset(new FbTk::Shape(frame.window, 0));
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
    if (!theme()->toolbar().usePixmap()) {
        m_window_pm = 0;
        frame.window.setBackgroundColor(theme()->toolbar().color());
    } else {
        FbTk::Orientation orient = FbTk::ROT0;
        Toolbar::Placement where = *m_rc_placement;
        if (where == LEFTCENTER || where == LEFTTOP || where == LEFTBOTTOM)
            orient = FbTk::ROT270;
        if (where == RIGHTCENTER || where == RIGHTTOP || where == RIGHTBOTTOM)
            orient = FbTk::ROT90;

        m_window_pm = screen().imageControl().renderImage(
                          frame.window.width(), frame.window.height(),
                          theme()->toolbar(), orient);
        frame.window.setBackgroundPixmap(m_window_pm);
    }
    if (tmp)
        screen().imageControl().removeImage(tmp);

    frame.window.setBorderColor(theme()->border().color());
    frame.window.setBorderWidth(theme()->border().width());

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

    if (theme()->shape() && m_shape.get())
        m_shape->update();

    relayout();

    // we're done with all resizing and stuff now we can request a new
    // area to be reserved on screen
    updateStrut();

#ifdef XINERAMA
    if (m_xineramaheadmenu)
        m_xineramaheadmenu->reloadHeads();
#endif // XINERAMA

}



void Toolbar::buttonPressEvent(XButtonEvent &be) {
    Display *dpy = Fluxbox::instance()->display();

    FbTk::Menu::hideShownMenu();

    if (be.subwindow) {
        // Do not intercept mouse events that are meant for the tray icon
        if (SystemTray::doesControl(be.subwindow)) {
            XAllowEvents(dpy, ReplayPointer, CurrentTime);
            return;
        }
#if 0
        // Unfortunately, the subwindow isn't exactly a reliable source here, so
        // we COULD query the pointer (what will usually return the systray itself) and
        // check that as well. NOTICE that due to the async nature of X11, the
        // pointer might have moved and the result isn't correct either.
        Window wr, wc; int junk; unsigned int ujunk;
        XQueryPointer(dpy, be.window, &wr, &wc, &junk, &junk, &junk, &junk, &ujunk);
        if (SystemTray::doesControl(wc)) {
            XAllowEvents(dpy, ReplayPointer, CurrentTime);
            return;
        }
#endif
    }

    if (Fluxbox::instance()->keys()->doAction(be.type, be.state, be.button,
                                              Keys::ON_TOOLBAR, 0, be.time)) {
        XAllowEvents(dpy, SyncPointer, CurrentTime);
        return;
    }

    if (be.button == 1)
        raise();
    if (be.button != 2 || be.subwindow) { // only handle direct toolbar MMBs
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        return;
    }

    XAllowEvents(dpy, SyncPointer, CurrentTime);
    screen()
        .placementStrategy()
        .placeAndShowMenu(menu(), be.x_root, be.y_root, false);
}

void Toolbar::updateCrossingState() {
    Window wr, wc;
    int rx, ry, x, y;
    unsigned int mask;
    const int bw = -theme()->border().width();
    bool hovered = false;
    if (XQueryPointer(Fluxbox::instance()->display(), window().window(), &wr, &wc, &rx, &ry, &x, &y, &mask))
        hovered = x >= bw && y >= bw && x < int(width()) && y < int(height());
    if (hovered) {
        if (m_rc_auto_raise)
            m_layeritem.moveToLayer(ResourceLayer::ABOVE_DOCK);
        if (m_rc_auto_hide && isHidden())
            toggleHidden();
    } else {
        if (m_rc_auto_hide && !isHidden())
            toggleHidden();
        if (m_rc_auto_raise)
            m_layeritem.moveToLayer(m_rc_layernum->getNum());
    }
}

void Toolbar::enterNotifyEvent(XCrossingEvent &ce) {
    Fluxbox::instance()->keys()->doAction(ce.type, ce.state, 0, Keys::ON_TOOLBAR);

    if (!m_rc_auto_hide && isHidden()) {
        toggleHidden();
    }

    if ((m_rc_auto_hide || m_rc_auto_raise) && !m_hide_timer.isTiming())
        m_hide_timer.start();
}

void Toolbar::leaveNotifyEvent(XCrossingEvent &event) {

    if (menu().isVisible())
        return;

    if (!m_hide_timer.isTiming() && (m_rc_auto_hide && !isHidden()) ||
       (m_rc_auto_raise && m_layeritem.getLayerNum() != m_rc_layernum->getNum()))
        m_hide_timer.start();

    if (!isHidden())
        Fluxbox::instance()->keys()->doAction(event.type, event.state, 0, Keys::ON_TOOLBAR);
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

void Toolbar::setPlacement(Toolbar::Placement where) {
    // disable vertical toolbar

    *m_rc_placement = where;
    int head_x = 0;
    int head_y = 0;
    int head_w = screen().width();
    int head_h = screen().height();

    if (screen().hasXinerama()) {
        int head = *m_rc_on_head;
        head_x = screen().getHeadX(head);
        head_y = screen().getHeadY(head);
        head_w = screen().getHeadWidth(head);
        head_h = screen().getHeadHeight(head);
    }

    int bw = theme()->border().width();
    int pixel = (bw == 0 ? 1 : 0); // So we get at least one pixel visible in hidden mode

    frame.width = (head_w - 2*bw) * (*m_rc_width_percent) / 100;

    //!! TODO: change this
    // max height of each toolbar items font...
    unsigned int max_height = m_tool_factory.maxFontHeight() + 2;

    if (theme()->height() > 0)
        max_height = theme()->height();

    if (*m_rc_height > 0 && *m_rc_height < 100)
        max_height = *m_rc_height;

    frame.height = max_height;
    frame.height += (frame.bevel_w * 2);

    // should we flipp sizes?
    if (isVertical()) {
        frame.width = frame.height;
        frame.height = head_h * (*m_rc_width_percent) / 100;

    } // else horizontal toolbar


    frame.x = head_x;
    frame.y = head_y;
    frame.x_hidden = head_x;
    frame.y_hidden = head_y;
    FbTk::Orientation orient = _values[where].orient;
    if (m_shape.get())
        m_shape->setPlaces(_values[where].shape);

    switch (where) {
    case TOPLEFT:
        frame.y_hidden += pixel - bw - frame.height;
        break;
    case BOTTOMLEFT:
        frame.y += head_h - static_cast<int>(frame.height) - 2*bw;
        frame.y_hidden += head_h - bw - pixel;
        break;
    case TOPCENTER:
        frame.x += (head_w - static_cast<int>(frame.width))/2 - bw;
        frame.x_hidden = frame.x;
        frame.y_hidden += pixel - bw - static_cast<int>(frame.height);
        break;
    case TOPRIGHT:
        frame.x += head_w - static_cast<int>(frame.width) - bw*2;
        frame.x_hidden = frame.x;
        frame.y_hidden += pixel - bw - static_cast<int>(frame.height);
        break;
    case BOTTOMRIGHT:
        frame.x += head_w - static_cast<int>(frame.width) - bw*2;
        frame.y += head_h - static_cast<int>(frame.height) - bw*2;
        frame.x_hidden = frame.x;
        frame.y_hidden += head_h - bw - pixel;
        break;
    case BOTTOMCENTER: // default is BOTTOMCENTER
        frame.x += (head_w - static_cast<int>(frame.width))/2 - bw;
        frame.y += head_h - static_cast<int>(frame.height) - bw*2;
        frame.x_hidden = frame.x;
        frame.y_hidden += head_h - bw - pixel;
        break;
    case LEFTCENTER:
        frame.y += (head_h - static_cast<int>(frame.height))/2 - bw;
        frame.y_hidden = frame.y;
        frame.x_hidden += pixel - static_cast<int>(frame.width) - bw;
        break;
    case LEFTTOP:
        frame.x_hidden += pixel - static_cast<int>(frame.width) - bw;
        break;
    case LEFTBOTTOM:
        frame.y = head_h - static_cast<int>(frame.height) - bw*2;
        frame.y_hidden = frame.y;
        frame.x_hidden += pixel - static_cast<int>(frame.width) - bw;
        break;
    case RIGHTCENTER:
        frame.x += head_w - static_cast<int>(frame.width) - bw*2;
        frame.y += (head_h - static_cast<int>(frame.height))/2 - bw;
        frame.x_hidden += head_w - bw - pixel;
        frame.y_hidden = frame.y;
        break;
    case RIGHTTOP:
        frame.x += head_w - static_cast<int>(frame.width) - bw*2;
        frame.x_hidden += head_w - bw - pixel;
        break;
    case RIGHTBOTTOM:
        frame.x += head_w - static_cast<int>(frame.width) - bw*2;
        frame.y += head_h - static_cast<int>(frame.height) - bw*2;
        frame.x_hidden += head_w - bw - pixel;
        frame.y_hidden = frame.y;
        break;
    }

    forAll(m_item_list, std::bind(std::mem_fn(&ToolbarItem::setOrientation), _1, orient));
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
        forAll(m_item_list, std::mem_fn(&ToolbarItem::parentMoved));
    }

}

void Toolbar::toggleAboveDock() {
    if (m_layeritem.getLayerNum() == m_rc_layernum->getNum())
        m_layeritem.moveToLayer(ResourceLayer::ABOVE_DOCK);
    else
        m_layeritem.moveToLayer(m_rc_layernum->getNum());
}

void Toolbar::moveToLayer(int layernum) {
    m_layeritem.moveToLayer(layernum);
    *m_rc_layernum = layernum;
}

void Toolbar::setupMenus(bool skip_new_placement) {
    _FB_USES_NLS;
    using namespace FbTk;

    typedef RefCount<Command<void> > RefCommand;
    typedef SimpleCommand<Toolbar> ToolbarCommand;

    menu().setLabel(_FB_XTEXT(Toolbar, Toolbar,
                              "Toolbar", "Title of Toolbar menu"));

    RefCommand reconfig_toolbar(new ToolbarCommand(*this, &Toolbar::reconfigure));
    RefCommand save_resources(FbTk::CommandParser<void>::instance().parse("saverc"));
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
    menu().insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, Visible,
                                             "Visible", "Whether this item is visible"),
                                   m_rc_visible, toggle_visible_cmd));

    menu().insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, AutoHide,
                                             "Auto hide", "Toggle auto hide of toolbar"),
                                   m_rc_auto_hide,
                                   reconfig_toolbar_and_save_resource));
    menu().insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, AutoRaise,
                                             "Auto raise", "Toggle auto raise of toolbar"),
                                   m_rc_auto_raise,
                                   reconfig_toolbar_and_save_resource));

    MenuItem *toolbar_menuitem =
        new FbTk::IntMenuItem(_FB_XTEXT(Toolbar, WidthPercent,
                                     "Toolbar width percent",
                                     "Percentage of screen width taken by toolbar"),
                           m_rc_width_percent,
                           0, 100, menu()); // min/max value


    toolbar_menuitem->setCommand(reconfig_toolbar_and_save_resource);
    menu().insertItem(toolbar_menuitem);
    menu().insertItem(new FbTk::BoolMenuItem(_FB_XTEXT(Common, MaximizeOver,
                                             "Maximize Over",
                                             "Maximize over this thing when maximizing"),
                                   m_rc_maximize_over,
                                   reconfig_toolbar_and_save_resource));
    menu().insertSubmenu(_FB_XTEXT(Menu, Layer, "Layer...", "Title of Layer menu"), &layerMenu());
#ifdef XINERAMA
    if (screen().hasXinerama()) {

        m_xineramaheadmenu = new XineramaHeadMenu<Toolbar>(screen().menuTheme(),
                                     screen(),
                                     screen().imageControl(),
                                     *screen().layerManager().getLayer(::ResourceLayer::MENU),
                                     *this,
                                     _FB_XTEXT(Toolbar, OnHead, "Toolbar on Head", "Title of toolbar on head menu"));
        menu().insertSubmenu(_FB_XTEXT(Menu, OnHead, "On Head...", "Title of On Head menu"), m_xineramaheadmenu);
    }
#endif // XINERAMA


    // menu is 3 wide, 5 down
    if (!skip_new_placement) {

        static const struct {
             const FbTk::FbString label;
             Toolbar::Placement placement;
        } pm[] = {
            { _FB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), Toolbar::TOPLEFT},
            { _FB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), Toolbar::LEFTTOP},
            { _FB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), Toolbar::LEFTCENTER},
            { _FB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), Toolbar::LEFTBOTTOM},
            { _FB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), Toolbar::BOTTOMLEFT},
            { _FB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), Toolbar::TOPCENTER},
            { "", Toolbar::TOPLEFT},
            { "", Toolbar::TOPLEFT},
            { "", Toolbar::TOPLEFT},
            { _FB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), Toolbar::BOTTOMCENTER},
            { _FB_XTEXT(Align, TopRight, "Top Right", "Top Right"), Toolbar::TOPRIGHT},
            { _FB_XTEXT(Align, RightTop, "Right Top", "Right Top"), Toolbar::RIGHTTOP},
            { _FB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), Toolbar::RIGHTCENTER},
            { _FB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), Toolbar::RIGHTBOTTOM},
            { _FB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), Toolbar::BOTTOMRIGHT}
        };

        placementMenu().setMinimumColumns(3);
        // create items in sub menu
        for (size_t i=0; i< sizeof(pm)/sizeof(pm[0]); ++i) {
            if (pm[i].label == "") {
                placementMenu().insert(pm[i].label);
                placementMenu().setItemEnabled(i, false);
            } else
                placementMenu().insertItem(new PlaceToolbarMenuItem(pm[i].label, *this,
                                                                pm[i].placement));
        }
    }

    menu().insertSubmenu(_FB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu"), &placementMenu());
    placementMenu().updateMenu();


    // this saves resources and clears the slit window to update alpha value
    FbTk::MenuItem *alpha_menuitem =
        new FbTk::IntMenuItem(_FB_XTEXT(Common, Alpha, "Alpha", "Transparency level"),
                           m_rc_alpha,
                           0, 255, menu());
    // setup command for alpha value
    MacroCommand *alpha_macrocmd = new MacroCommand();
    RefCount<Command<void> > alpha_cmd(new SimpleCommand<Toolbar>(*this, &Toolbar::updateAlpha));
    alpha_macrocmd->add(save_resources);
    alpha_macrocmd->add(alpha_cmd);
    RefCount<Command<void> > set_alpha_cmd(alpha_macrocmd);
    alpha_menuitem->setCommand(set_alpha_cmd);

    menu().insertItem(alpha_menuitem);
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

    FbTk::Orientation orient = _values[placement()].orient;

    // lock this
    m_resize_lock = true;
    // calculate size for fixed items
    ItemList::iterator item_it = m_item_list.begin();
    ItemList::iterator item_it_end = m_item_list.end();
    int bevel_width = theme()->bevelWidth();
    int fixed_width = bevel_width; // combined size of all fixed items
    int relative_width = 0; // combined *desired* size of all relative items
    int stretch_items = 0;
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
                fixed_width += std::max(borderW, last_bw);
            } else {
                first = false;
            }
        }

        last_bw = borderW;

        tmpw = (*item_it)->preferredWidth();
        tmph = (*item_it)->height();
        FbTk::translateSize(orient, tmpw, tmph);

        if ((*item_it)->type() == ToolbarItem::FIXED) {
            fixed_width += tmpw;
        } else if ((*item_it)->type() == ToolbarItem::SQUARE) {
            fixed_width += height;
            if (bevel_width)
                fixed_width -= 2*(borderW + bevel_width);
        } else {
            ++relative_items;
            relative_width += tmpw;
            if (!tmpw)
                ++stretch_items;
        }
    }

    // calculate what's going to be left over to the relative sized items
    float stretch_factor = 1.0f;
    if (relative_items) {
        if (relative_width <= width - fixed_width && stretch_items) {
            relative_width = int(width - fixed_width - relative_width)/stretch_items;
        } else if (relative_width) {
            stretch_factor = float(width - fixed_width)/relative_width;
            relative_width = 0;
        }
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
            next_x += std::max(borderW, last_bw);
        }
        last_bw = borderW;

        int tmpx = next_x + offset,
            tmpy = offset;

        if ((*item_it)->type() == ToolbarItem::RELATIVE) {
            unsigned int itemw = (*item_it)->preferredWidth(), itemh = (*item_it)->height();
            FbTk::translateSize(orient, itemw, itemh);
            tmpw = itemw ? std::floor(stretch_factor * itemw) : relative_width;
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
