// Keys.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

//$Id: Keys.cc,v 1.37 2003/09/06 13:58:06 fluxgen Exp $


#include "Keys.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/App.hh"
#include "FbTk/Command.hh"
#include "FbTk/KeyUtil.hh"

#include "CommandParser.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif	// HAVE_CTYPE_H

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif	// HAVE_SYS_TYPES_H

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif	// HAVE_SYS_WAIT_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif	// HAVE_UNISTD_H

#ifdef	HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif	// HAVE_SYS_STAT_H

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <memory>

using namespace std;

Keys::Keys(const char *filename):
    m_display(FbTk::App::instance()->display()),
    m_modmap(0) {

    loadModmap();

    if (filename != 0)
        load(filename);
}

Keys::~Keys() {	
    if (m_modmap)
        XFreeModifiermap(m_modmap);

    ungrabKeys();
    deleteTree();
}

/// Destroys the keytree
void Keys::deleteTree() {
    while (!m_keylist.empty()) {
        if (m_keylist.back())
            delete m_keylist.back();		
        m_keylist.pop_back();
    }
}

/// Ungrabs the keys
void Keys::ungrabKeys() {
    for (int screen=0; screen<ScreenCount(m_display); screen++) {
        XUngrabKey(m_display, AnyKey, AnyModifier,
                   RootWindow(m_display, screen));		
    }
}

/** 
    Load and grab keys
    TODO: error checking
    @return true on success else false
*/
bool Keys::load(const char *filename) {
    if (!filename)
        return false;
	
    //ungrab all keys
    ungrabKeys();

    //free memory of previous grabs
    deleteTree();

    XSync(m_display, False);
						
    //open the file
    ifstream infile(filename);
    if (!infile)
        return false; // faild to open file

    int line=0;//current line, so we can tell the user where the fault is

    while (!infile.eof()) {
        string linebuffer;

        getline(infile, linebuffer);

        line++;
        vector<string> val;
        //Parse arguments
        FbTk::StringUtil::stringtok(val, linebuffer.c_str());

        //must have at least 1 argument
        if (val.size() <= 0)
            continue;
			
        if (val[0][0] == '#') //the line is commented
            continue;
		
        unsigned int key=0, mod=0;		
        char keyarg=0;
        t_key *current_key=0, *last_key=0;
		
        for (unsigned int argc=0; argc<val.size(); argc++) {

            if (val[argc][0] != ':') { // parse key(s)
                keyarg++;
                if (keyarg==1) //first arg is modifier
                    mod = getModifier(val[argc].c_str());
                else if (keyarg>1) {

                    //keyarg=0;
                    int tmpmod = getModifier(val[argc].c_str());
                    if(tmpmod)
                        mod|=tmpmod; //If it's a modifier
                    else { 
                        key = getKey(val[argc].c_str()); // else get the key
                        if (key == 0) {
                            cerr<<"["<<filename<<"]: Invalid key/modifier on line("<<
                                line<<"): "<<linebuffer<<endl;
                            break; // get next line
                        }
                        if (!current_key) {
                            current_key = new t_key(key, mod);
                            last_key = current_key;
                        } else { 
                            t_key *temp_key = new t_key(key, mod);
                            last_key->keylist.push_back(temp_key);
                            last_key = temp_key;
                        }
                    }
                }			

            } else { // parse command line
                if (last_key == 0) {
                    cerr<<"File: "<<filename<<". Error on line: "<<line<<endl;
                    cerr<<"> "<<linebuffer<<endl;
                    break;
                }

                const char *str = 
                    FbTk::StringUtil::strcasestr(linebuffer.c_str(),
                                                 val[argc].c_str() + 1); // +1 to skip ':'
                if (str == 0) {
                    cerr<<"File: "<<filename<<". Error on line: "<<line<<endl;
                    cerr<<"> "<<linebuffer<<endl;
                } else {

                    last_key->m_command = CommandParser::instance().parseLine(str);

                    if (*last_key->m_command == 0) {
                        cerr<<"File: "<<filename<<". Error on line: "<<line<<endl;
                        cerr<<"> "<<linebuffer<<endl;
                    } else {
                        // Add the keychain to list
                        if (!mergeTree(current_key))
                            cerr<<"Keys: Failed to merge keytree!"<<endl;
                    }
                }
                delete current_key;
                current_key = 0;
                last_key = 0;

                break; // dont process this linebuffer more
            }  // end if
        } // end for
    } // end while eof

    return true;
}

void Keys::loadModmap() {
    if (m_modmap)
        XFreeModifiermap(m_modmap);

    m_modmap = XGetModifierMapping(m_display);
    // force reinit of modifiers
    FbTk::KeyUtil::init();
}

/**
 Grabs a key with the modifier
 and with numlock,capslock and scrollock
*/
void Keys::grabKey(unsigned int key, unsigned int mod) {
    const int capsmod = FbTk::KeyUtil::capslockMod();
    const int nummod = FbTk::KeyUtil::numlockMod();
    const int scrollmod = FbTk::KeyUtil::scrolllockMod();
    
    for (int screen=0; screen<ScreenCount(m_display); screen++) {
		
        Window root = RootWindow(m_display, screen);
		
        XGrabKey(m_display, key, mod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);
						
        // Grab with numlock, capslock and scrlock	

        //numlock	
        XGrabKey(m_display, key, mod|nummod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);		
        //scrolllock
        XGrabKey(m_display, key, mod|scrollmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);	
        //capslock
        XGrabKey(m_display, key, mod|capsmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);
	
        //capslock+numlock
        XGrabKey(m_display, key, mod|capsmod|nummod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);

        //capslock+scrolllock
        XGrabKey(m_display, key, mod|capsmod|scrollmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);						
	
        //capslock+numlock+scrolllock
        XGrabKey(m_display, key, mod|capsmod|scrollmod|nummod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);						

        //numlock+scrollLock
        XGrabKey(m_display, key, mod|nummod|scrollmod,
                 root, True,
                 GrabModeAsync, GrabModeAsync);
	
    }
			
}

/**
 @return the modifier for the modstr else zero on failure.
 TODO fix more masks
*/
unsigned int Keys::getModifier(const char *modstr) {
    if (!modstr)
        return 0;
    struct t_modlist{
        char *str;
        unsigned int mask;
        bool operator == (const char *modstr) {
            return  (strcasecmp(str, modstr) == 0 && mask !=0);
        }
    } modlist[] = {
        {"SHIFT", ShiftMask},
        {"CONTROL", ControlMask},
        {"MOD1", Mod1Mask},
        {"MOD2", Mod2Mask},
        {"MOD3", Mod3Mask},
        {"MOD4", Mod4Mask},
        {"MOD5", Mod5Mask},
        {0, 0}
    };

    // find mod mask string
    for (unsigned int i=0; modlist[i].str !=0; i++) {
        if (modlist[i] == modstr)		
            return modlist[i].mask;		
    }
	
    return 0;	
}

/**
 @return keycode of keystr on success else 0
*/
unsigned int Keys::getKey(const char *keystr) {
    if (!keystr)
        return 0;
    return XKeysymToKeycode(m_display,
                            XStringToKeysym(keystr));
}

/**
 @return the KeyAction of the XKeyEvent
*/
void Keys::doAction(XKeyEvent &ke) {
    static t_key *next_key = 0;
    // Remove numlock, capslock and scrolllock
    ke.state = FbTk::KeyUtil::cleanMods(ke.state);
	
    if (!next_key) {
	
        for (unsigned int i=0; i<m_keylist.size(); i++) {
            if (*m_keylist[i] == ke) {
                if (m_keylist[i]->keylist.size()) {
                    next_key = m_keylist[i];
                    break; //end for-loop 
                } else {
                    if (*m_keylist[i]->m_command != 0)
                        m_keylist[i]->m_command->execute();
                }
            }
        }
	
    } else { //check the nextkey
        t_key *temp_key = next_key->find(ke);
        if (temp_key) {
            if (temp_key->keylist.size()) {
                next_key = temp_key;								
            } else {
                next_key = 0;
                if (*temp_key->m_command != 0)
                    temp_key->m_command->execute();
            }
        }  else  {
            temp_key = next_key;		
            next_key = 0;
            if (*temp_key->m_command != 0)
                temp_key->m_command->execute();
                
        }		
    }
}

/**
 deletes the tree and load configuration
 returns true on success else false
*/
bool Keys::reconfigure(const char *filename) {
    deleteTree();
    return load(filename);
}

/**
 Merges two chains and binds new keys
 @return true on success else false.
*/
bool Keys::mergeTree(t_key *newtree, t_key *basetree) {
    if (basetree==0) {
        unsigned int baselist_i=0;
        for (; baselist_i<m_keylist.size(); baselist_i++) {
            if (m_keylist[baselist_i]->mod == newtree->mod && 
                m_keylist[baselist_i]->key == newtree->key) {
                if (newtree->keylist.size() && *m_keylist[baselist_i]->m_command == 0) {
                    //assumes the newtree only have one branch
                    return mergeTree(newtree->keylist[0], m_keylist[baselist_i]);
                } else
                    break;
            }
        }

        if (baselist_i == m_keylist.size()) {
            grabKey(newtree->key, newtree->mod);
            m_keylist.push_back(new t_key(newtree));			
            if (newtree->keylist.size())
                return mergeTree(newtree->keylist[0], m_keylist.back());
            return true;
        }
		
    } else {
        unsigned int baselist_i = 0;
        for (; baselist_i<basetree->keylist.size(); baselist_i++) {
            if (basetree->keylist[baselist_i]->mod == newtree->mod &&
                basetree->keylist[baselist_i]->key == newtree->key) {
                if (newtree->keylist.size()) {
                    //assumes the newtree only have on branch
                    return mergeTree(newtree->keylist[0], basetree->keylist[baselist_i]);
                } else
                    return false;
            }					
        }
        //if it wasn't in the list grab the key and add it to the list
        if (baselist_i==basetree->keylist.size()) {			
            grabKey(newtree->key, newtree->mod);
            basetree->keylist.push_back(new t_key(newtree));
            if (newtree->keylist.size())
                return mergeTree(newtree->keylist[0], basetree->keylist.back());
            return true;		
        }		
    }
	
    return false;
}

Keys::t_key::t_key(unsigned int key_, unsigned int mod_, FbTk::RefCount<FbTk::Command> command) {
    key = key_;
    mod = mod_;	
    m_command = command;
}

Keys::t_key::t_key(t_key *k) {
    key = k->key;
    mod = k->mod;
    m_command = k->m_command;
}

Keys::t_key::~t_key() {	
    while (!keylist.empty()) {		
        t_key *k = keylist.back();
        if (k != 0) { // make sure we don't have a bad key pointer
            delete k;
            keylist.pop_back();
        }
    }

}

unsigned int Keys::keycodeToModmask(unsigned int keycode) {
    if (!m_modmap) return 0;

    // search through modmap for this keycode
    for (int mod=0; mod < 8; mod++) {
        for (int key=0; key < m_modmap->max_keypermod; ++key) {
            // modifiermap is an array with 8 sets of keycodes
            // each max_keypermod long, but in a linear array.
            if (m_modmap->modifiermap[m_modmap->max_keypermod*mod + key] == keycode) {
                return (1<<mod);
            }
        } 
    }
    // no luck
    return 0;
}
