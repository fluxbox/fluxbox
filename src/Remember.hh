// Remember.hh for Fluxbox Window Manager
// Copyright (c) 2002 Xavier Brouckaert
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Remember.hh,v 1.2 2003/04/26 11:24:55 rathnor Exp $

/* Based on the original "Remember patch" by Xavier Brouckaert */

#ifndef REMEMBER_HH
#define REMEMBER_HH

#include "Window.hh"
#include "AtomHandler.hh"
#include "Screen.hh"
#include "StringUtil.hh"

#include <fstream>
#include <map>
#include <string>

class Application {
public:
    Application();
    inline void forgetWorkspace() { workspace_remember = false; }
    inline void forgetDimensions() { dimensions_remember = false; }
    inline void forgetPosition() { position_remember = false; }
    inline void forgetShadedstate() { shadedstate_remember = false; }
    inline void forgetTabstate() { tabstate_remember = false; }
    inline void forgetDecostate() { decostate_remember = false; }
    inline void forgetStuckstate() { stuckstate_remember = false; }
    inline void forgetJumpworkspace() { jumpworkspace_remember = false; }
    inline void forgetSaveOnClose() { save_on_close_remember = false; }
    
    inline void rememberWorkspace(int ws) 
        { workspace = ws; workspace_remember = true; }
    inline void rememberDimensions(int width, int height) 
        { w = width; h = height; dimensions_remember = true; }
    inline void rememberPosition(int posx, int posy)
        { x = posx; y = posy; position_remember = true; }
    inline void rememberShadedstate(bool state)
        { shadedstate = state; shadedstate_remember = true; }
    inline void rememberTabstate(bool state)
        { tabstate = state; tabstate_remember = true; }
    inline void rememberDecostate(unsigned int state)
        { decostate = state; decostate_remember = true; }
    inline void rememberStuckstate(bool state)
        { stuckstate = state; stuckstate_remember = true; }
    inline void rememberJumpworkspace(bool state)
        { jumpworkspace = state; jumpworkspace_remember = true; }
    inline void rememberSaveOnClose(bool state)
        { save_on_close = state; save_on_close_remember = true; }


    bool workspace_remember;
    unsigned int workspace;

    bool dimensions_remember;
    int w,h; // width, height

    bool position_remember;
    int x,y;

    bool shadedstate_remember;
    bool shadedstate;

    bool tabstate_remember;
    bool tabstate;

    bool decostate_remember;
    unsigned int decostate;

    bool stuckstate_remember;
    bool stuckstate;

    bool jumpworkspace_remember;
    bool jumpworkspace;

    bool save_on_close_remember;
    bool save_on_close;
};


/*
 * Class Remember is an atomhandler to avoid interfering with
 * the main code as much as possible, since we hope that one day
 * things like this (and maybe toolbar/slit) can become some sort
 * of modular plugin. Doing this should help give an idea of what
 * sort of interface abilities we'll need...
 */
class Remember : public AtomHandler {
public:
    enum Attribute {
        REM_WORKSPACE=0,
        REM_DIMENSIONS,
        REM_POSITION,
        REM_STUCKSTATE,
        REM_DECOSTATE,
        REM_SHADEDSTATE,
        //REM_TABSTATE, ... external tabs disabled atm
        REM_JUMPWORKSPACE,
        REM_SAVEONCLOSE,
        REM_LASTATTRIB // not actually used
    };

    typedef std::map<std::string,Application *> Apps;
    Remember();

    Application* find(WinClient &winclient);
    Application* find(const char* app_name);
    Application* add(WinClient &winclient);
    Application* add(const char* app_name);

    void load();
    void save();

    bool isRemembered(WinClient &win, Attribute attrib);
    void rememberAttrib(WinClient &win, Attribute attrib);
    void forgetAttrib(WinClient &win, Attribute attrib);

    // Functions relating to AtomHandler
    
    // Functions we actually use
    void setupWindow(FluxboxWindow &win);
    void updateWindowClose(FluxboxWindow &win);

    // Functions we ignore (zero from AtomHandler)
    // Leaving here in case they might be useful later

    void initForScreen(BScreen &screen) {}

    void updateClientList(BScreen &screen) {}
    void updateWorkspaceNames(BScreen &screen) {}
    void updateCurrentWorkspace(BScreen &screen) {}
    void updateWorkspaceCount(BScreen &screen) {}

    void updateWorkspace(FluxboxWindow &win) {}
    void updateState(FluxboxWindow &win) {}
    void updateHints(FluxboxWindow &win) {}
    void updateLayer(FluxboxWindow &win) {}

    bool checkClientMessage(const XClientMessageEvent &ce, 
        BScreen * screen, FluxboxWindow * const win) { return false; }


private:

    // returns number of lines read
    int parseApp(std::ifstream &file, Application *a);
    Apps apps;

};

#endif // REMEMBER_HH
