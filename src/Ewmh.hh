// Ewmh.hh for fluxbox
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

#include "AtomHandler.hh"
#include "FbTk/FbString.hh"

/// Implementes Extended Window Manager Hints ( http://www.freedesktop.org/Standards/wm-spec )
class Ewmh:public AtomHandler {
public:

    Ewmh();
    ~Ewmh();

    void initForScreen(BScreen &screen);
    void setupFrame(FluxboxWindow &win);
    void setupClient(WinClient &winclient);

    void updateFocusedWindow(BScreen &screen, Window win);
    void updateClientList(BScreen &screen);
    void updateWorkspaceNames(BScreen &screen);
    void updateCurrentWorkspace(BScreen &screen);
    void updateWorkspaceCount(BScreen &screen);
    void updateViewPort(BScreen &screen);
    void updateGeometry(BScreen &screen);
    void updateWorkarea(BScreen &screen);

    void updateState(FluxboxWindow &win);
    void updateLayer(FluxboxWindow &win);
    void updateHints(FluxboxWindow &win);
    void updateWorkspace(FluxboxWindow &win);

    bool checkClientMessage(const XClientMessageEvent &ce,
                            BScreen * screen, WinClient * const winclient);

    bool propertyNotify(WinClient &winclient, Atom the_property);
    void updateFrameClose(FluxboxWindow &win);

    void updateClientClose(WinClient &winclient);

    void updateFrameExtents(FluxboxWindow &win);
private:

    enum { STATE_REMOVE = 0, STATE_ADD = 1, STATE_TOGGLE = 2};

    void setState(FluxboxWindow &win, Atom state, bool value);
    void setState(FluxboxWindow &win, Atom state, bool value,
                  WinClient &client);
    void toggleState(FluxboxWindow &win, Atom state);
    void toggleState(FluxboxWindow &win, Atom state, WinClient &client);
    void updateStrut(WinClient &winclient);
    void updateActions(FluxboxWindow &win);

    void setupState(FluxboxWindow &win);

    FbTk::FbString getUTF8Property(Atom property);

    class EwmhAtoms;
    EwmhAtoms* m_net;
};
