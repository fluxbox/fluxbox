// Key2.hh for Fluxbox - an X11 Window manager
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
#ifndef _KEYS_HH_
#define _KEYS_HH_

#include <string>
#include <vector>
	
class Keys 
{
public:
enum KeyAction{
			grabIconify=0,
			grabRaise,
			grabLower,
			grabClose,
			grabAbortKeychain,
			grabWorkspace1, grabWorkspace2, grabWorkspace3,	grabWorkspace4,	grabWorkspace5,
			grabWorkspace6,	grabWorkspace7,	grabWorkspace8,	grabWorkspace9,	grabWorkspace10,
			grabWorkspace11, grabWorkspace12,	grabNextWorkspace, grabPrevWorkspace,
			grabLeftWorkspace, grabRightWorkspace,
			grabKillWindow, grabNextWindow,	grabPrevWindow,
			grabNextTab, grabPrevTab,
			grabShade, grabMaximize, grabStick, grabExecute,	grabVertMax,
			grabHorizMax,	grabNudgeRight,	grabNudgeLeft,grabNudgeUp,
			grabNudgeDown,	grabBigNudgeRight, grabBigNudgeLeft,
			grabBigNudgeUp,	grabBigNudgeDown,
			grabHorizInc,	grabVertInc, grabHorizDec, grabVertDec,
			grabToggleDecor,
			lastKeygrab
	};
	
	Keys(char *filename);
	~Keys();
	bool load(char *filename=0);
	KeyAction getAction(XKeyEvent *ke);
	bool reconfigure(char *filename);
	const char *getActionStr(KeyAction action);
	std::string getExecCommand() { return m_execcmdstring; }
private:
	void deleteTree();
	
	void bindKey(unsigned int key, unsigned int mod);
	unsigned int getModifier(char *modstr);
	unsigned int getKey(char *keystr);
	void grabKey(unsigned int key, unsigned int mod);
	std::string filename;	
	
	class t_key {	
		public:
		t_key(unsigned int key, unsigned int mod, KeyAction action_ = lastKeygrab);
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
};

#endif // _KEYS_HH_
