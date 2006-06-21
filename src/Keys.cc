// Keys.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

//$Id$


#include "Keys.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/App.hh"
#include "FbTk/Command.hh"

#include "CommandParser.hh"
#include "FbTk/I18n.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif	// HAVE_CTYPE_H

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CERRNO
  #include <cerrno>
#else
  #include <errno.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

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
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>

using namespace std;

Keys::Keys():
    m_display(FbTk::App::instance()->display())
{

}

Keys::~Keys() {

    FbTk::KeyUtil::ungrabKeys();
    deleteTree();
}

/// Destroys the keytree
void Keys::deleteTree() {
    for (keyspace_t::iterator map_it = m_map.begin(); map_it != m_map.end(); ++map_it) {
        keylist_t::iterator it = map_it->second->begin();
        const keylist_t::iterator it_end = map_it->second->end();
        for ( ; it != it_end; it++)
            delete *it;
        map_it->second->clear();
        delete map_it->second;
    }
    m_map.clear();
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
    FbTk::KeyUtil::ungrabKeys();

    //free memory of previous grabs
    deleteTree();

    m_map["default:"] = new keylist_t;

    FbTk::App::instance()->sync(false);

    //open the file
    ifstream infile(filename);
    if (!infile)
        return false; // faild to open file

    m_current_line = 0;//current line, so we can tell the user where the fault is

    while (!infile.eof()) {
        string linebuffer;

        getline(infile, linebuffer);

        m_current_line++;

        addBinding(linebuffer);
    } // end while eof

    m_current_line = 0;
    m_filename = filename;
    m_keylist = m_map["default:"];
    return true;
}

bool Keys::save(const char *filename) const {
    //!!
    //!! TODO: fix keybinding saving
    //!! (we probably need to save key actions
    //!! as strings instead of creating new Commands)

    // open file for writing
    //    ofstream outfile(filename);
    //    if (!outfile)
    return false;
    //    return true;
}

bool Keys::addBinding(const std::string &linebuffer) {

    vector<string> val;
    // Parse arguments
    FbTk::StringUtil::stringtok(val, linebuffer.c_str());

    // must have at least 1 argument
    if (val.size() <= 0)
        return true; // empty lines are valid.

    if (val[0][0] == '#' || val[0][0] == '!' ) //the line is commented
        return true; // still a valid line.

    unsigned int key = 0, mod = 0;
    t_key *current_key=0, *last_key=0;
    size_t argc = 0;
    std::string keyMode = "default:";

    if (val[0][val[0].length()-1] == ':') {
        argc++;
        keyspace_t::iterator it = m_map.find(val[0]);
        if (it == m_map.end())
            m_map[val[0]] = new keylist_t;
        keyMode = val[0];
    }
    _FB_USES_NLS;
    // for each argument
    for (; argc < val.size(); argc++) {

        if (val[argc][0] != ':') { // parse key(s)

            int tmpmod = FbTk::KeyUtil::getModifier(val[argc].c_str());
            if(tmpmod)
                mod |= tmpmod; //If it's a modifier
            else if (strcasecmp("NONE",val[argc].c_str()) == 0)
                mod = 0;
            else {
                // keycode covers the following three two-byte cases:
                // 0x       - hex
                // +[1-9]   - number between +1 and +9
                // numbers 10 and above
                //
                if (val[argc].size() > 1 && (isdigit(val[argc][0]) && 
                                             (isdigit(val[argc][1]) || val[argc][1] == 'x') || 
                                             val[argc][0] == '+' && isdigit(val[argc][1])) ) {
                        
                    key = strtoul(val[argc].c_str(), NULL, 0);

                    if (errno == EINVAL || errno == ERANGE)
                        key = 0;

                } else // convert from string symbol
                    key = FbTk::KeyUtil::getKey(val[argc].c_str()); 

                if (key == 0) {
                    cerr<<_FB_CONSOLETEXT(Keys, InvalidKeyMod, 
                                  "Keys: Invalid key/modifier on line", 
                                  "A bad key/modifier string was found on line (number following)")<<" "<<
                        m_current_line<<"): "<<linebuffer<<endl;
                    return false;
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

        } else { // parse command line
            if (last_key == 0) {
                cerr<<_FB_CONSOLETEXT(Keys, BadLine, "Keys: Error on line", "Error on line (number following)")<<": "<<m_current_line<<endl;
                cerr<<"> "<<linebuffer<<endl;
                return false;
            }
            bool ret_val = true;
            const char *str =
                FbTk::StringUtil::strcasestr(linebuffer.c_str(),
                                             val[argc].c_str() + 1); // +1 to skip ':'
            if (str == 0) {
                cerr<<_FB_CONSOLETEXT(Keys, BadLine, "Keys: Error on line", "Error on line (number following)")<<": "<<m_current_line<<endl;
                cerr<<"> "<<linebuffer<<endl;
                ret_val = false;
            } else {

                last_key->m_command = CommandParser::instance().parseLine(str);

                if (*last_key->m_command == 0) {
                    cerr<<_FB_CONSOLETEXT(Keys, BadLine, "Keys: Error on line", "Error on line (number following)")<<": "<<m_current_line<<endl;
                    cerr<<"> "<<linebuffer<<endl;
                } else {
                    // need to change keymode here so it doesn't get changed by CommandParser
                    m_keylist = m_map[keyMode];
                    // Add the keychain to list
                    if (!mergeTree(current_key)) {
                        cerr<<_FB_CONSOLETEXT(Keys, BadMerge, "Keys: Failed to merge keytree!", "relatively technical error message. Key bindings are stored in a tree structure")<<endl;
                        ret_val = false;
                    }
                }
            }
            delete current_key;
            current_key = 0;
            last_key = 0;

            return ret_val;

        }  // end if
    } // end for

    return false;
}


/**
 @return the KeyAction of the XKeyEvent; return false if not bound
*/
bool Keys::doAction(XKeyEvent &ke) {

    ke.state = FbTk::KeyUtil::instance().cleanMods(ke.state);

    static struct t_key* next_key = 0;

    if (!next_key) {
        bool retval = false;
        // need a local keylist, in case m_command->execute() changes it
        keylist_t *keylist = m_keylist;
        for (size_t i = 0; i < keylist->size(); i++) {
            if (*(*keylist)[i] == ke) {
                if ((*keylist)[i]->keylist.size()) {
                    next_key = (*keylist)[i];
                    return true; //still counts as being grabbed
                }
                if (*(*keylist)[i]->m_command != 0) {
                    (*keylist)[i]->m_command->execute();
                    retval = true;
                }
            }
        }
        return retval;
    }
    t_key *temp_key = next_key->find(ke);
    if (temp_key) {
        if (temp_key->keylist.size()) {
            next_key = temp_key;
            return true;
        }
        next_key = 0;
        if (*temp_key->m_command == 0)
            return false;
        temp_key->m_command->execute();
        return true;
    }
    temp_key = next_key;
    next_key = 0;
    if (*temp_key->m_command == 0)
        return false;
    temp_key->m_command->execute();
    return true;
}

/**
 deletes the tree and load configuration
 returns true on success else false
*/
bool Keys::reconfigure(const char *filename) {
    return load(filename);
}

/**
 Merges two chains and binds new keys
 @return true on success else false.
*/
bool Keys::mergeTree(t_key *newtree, t_key *basetree) {
    size_t baselist_i = 0;
    if (basetree==0) {
        for (; baselist_i<m_keylist->size(); baselist_i++) {
            if ((*m_keylist)[baselist_i]->mod == newtree->mod &&
                (*m_keylist)[baselist_i]->key == newtree->key) {
                if (newtree->keylist.size() && *(*m_keylist)[baselist_i]->m_command == 0) {
                    //assumes the newtree only have one branch
                    return mergeTree(newtree->keylist[0], (*m_keylist)[baselist_i]);
                } else
                    break;
            }
        }

        if (baselist_i == m_keylist->size()) {
            FbTk::KeyUtil::grabKey(newtree->key, newtree->mod);
            m_keylist->push_back(new t_key(newtree));
            if (newtree->keylist.size())
                return mergeTree(newtree->keylist[0], m_keylist->back());
            return true;
        }

    } else {
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
            FbTk::KeyUtil::grabKey(newtree->key, newtree->mod);
            basetree->keylist.push_back(new t_key(newtree));
            if (newtree->keylist.size())
                return mergeTree(newtree->keylist[0], basetree->keylist.back());
            return true;
        }
    }

    return false;
}

void Keys::keyMode(std::string keyMode = "default") {
    keyspace_t::iterator it = m_map.find(keyMode + ":");
    if (it == m_map.end())
        m_keylist = m_map["default:"];
    else
        m_keylist = it->second;
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
