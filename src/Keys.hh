// Keys.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: Keys.hh,v 1.16 2002/11/13 14:35:01 fluxgen Exp $

#ifndef KEYS_HH
#define KEYS_HH

#include <string>
#include <vector>
#include <X11/Xlib.h>

class Keys 
{
public:
	/**
		Key actions
	*/
	enum KeyAction{
			ICONIFY=0,
			RAISE, LOWER,
			CLOSE,
			ABORTKEYCHAIN,
			WORKSPACE,
			WORKSPACE1, WORKSPACE2,  WORKSPACE3,  WORKSPACE4,	
			WORKSPACE5, WORKSPACE6,	 WORKSPACE7,  WORKSPACE8,	
			WORKSPACE9, WORKSPACE10, WORKSPACE11, WORKSPACE12,	
			SENDTOWORKSPACE, // Send window to a workspace
			NEXTWORKSPACE, PREVWORKSPACE,
			LEFTWORKSPACE, RIGHTWORKSPACE,
			KILLWINDOW, NEXTWINDOW,	PREVWINDOW,
			NEXTTAB, PREVTAB, FIRSTTAB, LASTTAB, MOVETABPREV, MOVETABNEXT,
			SHADE, MAXIMIZE, 
			STICK,       // Make Sticky
			EXECUTE,	// Run command
			VERTMAX,    // Maximize vertical
			HORIZMAX,	// Maximize horizontal
			NUDGERIGHT, NUDGELEFT,NUDGEUP, NUDGEDOWN,	
			BIGNUDGERIGHT, BIGNUDGELEFT, BIGNUDGEUP, BIGNUDGEDOWN,
			HORIZINC, VERTINC, HORIZDEC, VERTDEC,
			TOGGLEDECOR,// toggle visibility of decor (title, frame, handles)
			TOGGLETAB,  // toggle visibilty of tab
			ROOTMENU,   // pop up rootmenu
			LASTKEYGRAB //mark end of keygrabbs
	};
	/**
		Constructor
		@param display display connection
		@param filename file to load, default none
	*/
	Keys(Display *display, const char *filename=0);
	/// destructor
	~Keys();
	/**
		Load configuration from file
		@return true on success, else false
	*/
	bool load(const char *filename=0);
	/**
		Determine action from XKeyEvent
		@return KeyAction value
	*/
	KeyAction getAction(XKeyEvent *ke);
	/**
		Reload configuration from filename
		@return true on success, else false
	*/
	bool reconfigure(const char *filename);
	/**
		Get string value of the KeyAction enum value
		@return string of action
	*/
	const char *getActionStr(KeyAction action);
	/**
		Get command to execute (key action EXECUTE)
		@return string to command
	*/
	const std::string &getExecCommand() { return m_execcmdstring; }
	/**
		@return number of parameters
	*/
	int getParam() const { return m_param; }

private:
	void deleteTree();
	void ungrabKeys();
	void bindKey(unsigned int key, unsigned int mod);
	/**
		@param modstr modifier string (i.e Mod4, Mod5)
		@return modifier number that match modstr
	*/
	unsigned int getModifier(const char *modstr);
	/**
		@param keystr a key string (i.e F1, Enter)
		@return key number that match keystr
	*/
	unsigned int getKey(const char *keystr);
	/**
		grab a key
		@param key the key
		@param mod the modifier
	*/
	void grabKey(unsigned int key, unsigned int mod);
	std::string filename;	
	
	class t_key {	
		public:
		t_key(unsigned int key, unsigned int mod, KeyAction action_ = Keys::LASTKEYGRAB);
		t_key(t_key *k);
		~t_key();
		
		inline t_key *find(unsigned int key_, unsigned int mod_) {
			for (unsigned int i=0; i<keylist.size(); i++) {
				if (keylist[i]->key == key_ && keylist[i]->mod == mod_)
					return keylist[i];				
			}			
			return 0;
		}
		inline t_key *find(XKeyEvent *ke) {
			for (unsigned int i=0; i<keylist.size(); i++) {
				if (keylist[i]->key == ke->keycode && keylist[i]->mod == ke->state)
					return keylist[i];				
			}			
			return 0;
		}
			
		inline bool operator == (XKeyEvent *ke) {
			return (mod == ke->state && key == ke->keycode);
		}
		
		KeyAction action;
		unsigned int key;
		unsigned int mod;
		std::vector<t_key *> keylist;
		std::string execcommand;
        int param;              // parameter to comands
	};
	/**
		merge two linked list
		@return true on success, else false
	*/
	bool mergeTree(t_key *newtree, t_key *basetree=0);

#ifdef DEBUG
	/// debug function
	void showTree();
	/// debug function
	void showKeyTree(t_key *key, unsigned int w=0);
#endif //DEBUG
	/// determine key modifier maps for caps-, num- and scrolllock
	void determineModmap();

	struct t_actionstr{
		const char *string;
		KeyAction action;
	};

	int m_capslock_mod, m_numlock_mod, m_scrolllock_mod; ///< modifiers
		
	static t_actionstr m_actionlist[];
	
	std::vector<t_key *> m_keylist;	
	t_key *m_abortkey;           ///< abortkey for keygrabbing chain
	std::string m_execcmdstring; ///< copy of the execcommandstring
    int m_param;                 ///< copy of the param argument
	Display *m_display;			 ///< display connection
};

#endif // _KEYS_HH_
