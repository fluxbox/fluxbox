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

// $Id: Keys.hh,v 1.6 2002/02/17 18:57:47 fluxgen Exp $

#ifndef KEYS_HH
#define KEYS_HH

#include <string>
#include <vector>
#include <X11/Xlib.h>

class Keys 
{
public:
enum KeyAction{
			ICONIFY=0,
			RAISE, LOWER,
			CLOSE,
			ABORTKEYCHAIN,
			WORKSPACE1, WORKSPACE2,  WORKSPACE3,  WORKSPACE4,	
			WORKSPACE5, WORKSPACE6,	 WORKSPACE7,  WORKSPACE8,	
			WORKSPACE9, WORKSPACE10, WORKSPACE11, WORKSPACE12,	
			NEXTWORKSPACE, PREVWORKSPACE,
			LEFTWORKSPACE, RIGHTWORKSPACE,
			KILLWINDOW, NEXTWINDOW,	PREVWINDOW,
			NEXTTAB, PREVTAB,
			SHADE, MAXIMIZE, STICK, 
			EXECUTE,	
			VERTMAX, HORIZMAX,	
			NUDGERIGHT, NUDGELEFT,NUDGEUP, NUDGEDOWN,	
			BIGNUDGERIGHT, BIGNUDGELEFT, BIGNUDGEUP, BIGNUDGEDOWN,
			HORIZINC, VERTINC, HORIZDEC, VERTDEC,
			TOGGLEDECOR,
			LASTKEYGRAB
	};
	
	Keys(Display *display, char *filename=0);
	~Keys();
	bool load(char *filename=0);
	KeyAction getAction(XKeyEvent *ke);
	bool reconfigure(char *filename);
	const char *getActionStr(KeyAction action);
	std::string getExecCommand() { return m_execcmdstring; }

private:
	void deleteTree();
	void ungrabKeys();
	void bindKey(unsigned int key, unsigned int mod);
	unsigned int getModifier(const char *modstr);
	unsigned int getKey(const char *keystr);
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
	};
	
	bool mergeTree(t_key *newtree, t_key *basetree=0);
	#ifdef DEBUG
	//debug functions
	void showTree();
	void showKeyTree(t_key *key, unsigned int w=0);
	#endif //DEBUG
	struct t_actionstr{
		const char *string;
		KeyAction action;
	};	
	static t_actionstr m_actionlist[];
	
	std::vector<t_key *> m_keylist;	
	t_key *m_abortkey; //abortkey for keygrabbing chain
	std::string m_execcmdstring; //copy of the execcommandstring
	Display *m_display;
};

#endif // _KEYS_HH_
