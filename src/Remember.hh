// Remember.hh for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                     and Simon Bowden    (rathnor at users.sourceforge.net)
// Copyright (c) 2002 Xavier Brouckaert
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

/* Based on the original "Remember patch" by Xavier Brouckaert */

#ifndef REMEMBER_HH
#define REMEMBER_HH

#include "AtomHandler.hh"
#include "ClientPattern.hh"


#include <map>
#include <list>
#include <memory>

class FluxboxWindow;
class BScreen;
class WinClient;
class Application;

namespace FbTk {
class AutoReloadHelper;
class Menu;
}

/**
 * Class Remember is an atomhandler to avoid interfering with
 * the main code as much as possible, since we hope that one day
 * things like this (and maybe toolbar/slit) can become some sort
 * of modular plugin. Doing this should help give an idea of what
 * sort of interface abilities we'll need...
 */
class Remember : public AtomHandler {
public:
    /**
     * holds which attributes to remember
     */
    enum Attribute {
        REM_DECOSTATE= 0,
        REM_DIMENSIONS,
        REM_FOCUSHIDDENSTATE,
        REM_ICONHIDDENSTATE,
        REM_JUMPWORKSPACE,
        REM_LAYER,
        REM_POSITION,
        REM_SAVEONCLOSE,
        REM_SHADEDSTATE,
        REM_STUCKSTATE,
        //REM_TABSTATE, ... external tabs disabled atm
        REM_WORKSPACE,
        REM_HEAD,
        REM_ALPHA,
        REM_MINIMIZEDSTATE,
        REM_MAXIMIZEDSTATE,
        REM_FULLSCREENSTATE,
        REM_FOCUSPROTECTION,
        REM_IGNORE_SIZEHINTS,
        REM_LASTATTRIB // not actually used
    };

    enum {
      POS_UPPERLEFT= 0,
      POS_UPPERRIGHT,
      POS_LOWERLEFT,
      POS_LOWERRIGHT,
      POS_CENTER
    };


    // a "pattern"  to the relevant app
    // each app exists ONLY for that pattern.
    // And we need to keep a list of pairs as we want to keep the
    // applications in the same order as they will be in the apps file
    typedef std::list< std::pair<ClientPattern *, Application *> > Patterns;

    // We keep track of which app is assigned to a winclient
    // particularly useful to update counters etc on windowclose
    typedef std::map<WinClient *, Application *> Clients;

    // we have to remember any startups we did so that they are saved again
    typedef std::list<std::string> Startups;

    Remember();
    ~Remember();

    Application* find(WinClient &winclient);
    Application* add(WinClient &winclient);
    FluxboxWindow* findGroup(Application *, BScreen &screen);

    void reconfigure();
    void checkReload();
    void reload();
    void save();

    bool isRemembered(WinClient &win, Attribute attrib);
    void rememberAttrib(WinClient &win, Attribute attrib);
    void forgetAttrib(WinClient &win, Attribute attrib);

    // Functions relating to AtomHandler

    // Functions we actually use
    void setupFrame(FluxboxWindow &win);
    void setupClient(WinClient &winclient);
    void updateClientClose(WinClient &winclient);

    void initForScreen(BScreen &screen);

    static FbTk::Menu* createMenu(BScreen& screen);

    // Functions we ignore (zero from AtomHandler)
    // Leaving here in case they might be useful later

    void updateFocusedWindow(BScreen &, Window) { }
    void updateClientList(BScreen &screen) {}
    void updateWorkspaceNames(BScreen &screen) {}
    void updateCurrentWorkspace(BScreen &screen) {}
    void updateWorkspaceCount(BScreen &screen) {}
    void updateWorkarea(BScreen &) { }

    void updateWorkspace(FluxboxWindow &win) {}
    void updateState(FluxboxWindow &win) {}
    void updateHints(FluxboxWindow &win) {}
    void updateLayer(FluxboxWindow &win) {}
    void updateFrameClose(FluxboxWindow &win) {}

    void updateDecoStateFromClient(WinClient& client);

    bool checkClientMessage(const XClientMessageEvent &ce, 
        BScreen * screen, WinClient * const winclient) { return false; }
    // ignore this
    bool propertyNotify(WinClient &winclient, Atom the_property) { return false; }

    static Remember &instance() { return *s_instance; }

private:

    std::unique_ptr<Patterns> m_pats;
    Clients m_clients;

    Startups m_startups;
    static Remember *s_instance;

    FbTk::AutoReloadHelper* m_reloader;
};

#endif // REMEMBER_HH
