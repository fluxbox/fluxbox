#!/usr/bin/env python3
"""Test fluxbox's _NET_WM_FULLSCREEN_MONITORS support (EWMH 1.5, section 6.3).

Commands:

  test     [--fluxbox PATH] [--keep]
           Start Xephyr with two 800x600 Xinerama heads side by side, run
           fluxbox in it (with a throw-away $HOME) and check the behavior.
           Exits 0 when all checks pass. --keep leaves everything running
           afterwards so you can poke at it.

  xephyr   [--fluxbox PATH]
           Same setup without the checks: prints DISPLAY, starts an xterm
           if there is one and waits for Ctrl-C.

  fullscreen WINDOW on|off       send _NET_WM_STATE(_FULLSCREEN)
  send       WINDOW T B L R      send _NET_WM_FULLSCREEN_MONITORS
  show       WINDOW              print the property and the geometry

WINDOW is a window id (0x... or decimal, see xwininfo/xprop). The last
three commands work on $DISPLAY.

Needs Xephyr and python3-xlib; on Fedora:
  dnf install xorg-x11-server-Xephyr python3-xlib
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

try:
    from Xlib import X, Xatom, Xutil, protocol
    from Xlib import display as xdisplay
    from Xlib.error import DisplayError, XError
except ImportError:
    sys.exit("needs python3-xlib (Fedora: dnf install python3-xlib)")

HEAD_W, HEAD_H = 800, 600
STATE_REMOVE, STATE_ADD = 0, 1
SOURCE_APP, SOURCE_PAGER = 1, 2


# --------------------------------------------------------------------------
# EWMH helpers

class Wm:
    def __init__(self, dpy):
        self.d = dpy
        self.screen = dpy.screen()
        self.root = self.screen.root

    def atom(self, name):
        return self.d.intern_atom(name)

    def window(self, wid):
        return self.d.create_resource_object("window", wid)

    def send(self, win, type_name, data):
        data = (list(data) + [0] * 5)[:5]
        ev = protocol.event.ClientMessage(window=win,
                                          client_type=self.atom(type_name),
                                          data=(32, data))
        self.root.send_event(ev, event_mask=X.SubstructureRedirectMask |
                             X.SubstructureNotifyMask)
        self.d.flush()

    def set_fullscreen(self, win, on):
        self.send(win, "_NET_WM_STATE",
                  [STATE_ADD if on else STATE_REMOVE,
                   self.atom("_NET_WM_STATE_FULLSCREEN"), 0, SOURCE_APP])

    def set_monitors(self, win, top, bottom, left, right):
        self.send(win, "_NET_WM_FULLSCREEN_MONITORS",
                  [top, bottom, left, right, SOURCE_APP])

    def activate(self, win, source):
        self.send(win, "_NET_ACTIVE_WINDOW", [source, X.CurrentTime, 0])

    def remote(self, command):
        """run a fluxbox command (needs allowRemoteActions)"""
        self.root.change_property(self.atom("_FLUXBOX_ACTION"), Xatom.STRING,
                                  8, command.encode())
        self.d.flush()

    def cardinals(self, win, name):
        p = win.get_full_property(self.atom(name), Xatom.CARDINAL)
        return list(p.value) if p else None

    def monitors(self, win):
        return self.cardinals(win, "_NET_WM_FULLSCREEN_MONITORS")

    def root_list(self, name, type_=Xatom.WINDOW):
        p = self.root.get_full_property(self.atom(name), type_)
        return list(p.value) if p else []

    def supported(self):
        return self.root_list("_NET_SUPPORTED", Xatom.ATOM)

    def active(self):
        v = self.root_list("_NET_ACTIVE_WINDOW")
        return v[0] if v else None

    def stacking(self):
        """the children of the root window (fluxbox's frames, ...) bottom to
        top, from XQueryTree. Not _NET_CLIENT_LIST_STACKING: fluxbox fills
        that in creation order."""
        return [w.id for w in self.root.query_tree().children]

    def frame_of(self, win):
        """the child of the root window holding win, i.e. its frame"""
        w = win
        while True:
            parent = w.query_tree().parent
            if parent.id == self.root.id:
                return w
            w = parent

    def geometry(self, win):
        """(x, y, w, h) of the client window in root coordinates"""
        g = win.get_geometry()
        t = self.root.translate_coords(win, 0, 0)
        return (t.x, t.y, g.width, g.height)

    def heads(self):
        try:
            r = self.d.xinerama_query_screens()
            return [(s.x, s.y, s.width, s.height) for s in r.screens]
        except Exception:
            return []

    def make_window(self, x, y, w, h, name, transient_for=None,
                    dialog=False, props=None):
        win = self.root.create_window(
            x, y, w, h, 0, self.screen.root_depth, X.InputOutput,
            X.CopyFromParent, background_pixel=self.screen.white_pixel,
            event_mask=X.StructureNotifyMask | X.PropertyChangeMask)
        win.set_wm_name(name)
        win.set_wm_class(name, "FullscreenMonitorsTest")
        win.set_wm_normal_hints(flags=Xutil.USPosition | Xutil.USSize,
                                x=x, y=y, width=w, height=h)
        if transient_for is not None:
            win.set_wm_transient_for(transient_for)
        if dialog:
            win.change_property(self.atom("_NET_WM_WINDOW_TYPE"), Xatom.ATOM,
                                32, [self.atom("_NET_WM_WINDOW_TYPE_DIALOG")])
        for pname, ptype, pvalue in props or []:
            win.change_property(self.atom(pname), ptype, 32, pvalue)
        win.map()
        self.d.flush()
        wait_for(self, lambda: win.get_attributes().map_state == X.IsViewable)
        settle(self)
        return win


def drain(wm):
    while wm.d.pending_events():
        wm.d.next_event()


def wait_for(wm, cond, timeout=3.0):
    """poll cond() until it is true or the timeout expires"""
    end = time.time() + timeout
    while True:
        wm.d.sync()
        drain(wm)
        try:
            if cond():
                return True
        except XError:
            pass
        if time.time() > end:
            return False
        time.sleep(0.05)


def settle(wm, secs=0.3):
    """give the window manager time to react to something we can't poll"""
    time.sleep(secs)
    wm.d.sync()
    drain(wm)


def on_one_head(geom):
    x, y, w, h = geom
    return (x >= 0 and x + w <= HEAD_W) or (x >= HEAD_W and x + w <= 2 * HEAD_W)


# --------------------------------------------------------------------------
# Xephyr + fluxbox sandbox

def default_fluxbox():
    repo = os.path.dirname(os.path.dirname(os.path.dirname(
        os.path.abspath(__file__))))
    for cand in (os.path.join(repo, "build", "fluxbox"),
                 os.path.join(repo, "fluxbox")):
        if os.access(cand, os.X_OK):
            return cand
    return shutil.which("fluxbox")


def free_display():
    for n in range(20, 100):
        if not os.path.exists("/tmp/.X11-unix/X%d" % n) and \
           not os.path.exists("/tmp/.X%d-lock" % n):
            return ":%d" % n
    sys.exit("no free display number")


class Sandbox:
    def __init__(self, fluxbox):
        if not fluxbox or not os.access(fluxbox, os.X_OK):
            sys.exit("fluxbox binary not found, pass --fluxbox PATH")
        if not shutil.which("Xephyr"):
            sys.exit("Xephyr not found "
                     "(Fedora: dnf install xorg-x11-server-Xephyr)")
        self.fluxbox = fluxbox
        self.display = free_display()
        self.home = tempfile.mkdtemp(prefix="fluxbox-fsmon-")
        self.log = os.path.join(self.home, "fluxbox.log")
        self.procs = []

    def start(self):
        self.procs.append(subprocess.Popen(
            ["Xephyr", self.display, "+xinerama", "-ac", "-br", "-noreset",
             "-screen", "%dx%d" % (HEAD_W, HEAD_H),
             "-screen", "%dx%d" % (HEAD_W, HEAD_H)],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL))

        dpy = None
        end = time.time() + 10
        while dpy is None:
            try:
                dpy = xdisplay.Display(self.display)
            except (DisplayError, ConnectionError, OSError):
                if time.time() > end or self.procs[0].poll() is not None:
                    self.stop()
                    sys.exit("Xephyr did not come up on %s" % self.display)
                time.sleep(0.1)
        wm = Wm(dpy)

        rcdir = os.path.join(self.home, ".fluxbox")
        os.makedirs(rcdir)
        rc = os.path.join(rcdir, "init")
        with open(rc, "w") as f:
            f.write("session.screen0.allowRemoteActions: true\n"
                    "session.screen0.focusNewWindows: true\n"
                    "session.screen0.toolbar.visible: false\n")

        env = dict(os.environ, HOME=self.home, DISPLAY=self.display)
        self.procs.append(subprocess.Popen(
            [self.fluxbox, "-display", self.display, "-rc", rc,
             "-log", self.log], env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL))

        if not wait_for(wm, lambda: wm.root_list("_NET_SUPPORTING_WM_CHECK"),
                        timeout=10):
            self.stop()
            sys.exit("fluxbox did not start, see %s" % self.log)
        settle(wm, 0.5)
        return wm

    def stop(self, keep_home=False):
        for p in reversed(self.procs):
            if p.poll() is None:
                p.terminate()
                try:
                    p.wait(3)
                except subprocess.TimeoutExpired:
                    p.kill()
        self.procs = []
        if not keep_home:
            shutil.rmtree(self.home, ignore_errors=True)

    def tail_log(self, lines=30):
        try:
            with open(self.log) as f:
                return "".join(f.readlines()[-lines:])
        except OSError:
            return "(no log)"


# --------------------------------------------------------------------------
# the checks

class Results:
    def __init__(self):
        self.failed = 0
        self.passed = 0

    def check(self, ident, what, ok, detail=""):
        if ok:
            self.passed += 1
            print("  ✓ %-5s %s" % (ident, what))
        else:
            self.failed += 1
            print("  ✗ %-5s %s%s" % (ident, what,
                                       ("  [%s]" % detail) if detail else ""))
        return ok


SPAN = (0, 0, 2 * HEAD_W, HEAD_H)
HEAD1 = (0, 0, HEAD_W, HEAD_H)
HEAD2 = (HEAD_W, 0, HEAD_W, HEAD_H)


def run_checks(wm):
    r = Results()

    heads = wm.heads()
    if heads != [HEAD1, HEAD2]:
        print("unexpected Xinerama layout %s, want %s" % (heads, [HEAD1, HEAD2]))
        r.failed += 1
        return r

    r.check("FM1", "_NET_SUPPORTED lists _NET_WM_FULLSCREEN_MONITORS",
            wm.atom("_NET_WM_FULLSCREEN_MONITORS") in wm.supported())

    # A: the window we make fullscreen
    a = wm.make_window(100, 100, 300, 200, "fsmon-A")
    wm.set_fullscreen(a, True)
    r.check("base", "plain fullscreen fills head 1",
            wait_for(wm, lambda: wm.geometry(a) == HEAD1), wm.geometry(a))
    r.check("base", "no property while no monitors were requested",
            wm.monitors(a) is None, wm.monitors(a))

    wm.set_monitors(a, 0, 0, 0, 1)
    r.check("FM1", "property reflects the request [0, 0, 0, 1]",
            wait_for(wm, lambda: wm.monitors(a) == [0, 0, 0, 1]),
            wm.monitors(a))
    r.check("FM2", "fullscreen spans both heads",
            wait_for(wm, lambda: wm.geometry(a) == SPAN), wm.geometry(a))

    wm.set_monitors(a, 0, 0, 0, 7)
    settle(wm)
    r.check("FM5", "out-of-range index is ignored",
            wm.monitors(a) == [0, 0, 0, 1] and wm.geometry(a) == SPAN,
            "%s %s" % (wm.monitors(a), wm.geometry(a)))

    # focus stealing: C on head 2, which A covers
    wm.activate(a, SOURCE_PAGER)
    wait_for(wm, lambda: wm.active() == a.id)
    c = wm.make_window(HEAD_W + 200, 100, 300, 200, "fsmon-C")
    c.configure(x=HEAD_W + 200, y=100)
    settle(wm)
    wm.activate(c, SOURCE_APP)
    settle(wm)
    r.check("FM4", "app on a covered head can't steal focus from A",
            wm.active() == a.id, "active=%s A=%s C=%s" % (
                wm.active(), hex(a.id), hex(c.id)))

    # layering: focus C explicitly, it has to show up above A
    wm.activate(c, SOURCE_PAGER)
    wait_for(wm, lambda: wm.active() == c.id)
    settle(wm)
    st = wm.stacking()
    fa, fc = wm.frame_of(a).id, wm.frame_of(c).id
    r.check("FM3", "focused window on a covered head is stacked above A",
            fa in st and fc in st and st.index(fc) > st.index(fa),
            "frames A=%s C=%s, stacking=%s" % (hex(fa), hex(fc),
                                               [hex(w) for w in st]))
    c.destroy()
    wm.activate(a, SOURCE_PAGER)
    wait_for(wm, lambda: wm.active() == a.id)

    # transient of the spanning window
    d = wm.make_window(0, 0, 200, 100, "fsmon-D", transient_for=a, dialog=True)
    r.check("FM10", "dialog of A is placed on a single head",
            on_one_head(wm.geometry(d)), wm.geometry(d))
    d.destroy()
    settle(wm)

    # the list survives leaving fullscreen
    wm.set_fullscreen(a, False)
    wait_for(wm, lambda: wm.geometry(a)[2] < 2 * HEAD_W)
    r.check("FM7", "property kept while not fullscreen",
            wm.monitors(a) == [0, 0, 0, 1], wm.monitors(a))
    wm.set_fullscreen(a, True)
    r.check("FM7", "fullscreen again spans both heads",
            wait_for(wm, lambda: wm.geometry(a) == SPAN), wm.geometry(a))

    # an explicit move to another head drops the list
    wm.activate(a, SOURCE_PAGER)
    wait_for(wm, lambda: wm.active() == a.id)
    wm.remote("SetHead 2")
    r.check("FM8", "SetHead 2 drops the property",
            wait_for(wm, lambda: wm.monitors(a) is None), wm.monitors(a))
    r.check("FM8", "SetHead 2 leaves A fullscreen on head 2",
            wait_for(wm, lambda: wm.geometry(a) == HEAD2), wm.geometry(a))
    a.destroy()
    settle(wm)

    # property already present when the window is managed (fluxbox restart)
    e = wm.make_window(100, 100, 300, 200, "fsmon-E", props=[
        ("_NET_WM_FULLSCREEN_MONITORS", Xatom.CARDINAL, [0, 0, 0, 1]),
        ("_NET_WM_STATE", Xatom.ATOM, [wm.atom("_NET_WM_STATE_FULLSCREEN")]),
    ])
    r.check("FM1", "existing property is honored when a window is managed",
            wait_for(wm, lambda: wm.geometry(e) == SPAN), wm.geometry(e))
    e.destroy()
    settle(wm)

    print("  ○ FM6   RandR (monitor unplugged): not covered, Xephyr "
          "can't change its Xinerama layout")
    return r


# --------------------------------------------------------------------------
# commands

def cmd_test(args):
    sb = Sandbox(args.fluxbox)
    wm = sb.start()
    print("fluxbox %s on %s (HOME=%s)" % (sb.fluxbox, sb.display, sb.home))
    try:
        r = run_checks(wm)
    except Exception:
        sb.stop(keep_home=True)
        print("log: %s" % sb.log)
        raise
    print("%d passed, %d failed" % (r.passed, r.failed))
    if r.failed:
        print("--- fluxbox log (%s) ---\n%s" % (sb.log, sb.tail_log()))
    if args.keep:
        print("keeping DISPLAY=%s running, Ctrl-C to stop" % sb.display)
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            pass
    sb.stop(keep_home=bool(r.failed))
    return 1 if r.failed else 0


def cmd_xephyr(args):
    sb = Sandbox(args.fluxbox)
    sb.start()
    env = dict(os.environ, DISPLAY=sb.display)
    if shutil.which("xterm"):
        subprocess.Popen(["xterm"], env=env)
    me = os.path.abspath(__file__)
    print("DISPLAY=%s  (fluxbox log: %s)\n" % (sb.display, sb.log))
    print("e.g.  export DISPLAY=%s" % sb.display)
    print("      W=$(xdotool getactivewindow 2>/dev/null || "
          "xwininfo | awk '/Window id/{print $4}')")
    print("      %s fullscreen $W on" % me)
    print("      %s send $W 0 0 0 1" % me)
    print("      %s show $W\n" % me)
    print("Ctrl-C to stop")
    try:
        while all(p.poll() is None for p in sb.procs):
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    sb.stop()
    return 0


def open_wm():
    try:
        return Wm(xdisplay.Display())
    except DisplayError as e:
        sys.exit("can't open display: %s" % e)


def cmd_fullscreen(args):
    wm = open_wm()
    wm.set_fullscreen(wm.window(args.window), args.state == "on")
    return 0


def cmd_send(args):
    wm = open_wm()
    wm.set_monitors(wm.window(args.window), args.top, args.bottom,
                    args.left, args.right)
    return 0


def cmd_show(args):
    wm = open_wm()
    win = wm.window(args.window)
    wm.d.sync()
    print("_NET_WM_FULLSCREEN_MONITORS: %s" % (wm.monitors(win) or "(not set)"))
    print("geometry (x, y, w, h):       %s" % (wm.geometry(win),))
    print("heads:                       %s" % (wm.heads(),))
    return 0


def main():
    p = argparse.ArgumentParser(
        description=__doc__.split("\n")[0],
        epilog=__doc__.split("\n", 2)[2],
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("test", help="run the automated checks in Xephyr")
    s.add_argument("--fluxbox", default=default_fluxbox())
    s.add_argument("--keep", action="store_true",
                   help="keep Xephyr and fluxbox running afterwards")
    s.set_defaults(func=cmd_test)

    s = sub.add_parser("xephyr", help="start the sandbox for manual testing")
    s.add_argument("--fluxbox", default=default_fluxbox())
    s.set_defaults(func=cmd_xephyr)

    wid = lambda v: int(v, 0)
    s = sub.add_parser("fullscreen", help="toggle _NET_WM_STATE_FULLSCREEN")
    s.add_argument("window", type=wid)
    s.add_argument("state", choices=("on", "off"))
    s.set_defaults(func=cmd_fullscreen)

    s = sub.add_parser("send", help="send _NET_WM_FULLSCREEN_MONITORS")
    s.add_argument("window", type=wid)
    for edge in ("top", "bottom", "left", "right"):
        s.add_argument(edge, type=int)
    s.set_defaults(func=cmd_send)

    s = sub.add_parser("show", help="show property and geometry")
    s.add_argument("window", type=wid)
    s.set_defaults(func=cmd_show)

    args = p.parse_args()
    sys.exit(args.func(args))


if __name__ == "__main__":
    main()
