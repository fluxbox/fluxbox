// Keys.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

//$Id: Keys.cc,v 1.15 2002/04/19 09:33:42 fluxgen Exp $

#ifdef		HAVE_CONFIG_H
#	 include "config.h"
#endif

#include "Keys.hh"

#include "StringUtil.hh"

#ifdef		HAVE_STDIO_H
#	 include <stdio.h>
#endif	// HAVE_STDIO_H

#ifdef		HAVE_CTYPE_H
#	 include <ctype.h>
#endif	// HAVE_CTYPE_H

#ifdef		STDC_HEADERS
#	 include <stdlib.h>
#	 include <string.h>
#  include <errno.h>
#endif	// STDC_HEADERS

#if HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef		HAVE_SYS_TYPES_H
#	 include <sys/types.h>
#endif	// HAVE_SYS_TYPES_H

#ifdef		HAVE_SYS_WAIT_H
#	 include <sys/wait.h>
#endif	// HAVE_SYS_WAIT_H

#ifdef		HAVE_UNISTD_H
#	 include <unistd.h>
#endif	// HAVE_UNISTD_H

#ifdef		HAVE_SYS_STAT_H
#	 include <sys/stat.h>
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

Keys::t_actionstr Keys::m_actionlist[] = {
	{"Minimize", ICONIFY},
	{"Raise", RAISE},
	{"Lower", LOWER},
	{"Close", CLOSE},
	{"AbortKeychain", ABORTKEYCHAIN},
	{"Workspace", WORKSPACE},
	{"Workspace1", WORKSPACE1},
	{"Workspace2", WORKSPACE2},
	{"Workspace3", WORKSPACE3},
	{"Workspace4", WORKSPACE4},
	{"Workspace5", WORKSPACE5},
	{"Workspace6", WORKSPACE6},
	{"Workspace7", WORKSPACE7},
	{"Workspace8", WORKSPACE8},
	{"Workspace9", WORKSPACE9},
	{"Workspace10", WORKSPACE10},
	{"Workspace11", WORKSPACE11},
	{"Workspace12",  WORKSPACE12},
	{"SendToWorkspace", SENDTOWORKSPACE},
	{"NextWorkspace", NEXTWORKSPACE},
	{"PrevWorkspace", PREVWORKSPACE},
	{"LeftWorkspace", LEFTWORKSPACE},
	{"RightWorkspace", RIGHTWORKSPACE},
	{"KillWindow", KILLWINDOW},
	{"NextWindow", NEXTWINDOW},
	{"PrevWindow", PREVWINDOW},
	{"NextTab", NEXTTAB},
	{"PrevTab", PREVTAB},
	{"ShadeWindow", SHADE},
	{"MaximizeWindow", MAXIMIZE},
	{"StickWindow", STICK},
	{"ExecCommand", EXECUTE},
	{"MaximizeVertical", VERTMAX},
	{"MaximizeHorizontal", HORIZMAX},
	{"NudgeRight", NUDGERIGHT},
	{"NudgeLeft", NUDGELEFT},
	{"NudgeUp", NUDGEUP},
	{"NudgeDown", NUDGEDOWN},
	{"BigNudgeRight", BIGNUDGERIGHT},
	{"BigNudgeLeft", BIGNUDGELEFT},
	{"BigNudgeUp", BIGNUDGEUP},
	{"BigNudgeDown", BIGNUDGEDOWN},
	{"HorizontalIncrement", HORIZINC},
	{"VerticalIncrement", VERTINC},
	{"HorizontalDecrement", HORIZDEC},
	{"VerticalDecrement", VERTDEC},
	{"ToggleDecor", TOGGLEDECOR},	
	{"ToggleTab", TOGGLETAB}, 
	{"RootMenu", ROOTMENU},
	{0, LASTKEYGRAB}
	};	

Keys::Keys(Display *display, char *filename):
m_abortkey(0),
m_display(display)
{
	assert(display);
	if (filename)
		load(filename);
}

Keys::~Keys() {	
	ungrabKeys();
	deleteTree();
}

//--------- deleteTree -----------
// Destroys the keytree and m_abortkey
//--------------------------------
void Keys::deleteTree() {
	while (!m_keylist.empty()) {
		if (m_keylist.back())
			delete m_keylist.back();		
		m_keylist.pop_back();
	}
	if (m_abortkey) {
		delete m_abortkey;
		m_abortkey=0;
	}	
}

//-------- ungrabKeys ---------
// Ungrabs the keys
//-----------------------------
void Keys::ungrabKeys() {
	for (int screen=0; screen<ScreenCount(m_display); screen++) {
		XUngrabKey(m_display, AnyKey, AnyModifier,
			RootWindow(m_display, screen));		
	}
}

//-------------- load ----------------
// Load and grab keys
// Returns true on success else false
// TODO: error checking
//------------------------------------
bool Keys::load(char *filename) {
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
		return false;
	
	
	auto_ptr<char> linebuffer(new char[1024]);
	
	int line=0;//current line, so we can tell the user where the fault is

	while (!infile.eof()) {
		
		infile.getline(linebuffer.get(), 1024);		

		line++;
		vector<string> val;
		//Parse arguments
		StringUtil::stringtok(val, linebuffer.get());
		//must have at least 1 argument
		if (val.size()<=0)
			continue;
			
		if (val[0][0]=='#') //the line is commented
			continue;
		
		unsigned int key=0, mod=0;		
		char keyarg=0;
		t_key *current_key=0, *last_key=0;
		
		for (unsigned int argc=0; argc<val.size(); argc++) {

			if (val[argc][0]!=':') {
				keyarg++;
				if (keyarg==1) //first arg is modifier
					mod = getModifier(val[argc].c_str());
				else if (keyarg>1) {

					//keyarg=0;
					int tmpmod=getModifier(val[argc].c_str());
					if(tmpmod)
						mod|=tmpmod; //If it's a modifier
					else{ 
						key = getKey(val[argc].c_str()); // else get the key
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

			} else {
				
				unsigned int i=0;

				for (i=0; i< LASTKEYGRAB; i++) {
					// +1 on the val[argc] because we dont want to compare the ':'
					if (strcasecmp(m_actionlist[i].string, val[argc].c_str()+1) == 0)
						break;	
				}

				if (i < LASTKEYGRAB ) {
					if (!current_key) {
						cerr<<"Error on line: "<<line<<endl;
						cerr<<linebuffer.get()<<endl;
						delete current_key;
						current_key = 0;
						last_key = 0;
						break; //break out and read next line
					}

					//special case for grabAbortKeychain
					if (m_actionlist[i].action == ABORTKEYCHAIN) {
						if (last_key!=current_key)
							cerr<<"Keys: "<<m_actionlist[i].string<<" cant be in chained mode"<<endl;
						else if (m_abortkey)
							cerr<<"Keys: "<<m_actionlist[i].string<<" is already bound."<<endl;
						else
							m_abortkey = new t_key(current_key->key, current_key->mod, ABORTKEYCHAIN);

						delete current_key;
						current_key = 0;
						last_key = 0;
						break; //break out and read next line
					}
					
					last_key->action = m_actionlist[i].action;
					switch(last_key->action) {
						case Keys::EXECUTE:
						last_key->execcommand = 
								const_cast<char *>
								(StringUtil::strcasestr(linebuffer.get(),
									getActionStr(Keys::EXECUTE))+
							strlen(getActionStr(Keys::EXECUTE)));
						break;
						case WORKSPACE:
                        case SENDTOWORKSPACE:
							if (argc + 1 < val.size())
								last_key->param = atoi( val[argc+1].c_str());
							else
								last_key->param = 0;
						break;
						case NEXTWINDOW:
                        case PREVWINDOW:
							if (argc + 1 < val.size())
								last_key->param = atoi( val[argc+1].c_str());
							else
								last_key->param = 0;
						break;
						case LEFTWORKSPACE:
						case RIGHTWORKSPACE:
						case NEXTWORKSPACE:
						case PREVWORKSPACE:
							if (argc + 1 < val.size())
								last_key->param = atoi( val[argc+1].c_str());
							else
								last_key->param = 1;
						break;
						case NUDGERIGHT:
						case NUDGELEFT:
						case NUDGEUP:
						case NUDGEDOWN:
							if (argc + 1 < val.size())
								last_key->param = atoi( val[argc+1].c_str());
							else
								last_key->param = 2;
						break;
						default:
						break;
					}

					//add the keychain to list										
					if (!mergeTree(current_key))
						cerr<<"Keys: Failed to merge keytree!"<<endl;
				
					//clear keypointers now that we have them in m_keylist					
					delete current_key;
					current_key = 0;
					last_key = 0;
					
				}	else { //destroy list if no action is found
					#ifdef DEBUG
					cerr<<"Didnt find action="<<val[argc]<<endl;
					#endif					
					//destroy current_key ... this will also destroy the last_key										
					delete current_key;
					current_key = 0;
					last_key = 0;
				}
				
				break; //dont process this linebuffer more
			}	
		}
	}

	return true;
}

//--------- grabKey ---------------------
// Grabs a key with the modifier
// and with numlock,capslock and scrollock
//----------------------------------------
void Keys::grabKey(unsigned int key, unsigned int mod) {

	for (int screen=0; screen<ScreenCount(m_display); screen++) {
		
		Window root = RootWindow(m_display, screen);
		
		XGrabKey(m_display, key, mod,
			root, True,
			GrabModeAsync, GrabModeAsync);
						
		// Grab with numlock, capslock and scrlock	

		//numlock	
		XGrabKey(m_display, key, mod|Mod2Mask,
			root, True,
			GrabModeAsync, GrabModeAsync);		
		//scrolllock
		XGrabKey(m_display, key, mod|Mod5Mask,
			root, True,
			GrabModeAsync, GrabModeAsync);	
		//capslock
		XGrabKey(m_display, key, mod|LockMask,
			root, True,
			GrabModeAsync, GrabModeAsync);
	
		//capslock+numlock
		XGrabKey(m_display, key, mod|LockMask|Mod2Mask,
			root, True,
			GrabModeAsync, GrabModeAsync);

		//capslock+scrolllock
		XGrabKey(m_display, key, mod|LockMask|Mod5Mask,
			root, True,
			GrabModeAsync, GrabModeAsync);						
	
		//capslock+numlock+scrolllock
		XGrabKey(m_display, key, mod|Mod2Mask|Mod5Mask|LockMask,
			root, True,
			GrabModeAsync, GrabModeAsync);						

		//numlock+scrollLock
		XGrabKey(m_display, key, mod|Mod2Mask|Mod5Mask,
			root, True,
			GrabModeAsync, GrabModeAsync);
	
	}
			
}

//------------ getModifier ---------------
// Returns the modifier for the modstr
// else zero on failure.
// TODO fix more masks
//----------------------------------------
unsigned int Keys::getModifier(const char *modstr) {
	if (!modstr)
		return 0;
	struct t_modlist{
		char *string;
		unsigned int mask;
		bool operator == (const char *modstr) {
			return  (strcasecmp(string, modstr) == 0 && mask !=0);
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
		
	for (unsigned int i=0; modlist[i].string!=0; i++) {
		if (modlist[i]==modstr)		
			return modlist[i].mask;		
	}
	
	return 0;	
}

//----------- getKey ----------------
// Returns keycode of keystr on success
// else it returns zero
//-----------------------------------
unsigned int Keys::getKey(const char *keystr) {
	if (!keystr)
		return 0;
	return XKeysymToKeycode(m_display,
		XStringToKeysym(keystr));
}

//--------- getAction -----------------
// returns the KeyAction of the XKeyEvent
//-------------------------------------
Keys::KeyAction Keys::getAction(XKeyEvent *ke) {	
	static t_key *next_key = 0;
	//remove numlock, capslock and scrolllock
	ke->state &= ~Mod2Mask & ~Mod5Mask & ~LockMask;
	
	if (m_abortkey && *m_abortkey==ke) { //abort current keychain
		next_key = 0;		
		return m_abortkey->action;
	}
	
	if (!next_key) {
	
		for (unsigned int i=0; i<m_keylist.size(); i++) {
			if (*m_keylist[i] == ke) {
				if (m_keylist[i]->keylist.size()) {
					next_key = m_keylist[i];
					break; //end for-loop 
				} else {
					if (m_keylist[i]->action == Keys::EXECUTE)
						m_execcmdstring = m_keylist[i]->execcommand; //update execcmdstring if action is grabExecute
					m_param = m_keylist[i]->param;
					return m_keylist[i]->action;
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
				if (temp_key->action == Keys::EXECUTE)
					m_execcmdstring = temp_key->execcommand; //update execcmdstring if action is grabExecute
				return temp_key->action;
			}
		}  else  {
			temp_key = next_key;		
			next_key = 0;
			if (temp_key->action == Keys::EXECUTE)
				m_execcmdstring = temp_key->execcommand; //update execcmdstring if action is grabExecute
			return temp_key->action;				
		}
		
	}
	
	return Keys::LASTKEYGRAB;
}

//--------- reconfigure -------------
// deletes the tree and load configuration
// returns true on success else false
//-----------------------------------
bool Keys::reconfigure(char *filename) {
	deleteTree();
	return load(filename);
}

//------------- getActionStr ------------------
// Tries to find the action for the key
// Returns actionstring on success else
// 0 on failure
//---------------------------------------------
const char *Keys::getActionStr(KeyAction action) {
	for (unsigned int i=0; m_actionlist[i].string!=0 ; i++) {
		if (m_actionlist[i].action == action) 
			return m_actionlist[i].string;
	}
	
	return 0;
}

#ifdef DEBUG
//--------- showTree -----------
// Debug function that show the
// keytree. Starts with the
// rootlist
//------------------------------
void Keys::showTree() {
	for (unsigned int i=0; i<m_keylist.size(); i++) {
		if (m_keylist[i]) {
			cerr<<i<<" ";
			showKeyTree(m_keylist[i]);	
		} else
			cerr<<"Null @ "<<i<<endl;
	}
}

//---------- showKeyTree --------
// Debug function to show t_key tree
//-------------------------------
void Keys::showKeyTree(t_key *key, unsigned int w) {	
	for (unsigned int i=0; i<w+1; i++)
		cerr<<"-";	
	if (!key->keylist.empty()) {
		for (unsigned int i=0; i<key->keylist.size(); i++) {
			cerr<<"( "<<(int)key->key<<" "<<(int)key->mod<<" )";
			showKeyTree(key->keylist[i], 4);		
			cerr<<endl;
		}
	} else
		cerr<<"( "<<(int)key->key<<" "<<(int)key->mod<<" ) {"<<getActionStr(key->action)<<"}"<<endl;
}
#endif //DEBUG
//------------ mergeTree ---------------
// Merges two chains and binds new keys
// Returns true on success else false.
//---------------------------------------
bool Keys::mergeTree(t_key *newtree, t_key *basetree) {
	if (basetree==0) {
		unsigned int baselist_i=0;
		for (; baselist_i<m_keylist.size(); baselist_i++) {
			if (m_keylist[baselist_i]->mod == newtree->mod && 
					m_keylist[baselist_i]->key == newtree->key) {
				if (newtree->keylist.size() && m_keylist[baselist_i]->action == LASTKEYGRAB) {
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

Keys::t_key::t_key(unsigned int key_, unsigned int mod_, KeyAction action_) {
	action = action_;
	key = key_;
	mod = mod_;	
	param = 0;
}

Keys::t_key::t_key(t_key *k) {
	action = k->action;
	key = k->key;
	mod = k->mod;
	execcommand = k->execcommand;
	param = k-> param;
}

Keys::t_key::~t_key() {	
	while (!keylist.empty()) {		
		t_key *k = keylist.back();
		delete k;
		keylist.pop_back();				
	}
}
