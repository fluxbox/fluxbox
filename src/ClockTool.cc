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

#include "ClockTool.hh"

#include "ToolTheme.hh"
#include "Screen.hh"
#include "FbTk/CommandParser.hh"
#include "CommandDialog.hh"
#include "fluxbox.hh"

#include "FbTk/MemFun.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/TextUtils.hh"
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
#include <typeinfo>


namespace {

static const char SWITCHES_SECONDS[] = "crsSTX+";
static const char SWITCHES_12_24H[] = "lIrkHT";
static const char SWITCHES_24_12H[] = "kHTlIr";
static const char SWITCH_AM_PM[] = "pP";

/**
 * return true if clock shows seconds. If clock doesn't show seconds then
 * there is no need to wake up every second to redraw the clock.
 */

int showSeconds(const std::string& fmt_string) {

    return FbTk::StringUtil::findCharFromAlphabetAfterTrigger(
        fmt_string, '%', SWITCHES_SECONDS, sizeof(SWITCHES_SECONDS), 0) != std::string::npos;
}


timeval calcNextTimeout(const std::string& fmt_string) {
    timeval now;
    timeval next;
    gettimeofday(&now, 0);
    next.tv_sec = 60 - (now.tv_sec % 60) - 1;
    next.tv_usec = 1000000 - now.tv_usec;
    if (next.tv_usec >= 1000000) {
        next.tv_sec++;
        next.tv_usec -= 1000000;
    }

    // wake up at next second-change
    if (showSeconds(fmt_string)) {
        next.tv_sec = 0;
    }

    return next;
}


} // end of anonymous namespace


class ClockMenuItem: public FbTk::MenuItem {
public:
    explicit ClockMenuItem(ClockTool &tool):
        FbTk::MenuItem(FbTk::BiDiString("")), m_tool(tool) {

        setClockModeLabel();
        setCloseOnClick(false);
    }

    void click(int button, int time, unsigned int mods) {

        // does the current format string contain something with 24/12h?
        size_t found;
        size_t pos = FbTk::StringUtil::findCharFromAlphabetAfterTrigger(
                m_tool.timeFormat(), '%', SWITCHES_24_12H, sizeof(SWITCHES_24_12H), &found);

        if (pos != std::string::npos) { // if so, exchange it with 12/24h
            std::string newformat = m_tool.timeFormat();
            newformat[pos+1] = SWITCHES_12_24H[found];

            if (found < 3) { // 24h? erase %P/%p (AM|PM / am|pm)
                pos = FbTk::StringUtil::findCharFromAlphabetAfterTrigger(
                    newformat, '%', SWITCH_AM_PM, sizeof(SWITCH_AM_PM), 0);
                if (pos != std::string::npos) {
                    newformat.erase(pos, 2);
                }
            }

            m_tool.setTimeFormat(newformat);
            setClockModeLabel();

        } // else some other strange format...so we don't do anything
        FbTk::MenuItem::click(button, time, mods);
    }
private:

    void setClockModeLabel() {
        _FB_USES_NLS;
        if (FbTk::StringUtil::findCharFromAlphabetAfterTrigger(
            m_tool.timeFormat(), '%', SWITCHES_24_12H, 3, 0) != std::string::npos) {
            setLabel( _FB_XTEXT(Toolbar, Clock24,   "Clock: 24h",   "set Clockmode to 24h") );
        } else {
            setLabel( _FB_XTEXT(Toolbar, Clock12,   "Clock: 12h",   "set Clockmode to 12h") );
        }
    }

    ClockTool &m_tool;
};

class EditClockFormatCmd: public FbTk::Command<void> {
public:
    void execute() {
        BScreen *screen = Fluxbox::instance()->mouseScreen();
        if (screen == 0)
            return;
        std::string resourcename = screen->name() + ".strftimeFormat";

        CommandDialog *dialog = new CommandDialog(*screen, "Edit Clock Format",
                                                  "SetResourceValue " + resourcename + " ");
        FbTk::RefCount<FbTk::Command<void> > cmd(FbTk::CommandParser<void>::instance().parse("reconfigure"));
        dialog->setPostCommand(cmd);
        dialog->setText(screen->resourceManager().resourceValue(resourcename));
        dialog->show();
    }
};

ClockTool::ClockTool(const FbTk::FbWindow &parent,
                     FbTk::ThemeProxy<ToolTheme> &theme, BScreen &screen,
                     FbTk::Menu &menu):
    ToolbarItem(ToolbarItem::FIXED),
    m_button(parent, theme->font(), FbTk::BiDiString("")),
    m_theme(theme),
    m_screen(screen),
    m_pixmap(0),
    m_timeformat(screen.resourceManager(), std::string("%k:%M"),
                 screen.name() + ".strftimeFormat", screen.altName() + ".StrftimeFormat"),
    m_stringconvertor(FbTk::StringConvertor::ToFbString) {
    // attach signals
    m_tracker.join(theme.reconfigSig(), FbTk::MemFun(*this, &ClockTool::themeReconfigured));

    std::string time_locale = setlocale(LC_TIME, NULL);
    size_t pos = time_locale.find('.');
    if (pos != std::string::npos)
        time_locale = time_locale.substr(pos+1);
    if (!time_locale.empty())
        m_stringconvertor.setSource(time_locale);

    _FB_USES_NLS;

    m_timer.setTimeout(calcNextTimeout(*m_timeformat));

    FbTk::RefCount<FbTk::Command<void> > update_graphic(new FbTk::SimpleCommand<ClockTool>(*this,
                                                                                    &ClockTool::updateTime));
    m_timer.setCommand(update_graphic);
    m_timer.start();

    m_button.setGC(m_theme->textGC());

    // setup menu
    FbTk::RefCount<FbTk::Command<void> > saverc(FbTk::CommandParser<void>::instance().parse("saverc"));
    FbTk::MenuItem *item = new ClockMenuItem(*this);
    item->setCommand(saverc);
    menu.insert(item);
    FbTk::RefCount<FbTk::Command<void> > editformat_cmd(new EditClockFormatCmd());
    menu.insert(_FB_XTEXT(Toolbar, ClockEditFormat,   "Edit Clock Format",   "edit Clock Format") , editformat_cmd);


    themeReconfigured();
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
    themeReconfigured();
}

void ClockTool::themeReconfigured() {
    updateTime();

    // + 2 to make the entire text fit inside
    // we only replace numbers with zeros because everything else should be
    // relatively static. If we replace all text with zeros then widths of
    // proportional fonts with some strftime formats will be considerably off.
    FbTk::FbString text(m_button.text().logical());

    int textlen = text.size();
    for (int i=0; i < textlen; ++i) {
        if (isdigit(text[i])) // don't bother replacing zeros
            text[i] = '0';
    }
    text.append("00"); // pad

    unsigned int new_width = m_button.width();
    unsigned int new_height = m_button.height();
    translateSize(orientation(), new_width, new_height);
    new_width = m_theme->font().textWidth(text.c_str(), text.size());
    translateSize(orientation(), new_width, new_height);
    if (new_width != m_button.width() || new_height != m_button.height()) {
        resize(new_width, new_height);
        resizeSig().emit();
    }

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

    m_timer.setTimeout(calcNextTimeout(*m_timeformat));

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
        if (m_button.text().logical() == text)
            return;

        m_button.setText(text);

        unsigned int new_width = m_theme->font().textWidth(time_string, time_string_len) + 2;
        if (new_width > m_button.width()) {
            resize(new_width, m_button.height());
            resizeSig().emit();
        }
#else // dont have strftime so we have to set it to hour:minut
        //        sprintf(time_string, "%d:%d", );
#endif // HAVE_STRFTIME
    }
}

// Just change things that affect the size
void ClockTool::updateSizing() {
    m_button.setBorderWidth(m_theme->border().width());
    // resizes if new timeformat
    themeReconfigured();
}

void ClockTool::reRender() {
    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);

    if (m_theme->texture().usePixmap()) {
        m_pixmap = m_screen.imageControl().renderImage(width(), height(),
                                                       m_theme->texture(), orientation());
        m_button.setBackgroundPixmap(m_pixmap);
    } else {
        m_pixmap = 0;
        m_button.setBackgroundColor(m_theme->texture().color());
    }
}


void ClockTool::renderTheme(int alpha) {
    m_button.setAlpha(alpha);
    m_button.setJustify(m_theme->justify());

    reRender();

    m_button.setBorderWidth(m_theme->border().width());
    m_button.setBorderColor(m_theme->border().color());
    m_button.clear();
}

void ClockTool::setOrientation(FbTk::Orientation orient) {
    m_button.setOrientation(orient);
    ToolbarItem::setOrientation(orient);
}
