// MenuItem.hh for FbTk - Fluxbox Toolkit
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: MenuItem.hh,v 1.2 2003/02/17 12:28:06 fluxgen Exp $

#ifndef FBTK_MENUITEM_HH
#define FBTK_MENUITEM_HH

#include "RefCount.hh"
#include "Command.hh"
#include <string>

namespace FbTk {

class Menu;

///   An interface for a menu item in Menu
class MenuItem {
public:
    MenuItem(
             const char *label)
        : m_label(label ? label : ""),
        m_submenu(0),
        m_enabled(true),
        m_selected(false)
    { }
    /// create a menu item with a specific command to be executed on click
    MenuItem(const char *label, RefCount<Command> &cmd):
        m_label(label ? label : ""),
        m_submenu(0),
        m_command(cmd),
        m_enabled(true),
        m_selected(false) {
		
    }

    MenuItem(const char *label, Menu *submenu)
        : m_label(label ? label : "")
        , m_submenu(submenu)
        , m_enabled(true)
        , m_selected(false)
    { }
    virtual ~MenuItem() { }

    void setCommand(RefCount<Command> &cmd) { m_command = cmd; }
    virtual void setSelected(bool selected) { m_selected = selected; }
    virtual void setEnabled(bool enabled) { m_enabled = enabled; }
    virtual void setLabel(const char *label) { m_label = (label ? label : ""); }
    Menu *submenu() { return m_submenu; }
    /** 
        @name accessors
    */
    //@{
    virtual const std::string &label() const { return m_label; }
    const Menu *submenu() const { return m_submenu; } 
    virtual bool isEnabled() const { return m_enabled; }
    virtual bool isSelected() const { return m_selected; }
    /**
       Called when the item was clicked with a specific button
       @param button the button number
       @param time the time stamp 
     */
    virtual void click(int button, int time);
    RefCount<Command> &command() { return m_command; }
    const RefCount<Command> &command() const { return m_command; }
    //@}
	
private:
    std::string m_label; ///< label of this item
    Menu *m_submenu; ///< a submenu, 0 if we don't have one
    RefCount<Command> m_command; ///< command to be executed
    bool m_enabled, m_selected;
};

};// end namespace FbTk

#endif // FBTK_MENUITEM_HH
