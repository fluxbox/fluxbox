// Ewmh.cc for fluxbox
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "Ewmh.hh"

#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "Layer.hh"
#include "fluxbox.hh"
#include "FbWinFrameTheme.hh"
#include "FocusControl.hh"
#include "Debug.hh"

#include "FbTk/App.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/I18n.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/Layer.hh"
#include "FbTk/FbPixmap.hh"

#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include <iostream>
#include <algorithm>
#include <new>
#include <cstring>
#include <cstdlib>


using std::cerr;
using std::endl;
using std::vector;
using std::list;

// mipspro has no new(nothrow)
#if defined sgi && ! defined GCC
#define FB_new_nothrow new
#else
#define FB_new_nothrow new(std::nothrow)
#endif


namespace {

/* From Extended Window Manager Hints, draft 1.3:
 *
 * _NET_WM_ICON CARDINAL[][2+n]/32
 *
 * This is an array of possible icons for the client. This specification does
 * not stipulate what size these icons should be, but individual desktop
 * environments or toolkits may do so. The Window Manager MAY scale any of
 * these icons to an appropriate size.
 *
 * This is an array of 32bit packed CARDINAL ARGB with high byte being A, low
 * byte being B. The first two cardinals are width, height. Data is in rows,
 * left to right and top to bottom. 
 *
 ***
 *
 * NOTE: the returned data for XA_CARDINAL is long if the rfmt equals
 * 32. sizeof(long) on 64bit machines is 8. to quote from
 * "man XGetWindowProperty":
 *
 *    If the returned format is 32, the property data will be stored as 
 *    an array of longs (which in a 64-bit application will be 64-bit 
 *    values that are padded in the upper 4 bytes).
 *
 * this is especially true on 64bit machines when some of the clients
 * (eg: tvtime, konqueror3) have problems to feed in the right data
 * into the _NET_WM_ICON property. we faced some segfaults because
 * width and height were not quite right because of ignoring 64bit
 * behaviour on client side.
 *
 * TODO: maybe move the pixmap-creation code to FbTk? */
void extractNetWmIcon(Atom net_wm_icon, WinClient& winclient) {

    typedef std::pair<int, int> Size;
    typedef std::map<Size, const unsigned long*> IconContainer;

    unsigned long* raw_data = 0;
    unsigned long nr_icon_data = 0;

    {
        Atom rtype;
        int rfmt;
        unsigned long nr_read;
        unsigned long nr_bytes_left;

        // no data or no _NET_WM_ICON
        if (! winclient.property(net_wm_icon, 0L, 0L, False, XA_CARDINAL,
                                 &rtype, &rfmt, &nr_read, &nr_bytes_left,
                                 reinterpret_cast<unsigned char**>(&raw_data)) || nr_bytes_left == 0) {

            if (raw_data)
                XFree(raw_data);

            return;
        }

        // actually there is some data in _NET_WM_ICON
        nr_icon_data = nr_bytes_left / sizeof(CARD32);

        fbdbg << "extractNetWmIcon: " << winclient.title().logical() << "\n";
        fbdbg << "nr_icon_data: " << nr_icon_data << "\n";

        // read all the icons stored in _NET_WM_ICON
        if (raw_data)
            XFree(raw_data);

        // something went wrong
        if (!winclient.property(net_wm_icon, 0L, nr_icon_data, False, XA_CARDINAL,
                           &rtype, &rfmt, &nr_read, &nr_bytes_left,
                           reinterpret_cast<unsigned char**>(&raw_data))) {

            return;
        }

        fbdbg << "nr_read: " << nr_read << "|" << nr_bytes_left << "\n";

    }

    IconContainer icon_data; // stores all available data, sorted by size (width x height)
    unsigned long width;
    unsigned long height;

    // analyze the available icons
    //
    // check also for invalid values coming in from "bad" applications
    unsigned long i;
    for (i = 0; i + 2 < nr_icon_data; i += width * height ) {

        width = raw_data[i++];
        if (width >= nr_icon_data) {

            fbdbg << "Ewmh.cc extractNetWmIcon found strange _NET_WM_ICON width (" 
                  << width << ") for " << winclient.title().logical() << "\n";
            break;
        }

        height = raw_data[i++];
        if (height >= nr_icon_data) {

            fbdbg << "Ewmh.cc extractNetWmIcon found strange _NET_WM_ICON height (" 
                  << height << ") for " << winclient.title().logical() << "\n";

            break;
        }

        // strange values stored in the NETWM_ICON
        if (i + width * height > nr_icon_data) {
            fbdbg << "Ewmh.cc extractNetWmIcon found strange _NET_WM_ICON dimensions (" 

                  << width << "x" << height << ")for " << winclient.title().logical() << "\n";

            break;
        }

        icon_data[Size(width, height)] = &raw_data[i];
    }

    // no valid icons found at all
    if (icon_data.empty()) {
        XFree(raw_data);
        return;
    }

    Display* dpy = FbTk::App::instance()->display();
    int scrn = winclient.screen().screenNumber();

    // the icon will not be used by the client but by
    // 'menu', 'iconbar', 'titlebar'. all these entities
    // are created based upon the rootwindow and
    // the default depth. if we would use winclient.depth()
    // and winclient.drawable() here we might get into trouble
    // (xfce4-terminal, skype .. 32bit visuals vs 24bit fluxbox
    // entities)
    Drawable parent = winclient.screen().rootWindow().drawable();
    unsigned int depth = DefaultDepth(dpy, scrn);

    // pick the smallest icon size atm
    // TODO: find a better criteria
    width = icon_data.begin()->first.first;
    height = icon_data.begin()->first.second;

    // tmp image for the pixmap
    XImage* img_pm = XCreateImage(dpy, DefaultVisual(dpy, scrn), depth,
                                  ZPixmap,
                                  0, NULL, width, height, 32, 0);
    if (!img_pm) {
        XFree(raw_data);
        return;
    }

    // tmp image for the mask
    XImage* img_mask = XCreateImage(dpy, DefaultVisual(dpy, scrn), 1,
                                  XYBitmap,
                                  0, NULL, width, height, 32, 0);

    if (!img_mask) {
        XFree(raw_data);
        XDestroyImage(img_pm);
        return;
    }

    // allocate some memory for the icons at client side
    img_pm->data = static_cast<char*>(malloc(img_pm->bytes_per_line * height));
    img_mask->data = static_cast<char*>(malloc(img_mask->bytes_per_line * height));


    const unsigned long* src = icon_data.begin()->second;
    unsigned int rgba;
    unsigned long pixel;
    unsigned long x;
    unsigned long y;
    unsigned char r, g, b, a;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++, src++) {

            rgba = *src; // use only 32bit

            a = ( rgba & 0xff000000 ) >> 24;
            r = ( rgba & 0x00ff0000 ) >> 16;
            g = ( rgba & 0x0000ff00 ) >> 8;
            b = ( rgba & 0x000000ff );

            // 15 bit display, 5R 5G 5B
            if (img_pm->red_mask == 0x7c00
                && img_pm->green_mask == 0x03e0
                && img_pm->blue_mask == 0x1f) {

                pixel = ((r << 7) & 0x7c00) | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);

            // 16 bit display, 5R 6G 5B
            } else if (img_pm->red_mask == 0xf800
                       && img_pm->green_mask == 0x07e0
                       && img_pm->blue_mask == 0x1f) {

                pixel = ((r << 8) & 0xf800) | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);

            // 24/32 bit display, 8R 8G 8B
            } else if (img_pm->red_mask == 0xff0000
                       && img_pm->green_mask == 0xff00
                       && img_pm->blue_mask == 0xff) {

                pixel = rgba & 0x00ffffff;

            } else {
                pixel = 0;
            }

            // transfer rgb data
            XPutPixel(img_pm, x, y, pixel);

            // transfer mask data
            XPutPixel(img_mask, x, y, a > 127 ? 0 : 1);
        }
    }

    // the final icon
    FbTk::PixmapWithMask icon;
    icon.pixmap() = FbTk::FbPixmap(parent, width, height, depth);
    icon.mask() = FbTk::FbPixmap(parent, width, height, 1);

    FbTk::GContext gc_pm(icon.pixmap());
    FbTk::GContext gc_mask(icon.mask());

    XPutImage(dpy, icon.pixmap().drawable(), gc_pm.gc(), img_pm, 0, 0, 0, 0, width, height);
    XPutImage(dpy, icon.mask().drawable(), gc_mask.gc(), img_mask, 0, 0, 0, 0, width, height);

    XDestroyImage(img_pm);   // frees img_pm->data as well
    XDestroyImage(img_mask); // frees img_mask->data as well

    XFree(raw_data);

    winclient.setIcon(icon);
}

} // end anonymous namespace

class Ewmh::EwmhAtoms {
public:

    EwmhAtoms() {
        Display *disp = FbTk::App::instance()->display();

        supported = XInternAtom(disp, "_NET_SUPPORTED", False);
        client_list = XInternAtom(disp, "_NET_CLIENT_LIST", False);
        client_list_stacking = XInternAtom(disp, "_NET_CLIENT_LIST_STACKING", False);
        number_of_desktops = XInternAtom(disp, "_NET_NUMBER_OF_DESKTOPS", False);
        desktop_geometry = XInternAtom(disp, "_NET_DESKTOP_GEOMETRY", False);
        desktop_viewport = XInternAtom(disp, "_NET_DESKTOP_VIEWPORT", False);
        current_desktop = XInternAtom(disp, "_NET_CURRENT_DESKTOP", False);
        desktop_names = XInternAtom(disp, "_NET_DESKTOP_NAMES", False);
        active_window = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
        workarea = XInternAtom(disp, "_NET_WORKAREA", False);
        supporting_wm_check = XInternAtom(disp, "_NET_SUPPORTING_WM_CHECK", False);
        virtual_roots = XInternAtom(disp, "_NET_VIRTUAL_ROOTS", False);

        close_window = XInternAtom(disp, "_NET_CLOSE_WINDOW", False);
        moveresize_window = XInternAtom(disp, "_NET_MOVERESIZE_WINDOW", False);
        restack_window = XInternAtom(disp, "_NET_RESTACK_WINDOW", False);
        request_frame_extents = XInternAtom(disp,
                "_NET_REQUEST_FRAME_EXTENTS", False);


        wm_moveresize = XInternAtom(disp, "_NET_WM_MOVERESIZE", False);

        properties = XInternAtom(disp, "_NET_PROPERTIES", False);
        wm_name = XInternAtom(disp, "_NET_WM_NAME", False);
        wm_icon_name = XInternAtom(disp, "_NET_WM_ICON_NAME", False);
        wm_desktop = XInternAtom(disp, "_NET_WM_DESKTOP", False);

        // type atoms
        wm_window_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
        wm_window_type_dock = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);
        wm_window_type_desktop = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
        wm_window_type_splash = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_SPLASH", False);
        wm_window_type_dialog = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DIALOG", False);
        wm_window_type_menu = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_MENU", False);
        wm_window_type_toolbar = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
        wm_window_type_normal = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_NORMAL", False);

        // state atom and the supported state atoms
        wm_state = XInternAtom(disp, "_NET_WM_STATE", False);
        wm_state_sticky = XInternAtom(disp, "_NET_WM_STATE_STICKY", False);
        wm_state_shaded = XInternAtom(disp, "_NET_WM_STATE_SHADED", False);
        wm_state_maximized_horz = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        wm_state_maximized_vert = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        wm_state_fullscreen = XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False);
        wm_state_hidden = XInternAtom(disp, "_NET_WM_STATE_HIDDEN", False);
        wm_state_skip_taskbar = XInternAtom(disp, "_NET_WM_STATE_SKIP_TASKBAR", False);
        wm_state_skip_pager = XInternAtom(disp, "_NET_WM_STATE_SKIP_PAGER", False);
        wm_state_above = XInternAtom(disp, "_NET_WM_STATE_ABOVE", False);
        wm_state_below = XInternAtom(disp, "_NET_WM_STATE_BELOW", False);
        wm_state_modal = XInternAtom(disp, "_NET_WM_STATE_MODAL", False);
        wm_state_demands_attention = XInternAtom(disp, "_NET_WM_STATE_DEMANDS_ATTENTION", False);

        // allowed actions
        wm_allowed_actions = XInternAtom(disp, "_NET_WM_ALLOWED_ACTIONS", False);
        wm_action_move = XInternAtom(disp, "_NET_WM_ACTION_MOVE", False);
        wm_action_resize = XInternAtom(disp, "_NET_WM_ACTION_RESIZE", False);
        wm_action_minimize = XInternAtom(disp, "_NET_WM_ACTION_MINIMIZE", False);
        wm_action_shade = XInternAtom(disp, "_NET_WM_ACTION_SHADE", False);
        wm_action_stick = XInternAtom(disp, "_NET_WM_ACTION_STICK", False);
        wm_action_maximize_horz = XInternAtom(disp, "_NET_WM_ACTION_MAXIMIZE_HORZ", False);
        wm_action_maximize_vert = XInternAtom(disp, "_NET_WM_ACTION_MAXIMIZE_VERT", False);
        wm_action_fullscreen = XInternAtom(disp, "_NET_WM_ACTION_FULLSCREEN", False);
        wm_action_change_desktop = XInternAtom(disp, "_NET_WM_ACTION_CHANGE_DESKTOP", False);
        wm_action_close = XInternAtom(disp, "_NET_WM_ACTION_CLOSE", False);

        wm_strut = XInternAtom(disp, "_NET_WM_STRUT", False);
        wm_icon_geometry = XInternAtom(disp, "_NET_WM_ICON_GEOMETRY", False);
        wm_icon = XInternAtom(disp, "_NET_WM_ICON", False);
        wm_pid = XInternAtom(disp, "_NET_WM_PID", False);
        wm_handled_icons = XInternAtom(disp, "_NET_WM_HANDLED_ICONS", False);

        frame_extents = XInternAtom(disp, "_NET_FRAME_EXTENTS", False);

        wm_ping = XInternAtom(disp, "_NET_WM_PING", False);
        utf8_string = XInternAtom(disp, "UTF8_STRING", False);
    };

    // root window properties
    Atom supported,
         client_list,
         client_list_stacking,
         number_of_desktops,
         desktop_geometry,
         desktop_viewport,
         current_desktop,
         desktop_names,
         active_window,
         workarea,
         supporting_wm_check,
         virtual_roots,
         moveresize_window,
         restack_window,
         request_frame_extents;

    // root window messages
    Atom close_window,
         wm_moveresize;

    // application window properties
    Atom properties,
         wm_name,
         wm_icon_name,
         wm_desktop,
         // types
         wm_window_type,
         wm_window_type_dock,
         wm_window_type_desktop,
         wm_window_type_splash,
         wm_window_type_dialog,
         wm_window_type_menu,
         wm_window_type_toolbar,
         wm_window_type_normal,

         // states
         wm_state,
         wm_state_sticky,
         wm_state_shaded,
         wm_state_maximized_horz,
         wm_state_maximized_vert,
         wm_state_fullscreen,
         wm_state_hidden,
         wm_state_skip_taskbar,
         wm_state_skip_pager,
         wm_state_below,
         wm_state_above,
         wm_state_modal,
         wm_state_demands_attention,

         // allowed actions
         wm_allowed_actions,
         wm_action_move,
         wm_action_resize,
         wm_action_minimize,
         wm_action_shade,
         wm_action_stick,
         wm_action_maximize_horz,
         wm_action_maximize_vert,
         wm_action_fullscreen,
         wm_action_change_desktop,
         wm_action_close,

         wm_strut,
         wm_icon_geometry,
         wm_icon,
         wm_pid,
         wm_handled_icons,

         frame_extents;

    // application protocols
    Atom wm_ping;

    Atom utf8_string;
};


enum EwmhMoveResizeDirection {
    _NET_WM_MOVERESIZE_SIZE_TOPLEFT    =   0,
    _NET_WM_MOVERESIZE_SIZE_TOP        =   1,
    _NET_WM_MOVERESIZE_SIZE_TOPRIGHT   =   2,
    _NET_WM_MOVERESIZE_SIZE_RIGHT      =   3,
    _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT =  4,
    _NET_WM_MOVERESIZE_SIZE_BOTTOM      =  5,
    _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT  =  6,
    _NET_WM_MOVERESIZE_SIZE_LEFT        =  7,
    _NET_WM_MOVERESIZE_MOVE             =  8,   // movement only
    _NET_WM_MOVERESIZE_SIZE_KEYBOARD    =  9,   // size via keyboard
    _NET_WM_MOVERESIZE_MOVE_KEYBOARD    = 10,   // move via keyboard
    _NET_WM_MOVERESIZE_CANCEL           = 11    // cancel operation
};

Ewmh::Ewmh() {
    setName("ewmh");
    m_net = new EwmhAtoms;
}

Ewmh::~Ewmh() {
    delete m_net;
}

void Ewmh::initForScreen(BScreen &screen) {
    Display *disp = FbTk::App::instance()->display();

    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_SUPPORTING_WM_CHECK
     *
     * The Window Manager MUST set this property on the root window
     * to be the ID of a child window created by himself, to indicate
     * that a compliant window manager is active. The child window
     * MUST also have the _NET_SUPPORTING_WM_CHECK property set to
     * the ID of the child window. The child window MUST also have
     * the _NET_WM_NAME property set to the name of the Window Manager.
     *
     * Rationale: The child window is used to distinguish an active
     * Window Manager from a stale _NET_SUPPORTING_WM_CHECK property
     * that happens to point to another window. If the
     * _NET_SUPPORTING_WM_CHECK window on the client window is missing
     * or not properly set, clients SHOULD assume that no conforming
     * Window Manager is present.
     */

    Window wincheck = screen.dummyWindow().window();

    if (wincheck != None) {
        screen.rootWindow().changeProperty(m_net->supporting_wm_check, XA_WINDOW, 32,
                                           PropModeReplace, (unsigned char *) &wincheck, 1);
        XChangeProperty(disp, wincheck, m_net->supporting_wm_check, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char *) &wincheck, 1);

        XChangeProperty(disp, wincheck, m_net->wm_name, m_net->utf8_string, 8,
                        PropModeReplace, (unsigned char *) "Fluxbox", strlen("Fluxbox"));
    }

    //set supported atoms
    Atom atomsupported[] = {
        // window properties
        m_net->wm_strut,
        m_net->wm_state,
        m_net->wm_name,
        m_net->wm_icon,
        m_net->wm_icon_name,

        // states that we support:
        m_net->wm_state_sticky,
        m_net->wm_state_shaded,
        m_net->wm_state_maximized_horz,
        m_net->wm_state_maximized_vert,
        m_net->wm_state_fullscreen,
        m_net->wm_state_hidden,
        m_net->wm_state_skip_taskbar,
        m_net->wm_state_modal,
        m_net->wm_state_below,
        m_net->wm_state_above,
        m_net->wm_state_demands_attention,

        // window type
        m_net->wm_window_type,
        m_net->wm_window_type_dock,
        m_net->wm_window_type_desktop,
        m_net->wm_window_type_splash,
        m_net->wm_window_type_dialog,
        m_net->wm_window_type_menu,
        m_net->wm_window_type_toolbar,
        m_net->wm_window_type_normal,

        // window actions
        m_net->wm_allowed_actions,
        m_net->wm_action_move,
        m_net->wm_action_resize,
        m_net->wm_action_minimize,
        m_net->wm_action_shade,
        m_net->wm_action_stick,
        m_net->wm_action_maximize_horz,
        m_net->wm_action_maximize_vert,
        m_net->wm_action_fullscreen,
        m_net->wm_action_change_desktop,
        m_net->wm_action_close,

        // root properties
        m_net->client_list,
        m_net->client_list_stacking,
        m_net->number_of_desktops,
        m_net->current_desktop,
        m_net->active_window,
        m_net->close_window,
        m_net->moveresize_window,
        m_net->workarea,
        m_net->restack_window,
        m_net->request_frame_extents,

        m_net->wm_moveresize,

        m_net->frame_extents,

        // desktop properties
        m_net->wm_desktop,
        m_net->desktop_names,
        m_net->desktop_viewport,
        m_net->desktop_geometry,

        m_net->supporting_wm_check
    };
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_SUPPORTED, ATOM[]/32
     *
     * This property MUST be set by the Window Manager
     * to indicate which hints it supports. For
     * example: considering _NET_WM_STATE both this
     * atom and all supported states
     * e.g. _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY,
     * would be listed. This assumes that backwards
     * incompatible changes will not be made to the
     * hints (without being renamed).
     */
    screen.rootWindow().changeProperty(m_net->supported, XA_ATOM, 32,
                                       PropModeReplace,
                                       (unsigned char *) &atomsupported,
                                       (sizeof atomsupported)/sizeof atomsupported[0]);

    // update atoms

    updateWorkspaceCount(screen);
    updateCurrentWorkspace(screen);
    updateWorkspaceNames(screen);
    updateClientList(screen);
    updateViewPort(screen);
    updateGeometry(screen);
    updateWorkarea(screen);

}

void Ewmh::setupClient(WinClient &winclient) {
    updateStrut(winclient);

    FbTk::FbString newtitle = winclient.textProperty(m_net->wm_name);
    if (!newtitle.empty())
        winclient.setTitle(newtitle);

    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    unsigned char* data = 0;


    extractNetWmIcon(m_net->wm_icon, winclient);


    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WM_WINDOW_TYPE, ATOM[]/32
     *
     * This SHOULD be set by the Client before mapping to a list of atoms
     * indicating the functional type of the window. This property SHOULD
     * be used by the window manager in determining the decoration,
     * stacking position and other behavior of the window. The Client
     * SHOULD specify window types in order of preference (the first being
     * most preferable) but MUST include at least one of the basic window
     * type atoms from the list below. This is to allow for extension of
     * the list of types whilst providing default behavior for Window
     * Managers that do not recognize the extensions.
     *
     */

    winclient.property(m_net->wm_window_type, 0, 0x7fffffff, False, XA_ATOM,
                       &ret_type, &fmt, &nitems, &bytes_after,
                       &data);
    WindowState::WindowType type = WindowState::TYPE_NORMAL;
    if (data) {
        Atom *atoms = (unsigned long *)data;
        for (unsigned long l = 0; l < nitems; ++l) {
            if (atoms[l] == m_net->wm_window_type_dock)
                type = WindowState::TYPE_DOCK;
            else if (atoms[l] == m_net->wm_window_type_desktop)
                type = WindowState::TYPE_DESKTOP;
            else if (atoms[l] == m_net->wm_window_type_splash)
                type = WindowState::TYPE_SPLASH;
            else if (atoms[l] == m_net->wm_window_type_dialog)
                type = WindowState::TYPE_DIALOG;
            else if (atoms[l] == m_net->wm_window_type_menu)
                type = WindowState::TYPE_MENU;
            else if (atoms[l] == m_net->wm_window_type_toolbar)
                type = WindowState::TYPE_TOOLBAR;
            else if (atoms[l] != m_net->wm_window_type_normal)
                continue;
            /*
             * NOT YET IMPLEMENTED:
             *   _NET_WM_WINDOW_TYPE_UTILITY
             */
            break;
        }
        XFree(data);
    } else if (winclient.isTransient()) {
        // if _NET_WM_WINDOW_TYPE not set and this window
        // has transient_for the type must be set to _NET_WM_WINDOW_TYPE_DIALOG
        type = WindowState::TYPE_DIALOG;
        winclient.
            changeProperty(m_net->wm_window_type,
                           XA_ATOM, 32, PropModeReplace,
                           (unsigned char*)&m_net->wm_window_type_dialog, 1);

    }
    winclient.setWindowType(type);


}

void Ewmh::setupFrame(FluxboxWindow &win) {
    setupState(win);
    bool exists;
    unsigned int desktop=static_cast<unsigned int>(win.winClient().cardinalProperty(m_net->wm_desktop, &exists));
    if (exists) {
        if (desktop == (unsigned int)(-1) && !win.isStuck())
            win.stick();
        else
            win.setWorkspace(desktop);
    } else {
        updateWorkspace(win);
    }

    updateFrameExtents(win);

}

void Ewmh::updateFrameClose(FluxboxWindow &win) {
}

void Ewmh::updateFocusedWindow(BScreen &screen, Window win) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_ACTIVE_WINDOW, WINDOW/32
     *
     * The window ID of the currently active window or None
     * if no window has the focus. This is a read-only
     * property set by the Window Manager.
     *
     */
    screen.rootWindow().changeProperty(m_net->active_window,
                                       XA_WINDOW, 32,
                                       PropModeReplace,
                                       (unsigned char *)&win, 1);
}

// EWMH says, regarding _NET_WM_STATE and _NET_WM_DESKTOP
// The Window Manager should remove the property whenever a window is withdrawn
// but it should leave the property in place when it is shutting down
void Ewmh::updateClientClose(WinClient &winclient){
    if (!winclient.screen().isShuttingdown()) {
        XDeleteProperty(FbTk::App::instance()->display(), winclient.window(),
                        m_net->wm_state);
        XDeleteProperty(FbTk::App::instance()->display(), winclient.window(),
                        m_net->wm_desktop);
    }
}

void Ewmh::updateClientList(BScreen &screen) {

    if (screen.isShuttingdown())
        return;

    list<Focusable *> creation_order_list =
            screen.focusControl().creationOrderList().clientList();

    size_t num = creation_order_list.size();
    Window *wl = FB_new_nothrow Window[num];
    if (wl == 0) {
        _FB_USES_NLS;
        cerr<<_FB_CONSOLETEXT(Ewmh, OutOfMemoryClientList,
                      "Fatal: Out of memory, can't allocate for EWMH client list", "")<<endl;
        return;
    }

    int win=0;
    list<Focusable *>::iterator client_it = creation_order_list.begin();
    list<Focusable *>::iterator client_it_end = creation_order_list.end();
    for (; client_it != client_it_end; ++client_it) {
        WinClient *client = dynamic_cast<WinClient *>(*client_it);
        if (client)
            wl[win++] = client->window();
    }

    /*  From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_CLIENT_LIST, WINDOW[]/32
     * _NET_CLIENT_LIST_STACKING, WINDOW[]/32
     *
     * These arrays contain all X Windows managed by
     * the Window Manager. _NET_CLIENT_LIST has
     * initial mapping order, starting with the oldest
     * window. _NET_CLIENT_LIST_STACKING has
     * bottom-to-top stacking order. These properties
     * SHOULD be set and updated by the Window
     * Manager.
     */
    screen.rootWindow().changeProperty(m_net->client_list,
                                       XA_WINDOW, 32,
                                       PropModeReplace, (unsigned char *)wl, num);
    screen.rootWindow().changeProperty(m_net->client_list_stacking,
                                       XA_WINDOW, 32,
                                       PropModeReplace, (unsigned char *)wl, num);

    delete [] wl;
}

void Ewmh::updateWorkspaceNames(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_DESKTOP_NAMES, UTF8_STRING[]
     *
     * The names of all virtual desktops.
     * This is a list of NULL-terminated strings in UTF-8
     * encoding [UTF8]. This property MAY be changed by a
     * Pager or the Window Manager at any time.
     *
     * Note: The number of names could be different from
     * _NET_NUMBER_OF_DESKTOPS. If it is less than
     * _NET_NUMBER_OF_DESKTOPS, then the desktops with high
     *  numbers are unnamed. If it is larger than
     * _NET_NUMBER_OF_DESKTOPS, then the excess names outside
     * of the _NET_NUMBER_OF_DESKTOPS are considered to be
     * reserved in case the number of desktops is increased.
     *
     * Rationale: The name is not a necessary attribute of a
     * virtual desktop. Thus the availability or unavailability
     * of names has no impact on virtual desktop functionality.
     * Since names are set by users and users are likely to
     * preset names for a fixed number of desktops, it
     * doesn't make sense to shrink or grow this list when the
     * number of available desktops changes.
     *
     */
    XTextProperty text;
    const BScreen::WorkspaceNames &workspacenames = screen.getWorkspaceNames();
    const size_t number_of_desks = workspacenames.size();

    /* the SPEC states "NULL-terminated strings". This implies, that also the
     * last element actually gets proper NULL-termination after being treated
     * by Xutf8TextListToTextProperty. Xutf8TextListToTextProperty removes
     * the NULL from the last name and thus it is missing when reading the
     * _NET_DESKTOP_NAMES property. This might confuse other WMs, pagers etc.
     * thus, an artifical "empty" name is added at the end of the regular
     * names list which is then properly encoded by Xutf8TextListToTextProperty
     * and everyone is happy
     */
    const char* names[number_of_desks+1];

    for (size_t i = 0; i < number_of_desks; i++) {
        names[i] = workspacenames[i].c_str();
    }
    names[number_of_desks] = NULL;

#ifdef X_HAVE_UTF8_STRING
    int code = Xutf8TextListToTextProperty(FbTk::App::instance()->display(),
                                const_cast<char**>(names), number_of_desks+1, XUTF8StringStyle, &text);
    if (code != XNoMemory && code != XLocaleNotSupported) {
        XSetTextProperty(FbTk::App::instance()->display(),
                         screen.rootWindow().window(),
                         &text, m_net->desktop_names);
        XFree(text.value);
    }
#else
    if (XStringListToTextProperty(names, number_of_desks+1, &text)) {
        XSetTextProperty(FbTk::App::instance()->display(), screen.rootWindow().window(),
                &text, m_net->desktop_names);
        XFree(text.value);
    }
#endif
}

void Ewmh::updateCurrentWorkspace(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_CURRENT_DESKTOP desktop, CARDINAL/32
     *
     * The index of the current desktop. This is always
     * an integer between 0 and _NET_NUMBER_OF_DESKTOPS - 1.
     * This MUST be set and updated by the Window Manager.
     *
     */
    unsigned long workspace = screen.currentWorkspaceID();
    screen.rootWindow().changeProperty(m_net->current_desktop,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)&workspace, 1);

}

void Ewmh::updateWorkspaceCount(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_NUMBER_OF_DESKTOPS, CARDINAL/32
     *
     * This property SHOULD be set and updated by the
     * Window Manager to indicate the number of virtual
     * desktops.
     */
    unsigned long numworkspaces = screen.numberOfWorkspaces();
    screen.rootWindow().changeProperty(m_net->number_of_desktops,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)&numworkspaces, 1);
}

void Ewmh::updateViewPort(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_DESKTOP_VIEWPORT x, y, CARDINAL[][2]/32
     *
     * Array of pairs of cardinals that define the
     * top left corner of each desktop's viewport.
     * For Window Managers that don't support large
     * desktops, this MUST always be set to (0,0).
     *
     */
    long value[2] = {0, 0}; // we dont support large desktops
    screen.rootWindow().changeProperty(m_net->desktop_viewport,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)value, 2);
}

void Ewmh::updateGeometry(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_DESKTOP_GEOMETRY width, height, CARDINAL[2]/32
     *
     * Array of two cardinals that defines the common size
     * of all desktops (this is equal to the screen size
     * if the Window Manager doesn't support large
     * desktops, otherwise it's equal to the virtual size
     * of the desktop). This property SHOULD be set by the
     * Window Manager.
     *
     */
    long value[2] = {screen.width(), screen.height()};
    screen.rootWindow().changeProperty(m_net->desktop_geometry,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)value, 2);

}

void Ewmh::updateWorkarea(BScreen &screen) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WORKAREA, x, y, width, height CARDINAL[][4]/32
     *
     * This property MUST be set by the Window Manager upon
     * calculating the work area for each desktop. Contains a
     * geometry for each desktop. These geometries are
     * specified relative to the viewport on each desktop and
     * specify an area that is completely contained within the
     * viewport. Work area SHOULD be used by desktop applications
     * to place desktop icons appropriately.
     *
     */

    /* !!TODO
     * Not sure how to handle xinerama stuff here.
     * So i'm just doing this on the first head.
     */
    unsigned long *coords = new unsigned long[4*screen.numberOfWorkspaces()];
    for (unsigned int i=0; i < screen.numberOfWorkspaces()*4; i+=4) {
        // x, y
        coords[i] = screen.maxLeft(0);
        coords[i + 1] = screen.maxTop(0);
        // width, height
        coords[i + 2] = screen.maxRight(0) - screen.maxLeft(0);
        coords[i + 3] = screen.maxBottom(0) - screen.maxTop(0);

    }
    screen.rootWindow().changeProperty(m_net->workarea,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)coords,
                                       4 * screen.numberOfWorkspaces());

    delete[] coords;
}

void Ewmh::updateState(FluxboxWindow &win) {


    updateActions(win);

    typedef vector<Atom> StateVec;

    StateVec state;

    if (win.isMaximizedHorz())
        state.push_back(m_net->wm_state_maximized_horz);
    if (win.isMaximizedVert())
        state.push_back(m_net->wm_state_maximized_vert);
    if (win.isStuck())
        state.push_back(m_net->wm_state_sticky);
    if (win.isShaded())
        state.push_back(m_net->wm_state_shaded);
    if (win.layerNum() == ResourceLayer::BOTTOM)
        state.push_back(m_net->wm_state_below);
    if (win.layerNum() == ResourceLayer::ABOVE_DOCK)
        state.push_back(m_net->wm_state_above);
    if (win.isIconic())
        state.push_back(m_net->wm_state_hidden);
    if (win.isIconHidden())
        state.push_back(m_net->wm_state_skip_taskbar);
    if (win.isFullscreen())
        state.push_back(m_net->wm_state_fullscreen);

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {

        StateVec client_state(state);
        Atom ret_type;
        int fmt;
        unsigned long nitems, bytes_after;
        unsigned char *data = 0;

        // set client-specific state
        if ((*it)->isStateModal())
            client_state.push_back(m_net->wm_state_modal);
        if (Fluxbox::instance()->attentionHandler().isDemandingAttention(**it))
            client_state.push_back(m_net->wm_state_demands_attention);

        // search the old states for _NET_WM_STATE_SKIP_PAGER and append it
        // to the current state, so it wont get deleted by us.
        (*it)->property(m_net->wm_state, 0, 0x7fffffff, False, XA_ATOM,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 &data);
        if (data) {
            Atom *old_states = (Atom *)data;
            for (unsigned long i=0; i < nitems; ++i) {
                if (old_states[i] == m_net->wm_state_skip_pager) {
                    client_state.push_back(m_net->wm_state_skip_pager);
                }
            }
            XFree(data);
        }

        if (!client_state.empty()) {
            (*it)->changeProperty(m_net->wm_state, XA_ATOM, 32, PropModeReplace,
                                  reinterpret_cast<unsigned char*>(&client_state.front()),
                                  client_state.size());
        } else
            (*it)->deleteProperty(m_net->wm_state);
    }
}

void Ewmh::updateLayer(FluxboxWindow &win) {
    updateState(win);
}

void Ewmh::updateHints(FluxboxWindow &win) {
}

void Ewmh::updateWorkspace(FluxboxWindow &win) {
    long workspace = win.workspaceNumber();

    if (win.isStuck())
        workspace = -1; // appear on all desktops/workspaces

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        (*it)->changeProperty(m_net->wm_desktop, XA_CARDINAL, 32, PropModeReplace,
                              (unsigned char *)&workspace, 1);
    }

}


// return true if we did handle the atom here
bool Ewmh::checkClientMessage(const XClientMessageEvent &ce,
                              BScreen * screen, WinClient * const winclient) {

    if (ce.message_type == m_net->wm_desktop) {
        // ce.data.l[0] = workspace number
        // valid window

        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;

        FluxboxWindow *fbwin = winclient->fbwindow();

        // if it's stick, make sure it is stuck.
        // otherwise, make sure it isn't stuck
        if (ce.data.l[0] == -1) {
            if (!fbwin->isStuck())
                fbwin->stick();
            return true;
        } else if (fbwin->isStuck())
            fbwin->stick();

        // the screen is the root window of the message,
        // which doesn't apply here (so borrow the variable :) )
        screen = &fbwin->screen();
        // valid workspace number?
        if (static_cast<unsigned int>
            (ce.data.l[0]) < screen->numberOfWorkspaces())
            screen->sendToWorkspace(ce.data.l[0], fbwin, false);

        return true;
    } else if (ce.message_type == m_net->wm_state) {
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;

        FluxboxWindow &win = *winclient->fbwindow();
        // ce.data.l[0] = the action (remove, add or toggle)
        // ce.data.l[1] = the first property to alter
        // ce.data.l[2] = second property to alter (can be zero)
        if (ce.data.l[0] == STATE_REMOVE) {
            setState(win, ce.data.l[1], false, *winclient);
            setState(win, ce.data.l[2], false, *winclient);
        } else if (ce.data.l[0] == STATE_ADD) {
            setState(win, ce.data.l[1], true, *winclient);
            setState(win, ce.data.l[2], true, *winclient);
        } else if (ce.data.l[0] == STATE_TOGGLE) {
            toggleState(win, ce.data.l[1]);
            toggleState(win, ce.data.l[2]);
        }
        return true;
    } else if (ce.message_type == m_net->number_of_desktops) {
        if (screen == 0)
            return true;
        // ce.data.l[0] = number of workspaces

        // no need to alter number of desktops if they are the same
        // or if requested number of workspace is less than zero
        if (screen->numberOfWorkspaces() == static_cast<unsigned int>(ce.data.l[0]) ||
            ce.data.l[0] < 0)
            return true;

        if (screen->numberOfWorkspaces() > static_cast<unsigned int>(ce.data.l[0])) {
            // remove last workspace until we have
            // the same number of workspaces
            while (screen->numberOfWorkspaces() != static_cast<unsigned int>(ce.data.l[0])) {
                screen->removeLastWorkspace();
                if (screen->numberOfWorkspaces() == 1) // must have at least one workspace
                    break;
            }
        } else { // add workspaces to screen until workspace count match the requested size
            while (screen->numberOfWorkspaces() != static_cast<unsigned int>(ce.data.l[0])) {
                screen->addWorkspace();
            }
        }

        return true;
    } else if (ce.message_type == m_net->current_desktop) {
        if (screen == 0)
            return true;
        // ce.data.l[0] = workspace number

        // prevent out of range value
        if (static_cast<unsigned int>(ce.data.l[0]) >= screen->numberOfWorkspaces())
            return true;
        screen->changeWorkspaceID(ce.data.l[0]);
        return true;
    } else if (ce.message_type == m_net->active_window) {

        // make sure we have a valid window
        if (winclient == 0)
            return true;
        // ce.window = window to focus

        // ce.data.l[0] == 2 means the request came from a pager
        if (winclient->fbwindow() && (ce.data.l[0] == 2 ||
            winclient->fbwindow()->focusRequestFromClient(*winclient))) {
            winclient->focus();
            winclient->fbwindow()->raise();
        }
        return true;
    } else if (ce.message_type == m_net->close_window) {
        if (winclient == 0)
            return true;
        // ce.window = window to close (which in this case is the win argument)
        winclient->sendClose();
        return true;
    } else if (ce.message_type == m_net->moveresize_window) {
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;
        // ce.data.l[0] = gravity and flags
        int x = (ce.data.l[0] & (1 << 8)) ? ce.data.l[1] :
            winclient->fbwindow()->x();
        int y = (ce.data.l[0] & (1 << 9)) ? ce.data.l[2] :
            winclient->fbwindow()->y();
        unsigned int width = (ce.data.l[0] & (1 << 10)) ? ce.data.l[3] :
            winclient->width();
        unsigned int height = (ce.data.l[0] & (1 << 11)) ? ce.data.l[4] :
            winclient->height();
        int win_gravity=ce.data.l[0] & 0xFF;
        winclient->fbwindow()->moveResizeForClient(x, y, width, height,
            win_gravity, winclient->old_bw);
        return true;
    } else if (ce.message_type == m_net->restack_window) {

        fbdbg << "Ewmh: restack window" << endl;

        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;

        // ce.data.l[0] = source indication
        // ce.data.l[1] = sibling window
        // ce.data.l[2] = detail


        WinClient *above_win = Fluxbox::instance()->searchWindow(ce.data.l[1]);
        if (above_win == 0 || above_win->fbwindow() == 0 ||
            above_win == winclient) // this would be very wrong :)
            return true;

        FbTk::LayerItem &below_item = winclient->fbwindow()->layerItem();
        FbTk::LayerItem &above_item = above_win->fbwindow()->layerItem();

        // this might break the transient_for layering

        // do restack if both items are on the same layer
        // else ignore restack
        if (&below_item.getLayer() == &above_item.getLayer())
            below_item.getLayer().stackBelowItem(below_item, &above_item);


        return true;
    } else if (ce.message_type == m_net->request_frame_extents) {
        if (!screen)
            return true;
        FbTk::ThemeProxy<FbWinFrameTheme> &theme = screen->focusedWinFrameTheme();
        unsigned int bw = theme->border().width();
        long title_h = theme->titleHeight() ? theme->titleHeight() + 2*bw : 
                theme->font().height() + 2*theme->bevelWidth() + 2 + 2*bw;
        long handle_h = theme->handleWidth() + 2*bw;
        long extents[4];
        // our frames currently don't protrude from left/right
        extents[0] = bw;
        extents[1] = bw;
        extents[2] = title_h;
        extents[3] = handle_h;

        XChangeProperty(FbTk::App::instance()->display(), ce.window,
            m_net->frame_extents, XA_CARDINAL, 32, PropModeReplace,
            (unsigned char *)extents, 4);

    } else if (ce.message_type == m_net->wm_moveresize) {
        if (winclient == 0 || winclient->fbwindow() == 0)
            return true;
        // data.l[0] = x_root
        // data.l[1] = y_root
        // data.l[2] = direction
        // data.l[3] = button
        // data.l[4] = source indication
        switch (ce.data.l[2] ) {
        case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:
        case _NET_WM_MOVERESIZE_SIZE_TOP:
        case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
        case _NET_WM_MOVERESIZE_SIZE_RIGHT:
        case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
        case _NET_WM_MOVERESIZE_SIZE_BOTTOM:
        case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
        case _NET_WM_MOVERESIZE_SIZE_LEFT:
        case _NET_WM_MOVERESIZE_SIZE_KEYBOARD:
            // startResizing uses relative coordinates
            winclient->fbwindow()->startResizing(ce.data.l[0] -
                winclient->fbwindow()->x() -
                winclient->fbwindow()->frame().window().borderWidth(),
                ce.data.l[1] - winclient->fbwindow()->y() -
                winclient->fbwindow()->frame().window().borderWidth(),
                static_cast<FluxboxWindow::ReferenceCorner>(ce.data.l[2]));
            break;
        case _NET_WM_MOVERESIZE_MOVE:
        case _NET_WM_MOVERESIZE_MOVE_KEYBOARD:
            winclient->fbwindow()->startMoving(ce.data.l[0], ce.data.l[1]);
            break;
        case _NET_WM_MOVERESIZE_CANCEL:
            if (winclient->fbwindow()->isMoving())
                winclient->fbwindow()->stopMoving(true);
            if (winclient->fbwindow()->isResizing())
                winclient->fbwindow()->stopResizing(true);
            break;
        default:
            cerr << "Ewmh: Unknown move/resize direction: " << ce.data.l[2] << endl;
            break;
        }
        return true;
    }

    // we didn't handle the ce.message_type here
    return false;
}


bool Ewmh::propertyNotify(WinClient &winclient, Atom the_property) {
    if (the_property == m_net->wm_strut) {
        updateStrut(winclient);
        return true;
    } else if (the_property == m_net->wm_name) {
        FbTk::FbString newtitle = winclient.textProperty(the_property);
        if (!newtitle.empty())
            winclient.setTitle(newtitle);
        if (winclient.fbwindow())
            winclient.fbwindow()->titleSig().emit(newtitle, *winclient.fbwindow());
        return true;
    } else if (the_property == m_net->wm_icon_name) {
        // we don't use icon title, since we don't show icons
        return true;
    } else if (the_property == m_net->wm_icon) {
        extractNetWmIcon(m_net->wm_icon, winclient);
        return true;
    }

    return false;
}

// wrapper for real setState, since most operations don't need the client
void Ewmh::setState(FluxboxWindow &win, Atom state, bool value) {
    setState(win, state, value, win.winClient());
}

// wrapper for real toggleState, since most operations don't need the client
void Ewmh::toggleState(FluxboxWindow &win, Atom state) {
    toggleState(win, state, win.winClient());
}

// set window state
void Ewmh::setState(FluxboxWindow &win, Atom state, bool value,
                    WinClient &client) {
    if (state == m_net->wm_state_sticky) { // STICKY
        if ((value && !win.isStuck()) ||
            (!value && win.isStuck()))
            win.stick();
    } else if (state == m_net->wm_state_shaded) { // SHADED
        if ((value && !win.isShaded()) ||
            (!value && win.isShaded()))
            win.shade();
    }  else if (state == m_net->wm_state_maximized_horz ) { // maximized Horizontal
        if (value ^ win.isMaximizedHorz())
            win.maximizeHorizontal();
    } else if (state == m_net->wm_state_maximized_vert) { // maximized Vertical
        if (value ^ win.isMaximizedVert())
            win.maximizeVertical();
    } else if (state == m_net->wm_state_fullscreen) { // fullscreen
        if ((value && !win.isFullscreen()) ||
            (!value && win.isFullscreen()))
        win.setFullscreen(value);
    } else if (state == m_net->wm_state_hidden) { // minimized
        if (value && !win.isIconic())
            win.iconify();
        else if (!value && win.isIconic())
            win.deiconify();
    } else if (state == m_net->wm_state_skip_taskbar) { // skip taskbar
        win.setIconHidden(value);
    } else if (state == m_net->wm_state_below) {  // bottom layer
        if (value)
            win.moveToLayer(ResourceLayer::BOTTOM);
        else if (win.layerNum() > ResourceLayer::NORMAL)
            win.moveToLayer(ResourceLayer::NORMAL);
    } else if (state == m_net->wm_state_above) { // above layer
        if (value)
            win.moveToLayer(ResourceLayer::ABOVE_DOCK);
        else if (win.layerNum() < ResourceLayer::NORMAL)
            win.moveToLayer(ResourceLayer::NORMAL);
    } else if (state == m_net->wm_state_demands_attention) {
        if (value) { // if add attention
            Fluxbox::instance()->attentionHandler().addAttention(client);
        } else { // erase it
            Fluxbox::instance()->attentionHandler().removeWindow(client);
        }
    } else if (state == m_net->wm_state_modal) {
        client.setStateModal(value);
    }

}

// toggle window state
void Ewmh::toggleState(FluxboxWindow &win, Atom state, WinClient &client) {
    if (state == m_net->wm_state_sticky) { // sticky
        win.stick();
    } else if (state == m_net->wm_state_shaded){ // shaded
        win.shade();
    } else if (state == m_net->wm_state_maximized_horz ) { // maximized Horizontal
        win.maximizeHorizontal();
    } else if (state == m_net->wm_state_maximized_vert) { // maximized Vertical
        win.maximizeVertical();
    } else if (state == m_net->wm_state_fullscreen) { // fullscreen
        win.setFullscreen(!win.isFullscreen()); // toggle current state
    } else if (state == m_net->wm_state_hidden) { // minimized
        if(win.isIconic())
            win.deiconify();
        else
            win.iconify();
    } else if (state == m_net->wm_state_skip_taskbar) { // taskbar
        win.setIconHidden(!win.isIconHidden());
    } else if (state == m_net->wm_state_below) { // bottom layer
        if (win.layerNum() == ResourceLayer::BOTTOM)
            win.moveToLayer(ResourceLayer::NORMAL);
        else
            win.moveToLayer(ResourceLayer::BOTTOM);

    } else if (state == m_net->wm_state_above) { // top layer
        if (win.layerNum() == ResourceLayer::ABOVE_DOCK)
            win.moveToLayer(ResourceLayer::NORMAL);
        else
            win.moveToLayer(ResourceLayer::ABOVE_DOCK);
    } else if (state == m_net->wm_state_modal) { // modal
        client.setStateModal(!client.isStateModal());
    }

}


void Ewmh::updateStrut(WinClient &winclient) {
    Atom ret_type = 0;
    int fmt = 0;
    unsigned long nitems = 0, bytes_after = 0;
    long *data = 0;
    if (winclient.property(m_net->wm_strut, 0, 4, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {

        int head = winclient.screen().getHead(winclient);
        winclient.setStrut(winclient.screen().requestStrut(head,
                           data[0], data[1],
                           data[2], data[3]));
        winclient.screen().updateAvailableWorkspaceArea();
    } else {
        winclient.clearStrut();
    }
}



void Ewmh::updateActions(FluxboxWindow &win) {

    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WM_ALLOWED_ACTIONS, ATOM[]
     *
     * A list of atoms indicating user operations that the
     * Window Manager supports for this window. Atoms present  in the
     * list indicate allowed actions, atoms not present in the list
     * indicate actions that are not supported for this window. The
     * Window Manager MUST keep this property updated to reflect the
     * actions which are currently "active" or "sensitive" for a window.
     * Taskbars, Pagers, and other tools use _NET_WM_ALLOWED_ACTIONS to
     * decide which actions should be made available to the user.
     */

    typedef vector<Atom> ActionsVector;
    ActionsVector actions;
    actions.reserve(10);
    // all windows can change desktop,
    // be shaded or be sticky
    actions.push_back(m_net->wm_action_change_desktop);
    actions.push_back(m_net->wm_action_shade);
    actions.push_back(m_net->wm_action_stick);

    if (win.isResizable())
        actions.push_back(m_net->wm_action_resize);
    if (win.isMoveable())
        actions.push_back(m_net->wm_action_move);
    if (win.isClosable())
        actions.push_back(m_net->wm_action_close);
    if (win.isIconifiable())
        actions.push_back(m_net->wm_action_minimize);

    unsigned int max_width, max_height;
    win.getMaxSize(&max_width, &max_height);

    // if unlimited max width we can maximize horizontal
    if (max_width == 0) {
        actions.push_back(m_net->wm_action_maximize_horz);
    }
    // if unlimited max height we can maxmize vert
    if (max_height == 0) {
        actions.push_back(m_net->wm_action_maximize_vert);
    }

    // if we have unlimited size in all directions we can have this window
    // in fullscreen mode
    if (max_height == 0 && max_width == 0) {
        actions.push_back(m_net->wm_action_fullscreen);
    }



    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        (*it)->changeProperty(m_net->wm_allowed_actions, XA_ATOM, 32, PropModeReplace,
                              reinterpret_cast<unsigned char*>(&actions.front()),
                              actions.size());
    }

}

void Ewmh::setupState(FluxboxWindow &win) {
    /* From Extended Window Manager Hints, draft 1.3:
     *
     * _NET_WM_STATE, ATOM[]
     *
     * A list of hints describing the window state. Atoms present in
     * the list MUST be considered set, atoms not present in the list
     * MUST be considered not set. The Window Manager SHOULD honor
     * _NET_WM_STATE whenever a withdrawn window requests to be mapped.
     * A Client wishing to change the state of a window MUST send a
     * _NET_WM_STATE client message to the root window (see below).
     * The Window Manager MUST keep this property updated to reflect
     * the current state of the window.
     *
     * The Window Manager should remove the property whenever a window
     * is withdrawn, but it should leave the property in place when it
     * is shutting down, e.g. in response to losing ownership of the
     * WM_Sn manager selection.
     */
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    unsigned char *data = 0;

    win.winClient().property(m_net->wm_state, 0, 0x7fffffff, False, XA_ATOM,
                             &ret_type, &fmt, &nitems, &bytes_after,
                             &data);
    if (data) {
        Atom *states = (Atom *)data;
        for (unsigned long i=0; i < nitems; ++i)
            setState(win, states[i], true);

        XFree(data);
    }
}

void Ewmh::updateFrameExtents(FluxboxWindow &win) {
    /* Frame extents are basically the amount the window manager frame
       protrudes from the client window, on left, right, top, bottom
       (it is independent of window position).
     */

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        long extents[4];
        // our frames currently don't protrude from left/right
        int bw = win.frame().window().borderWidth();
        extents[0] = bw;
        extents[1] = bw;
        extents[2] = win.frame().titlebarHeight() + bw;
        extents[3] = win.frame().handleHeight() + bw;

        (*it)->changeProperty(m_net->frame_extents,
                              XA_CARDINAL, 32, PropModeReplace,
                              (unsigned char *)extents, 4);
    }
}

