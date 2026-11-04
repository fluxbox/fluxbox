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
#include <map>
#include <vector>
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

// long_length for XGetWindowProperty(): read the whole property
const long PROPERTY_MAX_LENGTH = 0x7fffffff;

// each icon in _NET_WM_ICON starts with its width and height
const unsigned long NET_WM_ICON_HEADER = 2;

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
    for (i = 0; i + NET_WM_ICON_HEADER < nr_icon_data; i += width * height ) {

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

    // The one icon serves the iconbar, the titlebar and the menus,
    // mostly at 14 to 24 pixels. FbPixmap::scale() averages pixels when
    // shrinking but can only repeat them when growing, so pick the
    // smallest icon of at least 32 pixels; if there is none, the
    // biggest one. 32 rather than 48: 32 -> 16 is an exact 2:1 that
    // keeps 1 pixel outlines, while 48 -> 16 (3:1) can drop an outline
    // that sits off the 3 pixel grid (seen with ghostty's bezel), and
    // 32 pixel icons tend to have less empty margin. icon_data is
    // sorted by width, then height.
    const unsigned long wanted_size = 32;
    // the mask has only 1 bit: pixels at least half opaque are drawn
    const unsigned char alpha_opaque = 128;
    IconContainer::const_iterator best = icon_data.begin();
    for (IconContainer::const_iterator it = icon_data.begin();
         it != icon_data.end(); ++it) {
        best = it;
        if (static_cast<unsigned long>(it->first.first) >= wanted_size &&
            static_cast<unsigned long>(it->first.second) >= wanted_size) {
            break;
        }
    }
    width = best->first.first;
    height = best->first.second;

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

    // allocate some memory for the icons at client side. zeroed: the
    // loop below sets only the pixels, not the padding at the end of
    // each scanline, and XPutImage() sends the padding as well.
    img_pm->data = static_cast<char*>(calloc(img_pm->bytes_per_line * height, 1));
    img_mask->data = static_cast<char*>(calloc(img_mask->bytes_per_line * height, 1));


    const unsigned long* src = best->second;
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
            // XYBitmap: XPutImage() draws a 0 bit in the background
            // of the default GC, which is 1, so 0 means opaque
            XPutPixel(img_mask, x, y, a >= alpha_opaque ? 0 : 1);
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

// _NET_WM_FULLSCREEN_MONITORS: one monitor index per edge (top, bottom,
// left, right), see WindowState::FullscreenEdge
const int FULLSCREEN_EDGES = WindowState::FULLSCREEN_EDGES;

// the largest monitor index taken from a client. It only keeps the
// CARDINALs well inside int; whether the monitors exist is checked by the
// window.
const long FULLSCREEN_MONITOR_MAX = 0xffff;

bool toMonitorIndices(const long *l, int mon[FULLSCREEN_EDGES]) {
    for (int i = 0; i < FULLSCREEN_EDGES; ++i) {
        if (l[i] < 0 || l[i] > FULLSCREEN_MONITOR_MAX)
            return false;
        mon[i] = static_cast<int>(l[i]);
    }
    return true;
}

bool setFullscreenMonitors(FluxboxWindow &win, const int mon[FULLSCREEN_EDGES]) {
    return win.setFullscreenMonitors(mon[WindowState::FULLSCREEN_TOP],
                                     mon[WindowState::FULLSCREEN_BOTTOM],
                                     mon[WindowState::FULLSCREEN_LEFT],
                                     mon[WindowState::FULLSCREEN_RIGHT]);
}

/* From Extended Window Manager Hints, 1.5:
 *
 * _NET_WM_STRUT_PARTIAL, left, right, top, bottom, left_start_y,
 * left_end_y, right_start_y, right_end_y, top_start_x, top_end_x,
 * bottom_start_x, bottom_end_x,CARDINAL[12]/32
 *
 * Struts MUST be specified in root window coordinates, that is, they
 * are not relative to the edges of any view port or Xinerama monitor.
 *
 * Fluxbox keeps struts per head, measured from the head's edges. A
 * strut edge reserves space on a head when the reserved band ends
 * inside that head and its start/end range overlaps the head. A panel
 * on an inner edge between two heads thus only reserves space on the
 * head it sits on, and never swallows the whole neighbouring head.
 */
enum { SP_LEFT, SP_RIGHT, SP_TOP, SP_BOTTOM,
       SP_LEFT_START, SP_LEFT_END, SP_RIGHT_START, SP_RIGHT_END,
       SP_TOP_START, SP_TOP_END, SP_BOTTOM_START, SP_BOTTOM_END };

// Pixel ranges on one root-window axis (x for top/bottom struts,
// y for left/right struts):
//   strut: [strut_first, strut_last]  inclusive, as in _NET_WM_STRUT_PARTIAL
//   head:  [head_origin, head_origin + head_length)
bool strutSpanOverlapsHead(long strut_first, long strut_last,
                           int head_origin, int head_length) {
    return (strut_first < (head_origin + head_length)) && (strut_last >= head_origin);
}

// head rectangle in root window pixels
struct HeadGeometry {
    HeadGeometry(const BScreen &screen, int head):
        x(screen.getHeadX(head)), y(screen.getHeadY(head)),
        width(screen.getHeadWidth(head)), height(screen.getHeadHeight(head)) { }

    int x, y, width, height;
};

// space reserved at each edge of a head, in pixels from that edge
struct EdgeInsets {
    int left = 0, right = 0, top = 0, bottom = 0;

    bool any() const { return left || right || top || bottom; }
};

Strut *requestPartialStrut(BScreen &screen, const long *sp) {
    const long sw = screen.width(), sh = screen.height();
    const int nh = screen.numHeads();
    Strut *strut = 0;

    // without Xinerama, head 0 is the whole root window
    for (int head = (nh ? 1 : 0); head <= nh; ++head) {
        const HeadGeometry h(screen, head);
        EdgeInsets insets;

        if ((sp[SP_LEFT] > h.x) && (sp[SP_LEFT] <= (h.x + h.width)) &&
            strutSpanOverlapsHead(sp[SP_LEFT_START], sp[SP_LEFT_END], h.y, h.height))
            insets.left = sp[SP_LEFT] - h.x;

        if (((sw - sp[SP_RIGHT]) >= h.x) && ((sw - sp[SP_RIGHT]) < (h.x + h.width)) &&
            strutSpanOverlapsHead(sp[SP_RIGHT_START], sp[SP_RIGHT_END], h.y, h.height))
            insets.right = (h.x + h.width) - (sw - sp[SP_RIGHT]);

        if ((sp[SP_TOP] > h.y) && (sp[SP_TOP] <= (h.y + h.height)) &&
            strutSpanOverlapsHead(sp[SP_TOP_START], sp[SP_TOP_END], h.x, h.width))
            insets.top = sp[SP_TOP] - h.y;

        if (((sh - sp[SP_BOTTOM]) >= h.y) && ((sh - sp[SP_BOTTOM]) < (h.y + h.height)) &&
            strutSpanOverlapsHead(sp[SP_BOTTOM_START], sp[SP_BOTTOM_END], h.x, h.width))
            insets.bottom = (h.y + h.height) - (sh - sp[SP_BOTTOM]);

        if (insets.any())
            strut = screen.requestStrut(head, insets.left, insets.right,
                                        insets.top, insets.bottom, strut);
    }
    return strut;
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
        wm_fullscreen_monitors = XInternAtom(disp, "_NET_WM_FULLSCREEN_MONITORS", False);

        properties = XInternAtom(disp, "_NET_PROPERTIES", False);
        wm_name = XInternAtom(disp, "_NET_WM_NAME", False);
        wm_visible_name = XInternAtom(disp, "_NET_WM_VISIBLE_NAME", False);
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
        wm_window_type_popup_menu = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
        wm_window_type_tooltip = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_TOOLTIP", False);
        wm_window_type_notification = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
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
        wm_strut_partial = XInternAtom(disp, "_NET_WM_STRUT_PARTIAL", False);
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
         wm_moveresize,
         wm_fullscreen_monitors;

    // application window properties
    Atom properties,
         wm_name,
         wm_visible_name,
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
         wm_window_type_popup_menu,
         wm_window_type_tooltip,
         wm_window_type_notification,
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
         wm_strut_partial,
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

// data.l[0] of _NET_MOVERESIZE_WINDOW: gravity in the low byte, then
// one bit per value the client wants changed
enum EwmhMoveResizeWindowFlags {
    _NET_MOVERESIZE_WINDOW_GRAVITY = 0xFF,
    _NET_MOVERESIZE_WINDOW_X       = 1 << 8,
    _NET_MOVERESIZE_WINDOW_Y       = 1 << 9,
    _NET_MOVERESIZE_WINDOW_WIDTH   = 1 << 10,
    _NET_MOVERESIZE_WINDOW_HEIGHT  = 1 << 11
};

// source indication in client messages
enum EwmhSourceIndication {
    EWMH_SOURCE_NONE        = 0,    // client of an older spec
    EWMH_SOURCE_APPLICATION = 1,
    EWMH_SOURCE_PAGER       = 2     // pagers, taskbars: acting for the user
};

// _NET_WM_DESKTOP 0xFFFFFFFF: the window is on all desktops. Xlib
// sign-extends 32 bit client message data into long, so it reads -1.
const long EWMH_ALL_DESKTOPS = -1;

// order of the CARDINAL[4] in _NET_FRAME_EXTENTS and _NET_WM_STRUT
enum EwmhSide {
    EWMH_LEFT = 0,
    EWMH_RIGHT,
    EWMH_TOP,
    EWMH_BOTTOM,
    EWMH_NUM_SIDES
};

// order of the CARDINAL[4] per desktop in _NET_WORKAREA
enum EwmhGeometry {
    EWMH_X = 0,
    EWMH_Y,
    EWMH_WIDTH,
    EWMH_HEIGHT,
    EWMH_NUM_GEOMETRY
};

Ewmh::Ewmh() {
    setName("ewmh");
    m_net = new EwmhAtoms;

    // timers run only once all pending X events are handled, so a burst
    // of restacks (e.g. a client flooding ConfigureRequests) writes
    // _NET_CLIENT_LIST_STACKING once. it also keeps the rebuild out of
    // the middle of a window's destruction, where the layers emit too.
    m_stacking_timer.setTimeout(0);
    m_stacking_timer.fireOnce(true);
    m_stacking_timer.setFunctor([this]() { updateStackingLists(); });
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
        m_net->wm_strut_partial,
        m_net->wm_state,
        m_net->wm_name,
        m_net->wm_visible_name,
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
        m_net->wm_window_type_popup_menu,
        m_net->wm_window_type_tooltip,
        m_net->wm_window_type_notification,
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
        m_net->wm_fullscreen_monitors,

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
    updateWorkarea(screen);

    // only remembers the screen; screens get deleted after the event loop
    // ended (Fluxbox::~Fluxbox()), so no timer runs for a deleted one
    join(screen.layerManager().stackingSig(),
         [this, &screen]() { stackingChanged(screen); });

    // struts are kept per head: after a RandR change map them again
    join(screen.headsChangedSig(),
         [this](BScreen &scr) { updateStruts(scr); });
}

void Ewmh::updateStruts(BScreen &screen) {
    const list<Focusable *> &clients =
            screen.focusControl().creationOrderList().clientList();
    for (list<Focusable *>::const_iterator it = clients.begin();
            it != clients.end(); ++it) {
        WinClient *client = dynamic_cast<WinClient *>(*it);
        if (client)
            updateStrut(*client);
    }
}

void Ewmh::setupClient(WinClient &winclient) {
    updateStrut(winclient);

    FbTk::FbString newtitle = winclient.textProperty(m_net->wm_name);
    if (!newtitle.empty())
        winclient.setTitle(newtitle);

    // the signal lives in winclient, the slot dies together with it
    join(winclient.titleSig(),
         [this, &winclient](const std::string &, Focusable &) {
             updateVisibleName(winclient);
         });

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

    winclient.property(m_net->wm_window_type, 0, PROPERTY_MAX_LENGTH, False, XA_ATOM,
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
            else if (atoms[l] == m_net->wm_window_type_popup_menu)
                type = WindowState::TYPE_POPUP_MENU;
            else if (atoms[l] == m_net->wm_window_type_tooltip)
                type = WindowState::TYPE_TOOLTIP;
            else if (atoms[l] == m_net->wm_window_type_notification)
                type = WindowState::TYPE_NOTIFICATION;
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

/**
 * _NET_WM_VISIBLE_NAME, UTF8_STRING
 *
 * If the Window Manager displays a window name other than _NET_WM_NAME
 * the Window Manager MUST set this to the title displayed in UTF-8
 * encoding.
 *
 * The title differs from what the client asked for when the user renamed
 * the window (SetTitle, SetTitleDialog) or when fluxbox shortened it. Compare against _NET_WM_NAME and, for clients without it,
 * WM_NAME, so that plain WM_NAME clients do not get a redundant copy.
 */
void Ewmh::updateVisibleName(WinClient &winclient) {
    const std::string &title = winclient.title().logical();

    bool exists = false;
    FbTk::FbString client_name = winclient.textProperty(m_net->wm_name, &exists);
    if (!exists) {
        client_name = winclient.textProperty(XA_WM_NAME);
    }

    if (title.empty() || title == client_name) {
        winclient.deleteProperty(m_net->wm_visible_name);
        return;
    }

    // fluxbox keeps titles in UTF-8 internally
    winclient.changeProperty(m_net->wm_visible_name, m_net->utf8_string, 8,
                             PropModeReplace,
                             reinterpret_cast<unsigned char*>(const_cast<char*>(title.data())),
                             title.size());
}

void Ewmh::setupFrame(FluxboxWindow &win) {
    // before setupState(): any state change rewrites (or deletes)
    // _NET_WM_FULLSCREEN_MONITORS from what we know about the window
    setupFullscreenMonitors(win);
    setupState(win);
    bool exists;
    unsigned int desktop=static_cast<unsigned int>(win.winClient().cardinalProperty(m_net->wm_desktop, &exists));
    if (exists) {
        if (desktop == static_cast<unsigned int>(EWMH_ALL_DESKTOPS) && !win.isStuck())
            win.stick();
        else
            win.setWorkspace(desktop);
    } else {
        updateWorkspace(win);
    }

    updateFrameExtents(win);

    // switching tabs leaves the layers alone, but changes which client is
    // the top one in _NET_CLIENT_LIST_STACKING. setCurrentClient() emits
    // the frame's titleSig() for that, focused or not. the signal lives in
    // win, the slot dies together with it
    join(win.titleSig(),
         [this, &win](const std::string &, Focusable &) {
             stackingChanged(win.screen());
         });
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

    delete [] wl;

    // new and gone clients change _NET_CLIENT_LIST_STACKING as well
    stackingChanged(screen);
}

void Ewmh::stackingChanged(BScreen &screen) {
    m_stacking_screens.insert(&screen);
    m_stacking_timer.start(); // does nothing if already started
}

void Ewmh::updateStackingLists() {
    std::set<BScreen *> screens;
    screens.swap(m_stacking_screens);

    std::set<BScreen *>::iterator it = screens.begin();
    for (; it != screens.end(); ++it) {
        updateClientListStacking(**it);
    }
}

void Ewmh::updateClientListStacking(BScreen &screen) {

    if (screen.isShuttingdown()) {
        return;
    }

    // the layers hold LayerItems, find the window behind each
    typedef std::map<const FbTk::LayerItem *, const FluxboxWindow *> ItemWindows;
    ItemWindows windows;
    const list<Focusable *> &winlist =
            screen.focusControl().creationOrderWinList().clientList();
    list<Focusable *>::const_iterator win_it = winlist.begin();
    for (; win_it != winlist.end(); ++win_it) {
        const FluxboxWindow *fbwin = (*win_it)->fbwindow();
        if (fbwin && fbwin->numClients() > 0) {
            windows[&fbwin->layerItem()] = fbwin;
        }
    }

    // layer 0 is the top one, and each layer lists its items top first
    vector<Window> stacking;
    const FbTk::MultLayers &layers = screen.layerManager();
    for (size_t l = layers.numLayers(); l > 0; --l) {
        const FbTk::Layer::ItemList &items = layers.getLayer(l - 1)->itemList();
        FbTk::Layer::ItemList::const_reverse_iterator item_it = items.rbegin();
        for (; item_it != items.rend(); ++item_it) {

            ItemWindows::const_iterator found = windows.find(*item_it);
            if (found == windows.end()) {
                continue; // the slit, the toolbar, ...
            }

            // the current client is the one shown, the other tabs are
            // below it
            const FluxboxWindow &fbwin = *found->second;
            const WinClient *current = &fbwin.winClient();
            FluxboxWindow::ClientList::const_iterator client_it = fbwin.clientList().begin();
            for (; client_it != fbwin.clientList().end(); ++client_it) {
                if (*client_it != current) {
                    stacking.push_back((*client_it)->window());
                }
            }
            stacking.push_back(current->window());
        }
    }

    unsigned char *data = 0;
    if (!stacking.empty()) {
        data = reinterpret_cast<unsigned char *>(&stacking[0]);
    }
    screen.rootWindow().changeProperty(m_net->client_list_stacking,
                                       XA_WINDOW, 32,
                                       PropModeReplace, data, stacking.size());
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
    // value-initialized, so the extra last name stays NULL
    vector<const char*> names(number_of_desks + 1);

    for (size_t i = 0; i < number_of_desks; i++) {
        names[i] = workspacenames[i].c_str();
    }

#ifdef X_HAVE_UTF8_STRING
    int code = Xutf8TextListToTextProperty(FbTk::App::instance()->display(),
                                const_cast<char**>(&names[0]), names.size(), XUTF8StringStyle, &text);
    if (code != XNoMemory && code != XLocaleNotSupported) {
        XSetTextProperty(FbTk::App::instance()->display(),
                         screen.rootWindow().window(),
                         &text, m_net->desktop_names);
        XFree(text.value);
    }
#else
    if (XStringListToTextProperty(const_cast<char**>(&names[0]), names.size(), &text)) {
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
                                       (unsigned char *)value,
                                       sizeof(value) / sizeof(value[0]));
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
                                       (unsigned char *)value,
                                       sizeof(value) / sizeof(value[0]));

}

void Ewmh::updateWorkarea(BScreen &screen) {
    // screen resize and workspace-area signals share this update path
    updateGeometry(screen);

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
    const unsigned int nr_coords = EWMH_NUM_GEOMETRY * screen.numberOfWorkspaces();
    unsigned long *coords = new unsigned long[nr_coords];
    for (unsigned int i = 0; i < nr_coords; i += EWMH_NUM_GEOMETRY) {
        coords[i + EWMH_X] = screen.maxLeft(0);
        coords[i + EWMH_Y] = screen.maxTop(0);
        coords[i + EWMH_WIDTH] = screen.maxRight(0) - screen.maxLeft(0);
        coords[i + EWMH_HEIGHT] = screen.maxBottom(0) - screen.maxTop(0);
    }
    screen.rootWindow().changeProperty(m_net->workarea,
                                       XA_CARDINAL, 32,
                                       PropModeReplace,
                                       (unsigned char *)coords,
                                       nr_coords);

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

    long monitors[FULLSCREEN_EDGES] = { 0 };
    if (win.hasFullscreenMonitors()) {
        for (int i = 0; i < FULLSCREEN_EDGES; ++i)
            monitors[i] = win.fullscreenMonitors()[i];
    }

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {

        /* _NET_WM_FULLSCREEN_MONITORS, CARDINAL[4]/32
         *
         * The Window Manager MUST keep this list updated to reflect the
         * current state of the window.
         */
        if (win.hasFullscreenMonitors())
            (*it)->changeProperty(m_net->wm_fullscreen_monitors, XA_CARDINAL,
                                  32, PropModeReplace,
                                  reinterpret_cast<unsigned char*>(monitors),
                                  FULLSCREEN_EDGES);
        else
            (*it)->deleteProperty(m_net->wm_fullscreen_monitors);

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
        (*it)->property(m_net->wm_state, 0, PROPERTY_MAX_LENGTH, False, XA_ATOM,
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
        workspace = EWMH_ALL_DESKTOPS;

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
        if (ce.data.l[0] == EWMH_ALL_DESKTOPS) {
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
        // or more than the screen allows
        if (screen->numberOfWorkspaces() == static_cast<unsigned int>(ce.data.l[0]) ||
            ce.data.l[0] < 0 ||
            ce.data.l[0] > static_cast<long>(BScreen::MAX_WORKSPACES))
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

        // ce.data.l[0] = source indication
        if (winclient->fbwindow() && (ce.data.l[0] == EWMH_SOURCE_PAGER ||
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
        const long flags = ce.data.l[0];
        int x = (flags & _NET_MOVERESIZE_WINDOW_X) ? ce.data.l[1] :
            winclient->fbwindow()->x();
        int y = (flags & _NET_MOVERESIZE_WINDOW_Y) ? ce.data.l[2] :
            winclient->fbwindow()->y();
        unsigned int width = (flags & _NET_MOVERESIZE_WINDOW_WIDTH) ? ce.data.l[3] :
            winclient->width();
        unsigned int height = (flags & _NET_MOVERESIZE_WINDOW_HEIGHT) ? ce.data.l[4] :
            winclient->height();
        int win_gravity = flags & _NET_MOVERESIZE_WINDOW_GRAVITY;
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
        long title_h = theme->effectiveTitleHeight() + 2*bw;
        long handle_h = theme->handleWidth() + 2*bw;
        long extents[EWMH_NUM_SIDES];
        // our frames currently don't protrude from left/right
        extents[EWMH_LEFT] = bw;
        extents[EWMH_RIGHT] = bw;
        extents[EWMH_TOP] = title_h;
        extents[EWMH_BOTTOM] = handle_h;

        XChangeProperty(FbTk::App::instance()->display(), ce.window,
            m_net->frame_extents, XA_CARDINAL, 32, PropModeReplace,
            (unsigned char *)extents, EWMH_NUM_SIDES);

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
    } else if (ce.message_type == m_net->wm_fullscreen_monitors) {
        // data.l[FULLSCREEN_TOP ... FULLSCREEN_RIGHT] = the monitor
        // defining that edge; data.l[FULLSCREEN_EDGES] = source indication
        const long *l = ce.data.l;
        int mon[FULLSCREEN_EDGES];
        if (!toMonitorIndices(l, mon)) {
            fbdbg << "Ewmh: ignoring invalid _NET_WM_FULLSCREEN_MONITORS "
                  << l[WindowState::FULLSCREEN_TOP] << " "
                  << l[WindowState::FULLSCREEN_BOTTOM] << " "
                  << l[WindowState::FULLSCREEN_LEFT] << " "
                  << l[WindowState::FULLSCREEN_RIGHT] << endl;
            return true;
        }

        if (winclient == 0 || winclient->fbwindow() == 0) {
            if (ce.window == None ||
                (screen != 0 && ce.window == screen->rootWindow().window())) {
                return true;
            }
            // not managed yet: e.g. mpv --fs-screen=all sends this once,
            // right before it maps its window. Keep the request in the
            // property, setupFullscreenMonitors() picks it up on manage.
            long monitors[FULLSCREEN_EDGES] = {
                mon[WindowState::FULLSCREEN_TOP], mon[WindowState::FULLSCREEN_BOTTOM],
                mon[WindowState::FULLSCREEN_LEFT], mon[WindowState::FULLSCREEN_RIGHT] };
            XChangeProperty(FbTk::App::instance()->display(), ce.window,
                            m_net->wm_fullscreen_monitors, XA_CARDINAL, 32,
                            PropModeReplace,
                            reinterpret_cast<unsigned char*>(monitors),
                            FULLSCREEN_EDGES);
            return true;
        }

        if (!setFullscreenMonitors(*winclient->fbwindow(), mon)) {
            fbdbg << "Ewmh: ignoring _NET_WM_FULLSCREEN_MONITORS for missing monitors "
                  << l[WindowState::FULLSCREEN_TOP] << " "
                  << l[WindowState::FULLSCREEN_BOTTOM] << " "
                  << l[WindowState::FULLSCREEN_LEFT] << " "
                  << l[WindowState::FULLSCREEN_RIGHT] << endl;
        }
        return true;
    }

    // we didn't handle the ce.message_type here
    return false;
}


bool Ewmh::propertyNotify(WinClient &winclient, Atom the_property) {
    if (the_property == m_net->wm_strut ||
        the_property == m_net->wm_strut_partial) {
        updateStrut(winclient);
        return true;
    } else if (the_property == m_net->wm_name) {
        FbTk::FbString newtitle = winclient.textProperty(the_property);
        if (!newtitle.empty())
            winclient.setTitle(newtitle);
        if (winclient.fbwindow())
            winclient.fbwindow()->titleSig().emit(winclient.title().logical(),
                                                  *winclient.fbwindow());
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

    // the spec: a client that sets _NET_WM_STRUT_PARTIAL also sets
    // _NET_WM_STRUT for older WMs, and the partial one takes precedence.
    if (winclient.property(m_net->wm_strut_partial, 0, 12, False, XA_CARDINAL,
                           &ret_type, &fmt, &nitems, &bytes_after,
                           (unsigned char **) &data) && data) {
        if (nitems >= 12) {
            Strut *strut = requestPartialStrut(winclient.screen(), data);
            XFree(data);
            if (strut) {
                winclient.setStrut(strut);
                winclient.screen().updateAvailableWorkspaceArea();
            } else {
                winclient.clearStrut();
            }
            return;
        }
        XFree(data);
        data = 0;
    }

    if (winclient.property(m_net->wm_strut, 0, EWMH_NUM_SIDES, False, XA_CARDINAL,
                                 &ret_type, &fmt, &nitems, &bytes_after,
                                 (unsigned char **) &data) && data) {

        if (nitems >= EWMH_NUM_SIDES) {
            int head = winclient.screen().getHead(winclient);
            winclient.setStrut(winclient.screen().requestStrut(head,
                               data[EWMH_LEFT], data[EWMH_RIGHT],
                               data[EWMH_TOP], data[EWMH_BOTTOM]));
            winclient.screen().updateAvailableWorkspaceArea();
        } else {
            winclient.clearStrut();
        }
        XFree(data);
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

    win.winClient().property(m_net->wm_state, 0, PROPERTY_MAX_LENGTH, False, XA_ATOM,
                             &ret_type, &fmt, &nitems, &bytes_after,
                             &data);
    if (data) {
        Atom *states = (Atom *)data;
        for (unsigned long i=0; i < nitems; ++i)
            setState(win, states[i], true);

        XFree(data);
    }
}

void Ewmh::setupFullscreenMonitors(FluxboxWindow &win) {
    // the property is read-only for clients, but honor it when it is
    // already there, e.g. when we restart or replace another WM
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    unsigned char *data = 0;

    win.winClient().property(m_net->wm_fullscreen_monitors, 0, FULLSCREEN_EDGES,
                             False, XA_CARDINAL, &ret_type, &fmt, &nitems,
                             &bytes_after, &data);
    if (!data)
        return;

    int mon[FULLSCREEN_EDGES];
    if (fmt == 32 && nitems == static_cast<unsigned long>(FULLSCREEN_EDGES) &&
        toMonitorIndices(reinterpret_cast<long*>(data), mon))
        setFullscreenMonitors(win, mon);
    XFree(data);
}

void Ewmh::updateFrameExtents(FluxboxWindow &win) {
    /* Frame extents are basically the amount the window manager frame
       protrudes from the client window, on left, right, top, bottom
       (it is independent of window position).
     */

    FluxboxWindow::ClientList::iterator it = win.clientList().begin();
    FluxboxWindow::ClientList::iterator it_end = win.clientList().end();
    for (; it != it_end; ++it) {
        long extents[EWMH_NUM_SIDES];
        // our frames currently don't protrude from left/right
        int bw = win.frame().window().borderWidth();
        extents[EWMH_LEFT] = bw;
        extents[EWMH_RIGHT] = bw;
        extents[EWMH_TOP] = win.frame().titlebarHeight() + bw;
        extents[EWMH_BOTTOM] = win.frame().handleHeight() + bw;

        (*it)->changeProperty(m_net->frame_extents,
                              XA_CARDINAL, 32, PropModeReplace,
                              (unsigned char *)extents, EWMH_NUM_SIDES);
    }
}

