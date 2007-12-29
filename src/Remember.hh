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

// $Id$

/* Based on the original "Remember patch" by Xavier Brouckaert */

#ifndef REMEMBER_HH
#define REMEMBER_HH

#include "AtomHandler.hh"
#include "ClientPattern.hh"

#include "FbTk/RefCount.hh"

#include <fstream>
#include <map>
#include <list>
#include <string>
#include <utility>
#include <memory>

class FluxboxWindow;
class BScreen;
class WinClient;

class Application {
public:
    Application(bool grouped, ClientPattern *pat = 0);
    void reset();
    void forgetWorkspace() { workspace_remember = false; }
    void forgetHead() { head_remember = false; }
    void forgetDimensions() { dimensions_remember = false; }
    void forgetPosition() { position_remember = false; }
    void forgetShadedstate() { shadedstate_remember = false; }
    void forgetTabstate() { tabstate_remember = false; }
    void forgetDecostate() { decostate_remember = false; }
    void forgetFocusHiddenstate() { focushiddenstate_remember= false; }
    void forgetIconHiddenstate() { iconhiddenstate_remember= false; }
    void forgetStuckstate() { stuckstate_remember = false; }
    void forgetJumpworkspace() { jumpworkspace_remember = false; }
    void forgetLayer() { layer_remember = false; }
    void forgetSaveOnClose() { save_on_close_remember = false; }
    void forgetAlpha() { alpha_remember = false; }
    void forgetMinimizedstate() { minimizedstate_remember = false; }
    void forgetMaximizedstate() { maximizedstate_remember = false; }
    void forgetFullscreenstate() { fullscreenstate_remember = false; }

    void rememberWorkspace(int ws)
        { workspace = ws; workspace_remember = true; }
    void rememberHead(int h)
        { head = h; head_remember = true; }
    void rememberDimensions(int width, int height)
        { w = width; h = height; dimensions_remember = true; }
    void rememberFocusHiddenstate(bool state)
        { focushiddenstate= state; focushiddenstate_remember= true; }
    void rememberIconHiddenstate(bool state)
        { iconhiddenstate= state; iconhiddenstate_remember= true; }
    void rememberPosition(int posx, int posy, unsigned char rfc= 0 )
        { x = posx; y = posy; refc = rfc; position_remember = true; }
    void rememberShadedstate(bool state)
        { shadedstate = state; shadedstate_remember = true; }
    void rememberTabstate(bool state)
        { tabstate = state; tabstate_remember = true; }
    void rememberDecostate(unsigned int state)
        { decostate = state; decostate_remember = true; }
    void rememberStuckstate(bool state)
        { stuckstate = state; stuckstate_remember = true; }
    void rememberJumpworkspace(bool state)
        { jumpworkspace = state; jumpworkspace_remember = true; }
    void rememberLayer(int layernum) 
        { layer = layernum; layer_remember = true; }
    void rememberSaveOnClose(bool state)
        { save_on_close = state; save_on_close_remember = true; }
    void rememberAlpha(int focused_a, int unfocused_a)
        { focused_alpha = focused_a; unfocused_alpha = unfocused_a; alpha_remember = true; }
    void rememberMinimizedstate(bool state)
        { minimizedstate = state; minimizedstate_remember = true; }
    void rememberMaximizedstate(int state)
        { maximizedstate = state; maximizedstate_remember = true; }
    void rememberFullscreenstate(bool state)
        { fullscreenstate = state; fullscreenstate_remember = true; }

    bool workspace_remember;
    unsigned int workspace;

    bool head_remember;
    int head;

    bool dimensions_remember;
    int w,h; // width, height

    bool position_remember;
    int x,y;
    unsigned char refc;    // referenceCorner-> 0 - upperleft
                           //                   1 - upperight
                           //                   2 - lowerleft
                           //                   3 - lowerright

    bool alpha_remember;
    int focused_alpha;
    int unfocused_alpha;

    bool shadedstate_remember;
    bool shadedstate;

    bool tabstate_remember;
    bool tabstate;

    bool decostate_remember;
    unsigned int decostate;

    bool stuckstate_remember;
    bool stuckstate;

    bool focushiddenstate_remember;
    bool focushiddenstate;

    bool iconhiddenstate_remember;
    bool iconhiddenstate;

    bool jumpworkspace_remember;
    bool jumpworkspace;

    bool layer_remember;
    int layer;

    bool save_on_close_remember;
    bool save_on_close;

    bool minimizedstate_remember;
    bool minimizedstate;

    bool maximizedstate_remember;
    int maximizedstate;

    bool fullscreenstate_remember;
    bool fullscreenstate;

    bool is_grouped;
    FbTk::RefCount<ClientPattern> group_pattern;

};

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
        REM_LASTATTRIB // not actually used
    };

    enum {
      POS_UPPERLEFT= 0,
      POS_UPPERRIGHT,
      POS_LOWERLEFT,
      POS_LOWERRIGHT,
      POS_CENTER,
      POS_WINCENTER
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

    void reconfigure(); // was load
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

    bool checkClientMessage(const XClientMessageEvent &ce, 
        BScreen * screen, WinClient * const winclient) { return false; }
    // ignore this
    bool propertyNotify(WinClient &winclient, Atom the_property) { return false; }

    static Remember &instance() { return *s_instance; }

private:

    // returns number of lines read
    // optionally can give a line to read before the first (lookahead line)
    int parseApp(std::ifstream &file, Application &app, std::string *first_line = 0);

    Application *findMatchingPatterns(ClientPattern *pat, Patterns *patlist,
            bool is_group, ClientPattern *match_pat = 0);

    std::auto_ptr<Patterns> m_pats;
    Clients m_clients;

    Startups m_startups;
    static Remember *s_instance;

    time_t m_last_timestamp;
};

#endif // REMEMBER_HH
