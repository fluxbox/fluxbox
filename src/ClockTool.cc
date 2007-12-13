// ClockTool.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

#include "ClockTool.hh"

#include "ToolTheme.hh"
#include "Screen.hh"
#include "FbTk/CommandRegistry.hh"
#include "CommandDialog.hh"
#include "fluxbox.hh"

#include "FbTk/SimpleCommand.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/Menu.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/I18n.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_CTIME
  #include <ctime>
#else
  #include <time.h>
#endif
#include <sys/time.h>
#include <string>
#include <typeinfo>

class ClockMenuItem: public FbTk::MenuItem {
public:
    explicit ClockMenuItem(ClockTool &tool):
        FbTk::MenuItem(""), m_tool(tool) { 
        // determine 12/24 hour format
        _FB_USES_NLS;
        if (m_tool.timeFormat().find("%k") != std::string::npos ||
            m_tool.timeFormat().find("%H") != std::string::npos ||
            m_tool.timeFormat().find("%T") != std::string::npos)
            setLabel( _FB_XTEXT(Toolbar, Clock24,   "Clock: 24h",   "set Clockmode to 24h") );
        else
            setLabel( _FB_XTEXT(Toolbar, Clock12,   "Clock: 12h",   "set Clockmode to 12h") );
    }

    void click(int button, int time) {
        std::string newformat = m_tool.timeFormat();
        size_t pos = newformat.find("%k");
        std::string newstr;
        bool clock24hour = true;

        _FB_USES_NLS;

        if (pos != std::string::npos)
            newstr = "%l";
        else if ((pos = newformat.find("%H")) != std::string::npos)
            newstr = "%I";
        else if ((pos = newformat.find("%T")) != std::string::npos)
            newstr = "%r";
        
        // 12 hour
        if (newstr.empty()) {
            clock24hour = false;
            if ((pos = newformat.find("%l")) != std::string::npos)
                newstr = "%k";
            else if ((pos = newformat.find("%I")) != std::string::npos)
                newstr = "%H";
            else if ((pos = newformat.find("%r")) != std::string::npos)
                newstr = "%T";
            
        }
        
        if (!newstr.empty()) {

            newformat.replace(pos, 2, newstr);
            if (!clock24hour) { // erase %P/%p (AM|PM / am|pm)
                pos = newformat.find("%p");
                if (pos != std::string::npos)
                    newformat.erase(pos, 2);
                else if ((pos = newformat.find("%P")) != std::string::npos)
                    newformat.erase(pos, 2);
            }

        
            m_tool.setTimeFormat(newformat);

            if (m_tool.timeFormat().find("%k") != std::string::npos ||
                m_tool.timeFormat().find("%H") != std::string::npos ||
                m_tool.timeFormat().find("%T") != std::string::npos)
                setLabel( _FB_XTEXT(Toolbar, Clock24,   "Clock: 24h",   "set Clockmode to 24h") );
            else
                setLabel( _FB_XTEXT(Toolbar, Clock12,   "Clock: 12h",   "set Clockmode to 12h") );
        
        } // else some other strange format...so we don't do anything
        FbTk::MenuItem::click(button, time);
    }
private:
    ClockTool &m_tool;
};

class EditClockFormatCmd: public FbTk::Command {
public:
    void execute() {
        BScreen *screen = Fluxbox::instance()->mouseScreen();
        if (screen == 0)
            return;
        std::string resourcename = screen->name() + ".strftimeFormat";

        CommandDialog *dialog = new CommandDialog(*screen, "Edit Clock Format", 
                                                  "SetResourceValue " + resourcename + " ");
        FbTk::RefCount<FbTk::Command> cmd(FbTk::CommandRegistry::instance().parseLine("reconfigure"));
        dialog->setPostCommand(cmd);
        dialog->setText(screen->resourceManager().resourceValue(resourcename));
        dialog->show();
    }
};

ClockTool::ClockTool(const FbTk::FbWindow &parent,
                     ToolTheme &theme, BScreen &screen, FbTk::Menu &menu):
    ToolbarItem(ToolbarItem::FIXED),
    m_button(parent, theme.font(), ""),
    m_theme(theme),
    m_screen(screen),
    m_pixmap(0),
    m_timeformat(screen.resourceManager(), std::string("%k:%M"), 
                 screen.name() + ".strftimeFormat", screen.altName() + ".StrftimeFormat"),
    m_stringconvertor(FbTk::StringConvertor::ToFbString) {
    // attach signals
    theme.reconfigSig().attach(this);

    std::string time_locale = setlocale(LC_TIME, NULL);
    size_t pos = time_locale.find('.');
    if (pos != std::string::npos)
        time_locale = time_locale.substr(pos+1);
    if (!time_locale.empty())
        m_stringconvertor.setSource(time_locale);

    _FB_USES_NLS;

    // setup timer to check the clock every 0.01 second
    // if nothing has changed, it wont update the graphics
    m_timer.setInterval(1);
    // m_timer.setTimeout(delay); // don't need to set timeout on interval timer
    FbTk::RefCount<FbTk::Command> update_graphic(new FbTk::SimpleCommand<ClockTool>(*this, 
                                                                                    &ClockTool::updateTime));
    m_timer.setCommand(update_graphic);
    m_timer.start();

    m_button.setGC(m_theme.textGC());

    // setup menu
    FbTk::RefCount<FbTk::Command> saverc(FbTk::CommandRegistry::instance().parseLine("saverc"));
    FbTk::MenuItem *item = new ClockMenuItem(*this);
    item->setCommand(saverc);
    menu.insert(item);
    FbTk::RefCount<FbTk::Command> editformat_cmd(new EditClockFormatCmd());
    menu.insert(_FB_XTEXT(Toolbar, ClockEditFormat,   "Edit Clock Format",   "edit Clock Format") , editformat_cmd);


    update(0);
}

ClockTool::~ClockTool() {
    // remove cached pixmap
    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);
}

void ClockTool::move(int x, int y) {
    m_button.move(x, y);
}

void ClockTool::resize(unsigned int width, unsigned int height) {
    m_button.resize(width, height);
    reRender();
    m_button.clear();
}

void ClockTool::moveResize(int x, int y,
                      unsigned int width, unsigned int height) {
    m_button.moveResize(x, y, width, height);
    reRender();
    m_button.clear();
}

void ClockTool::show() {
    m_button.show();
}

void ClockTool::hide() {
    m_button.hide();
}

void ClockTool::setTimeFormat(const std::string &format) {
    *m_timeformat = format;
    update(0);
}

void ClockTool::update(FbTk::Subject *subj) {
    updateTime();

    // + 2 to make the entire text fit inside
    // we only replace numbers with zeros because everything else should be
    // relatively static. If we replace all text with zeros then widths of
    // proportional fonts with some strftime formats will be considerably off.
    std::string text(m_button.text());

    int textlen = text.size();
    for (int i=0; i < textlen; ++i) {
        if (isdigit(text[i])) // don't bother replacing zeros
            text[i] = '0';
    }
    text.append("00"); // pad

    unsigned int new_width = m_button.width();
    unsigned int new_height = m_button.height();
    translateSize(orientation(), new_width, new_height);
    new_width = m_theme.font().textWidth(text.c_str(), text.size());
    translateSize(orientation(), new_width, new_height);
    if (new_width != m_button.width() || new_height != m_button.height()) {
        resize(new_width, new_height);
        resizeSig().notify();
    }

    if (subj != 0 && typeid(*subj) == typeid(ToolTheme))
        renderTheme(m_button.alpha());

}

unsigned int ClockTool::borderWidth() const { 
    return m_button.borderWidth();
}

unsigned int ClockTool::width() const {
    return m_button.width();
}

unsigned int ClockTool::height() const {
    return m_button.height();
}

void ClockTool::updateTime() {
    // update clock
    timeval now;
    gettimeofday(&now, 0);
    time_t the_time = now.tv_sec;

    if (the_time != -1) {
        char time_string[255];
        int time_string_len;
        struct tm *time_type = localtime(&the_time);
        if (time_type == 0)
            return;

#ifdef HAVE_STRFTIME
        time_string_len = strftime(time_string, 255, m_timeformat->c_str(), time_type);
        if( time_string_len == 0)
            return;
        std::string text = m_stringconvertor.recode(time_string);
        if (m_button.text() == text)
            return;

        m_button.setText(text);

        unsigned int new_width = m_theme.font().textWidth(time_string, time_string_len) + 2;
        if (new_width > m_button.width()) {
            resize(new_width, m_button.height());
            resizeSig().notify();
        }
#else // dont have strftime so we have to set it to hour:minut
        //        sprintf(time_string, "%d:%d", );
#endif // HAVE_STRFTIME
    }
}

// Just change things that affect the size
void ClockTool::updateSizing() {
    m_button.setBorderWidth(m_theme.border().width());
    // resizes if new timeformat 
    update(0);
}

void ClockTool::reRender() {
    if (m_pixmap) 
        m_screen.imageControl().removeImage(m_pixmap);

    if (m_theme.texture().usePixmap()) {
        m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                                                       m_theme.texture(), orientation());
        m_button.setBackgroundPixmap(m_pixmap);
    } else {
        m_pixmap = 0;
        m_button.setBackgroundColor(m_theme.texture().color());
    }
}


void ClockTool::renderTheme(unsigned char alpha) {
    m_button.setAlpha(alpha);
    m_button.setJustify(m_theme.justify());

    reRender();

    m_button.setBorderWidth(m_theme.border().width());
    m_button.setBorderColor(m_theme.border().color());
    m_button.clear();
}

void ClockTool::setOrientation(FbTk::Orientation orient) {
    m_button.setOrientation(orient);
    ToolbarItem::setOrientation(orient);
}
