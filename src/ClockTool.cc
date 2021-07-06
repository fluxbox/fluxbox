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
#include "FbTk/FbTime.hh"

#include <ctime>
#include <typeinfo>
#include <cstdio>

namespace {

const char SWITCHES_SECONDS[] = "crsSTX+";
const char SWITCHES_12_24H[] = "lIrkHT";
const char SWITCHES_24_12H[] = "kHTlIr";
const char SWITCH_AM_PM[] = "pP";
const char IGNORE_AFTER_TRIGGER[] = "0123456789";

int showSeconds(const std::string& fmt) {

    return FbTk::StringUtil::findCharFromAlphabetAfterTrigger(
        fmt, '%', SWITCHES_SECONDS, sizeof(SWITCHES_SECONDS), 0,
        IGNORE_AFTER_TRIGGER, sizeof(IGNORE_AFTER_TRIGGER)) != std::string::npos;
}

uint64_t calcNextTimeout(const std::string& fmt) {

    uint64_t now = FbTk::FbTime::system();
    uint64_t unit = FbTk::FbTime::IN_SECONDS;
    if (!showSeconds(fmt)) { // microseconds till next full minute
         unit *= 60L;
    }
    return FbTk::FbTime::remainingNext(now, unit);
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
            m_tool.timeFormat(), '%', SWITCHES_24_12H, sizeof(SWITCHES_24_12H), &found,
            IGNORE_AFTER_TRIGGER, sizeof(IGNORE_AFTER_TRIGGER));

        if (pos != std::string::npos) { // if so, exchange it with 12/24h
            std::string newformat = m_tool.timeFormat();
            newformat[pos+1] = SWITCHES_12_24H[found];

            if (found < 3) { // 24h? erase %P/%p (AM|PM / am|pm)
                pos = FbTk::StringUtil::findCharFromAlphabetAfterTrigger(
                    newformat, '%', SWITCH_AM_PM, sizeof(SWITCH_AM_PM), 0,
                    IGNORE_AFTER_TRIGGER, sizeof(IGNORE_AFTER_TRIGGER));
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
            m_tool.timeFormat(), '%', SWITCHES_24_12H, 3, 0,
            IGNORE_AFTER_TRIGGER, sizeof(IGNORE_AFTER_TRIGGER)) != std::string::npos) {
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
    m_tracker.join(theme.reconfigSig(), FbTk::MemFun(*this, &ClockTool::updateTime));

    std::string time_locale = setlocale(LC_TIME, NULL);
    size_t pos = time_locale.find('.');
    if (pos != std::string::npos)
        time_locale = time_locale.substr(pos+1);
    if (!time_locale.empty())
        m_stringconvertor.setSource(time_locale);

    _FB_USES_NLS;

    FbTk::RefCount<FbTk::Command<void> > update_graphic(new FbTk::SimpleCommand<ClockTool>(*this,
                                                                                    &ClockTool::updateTime));
    m_timer.setCommand(update_graphic);

    m_button.setGC(m_theme->textGC());

    // setup menu
    FbTk::RefCount<FbTk::Command<void> > saverc(FbTk::CommandParser<void>::instance().parse("saverc"));
    FbTk::MenuItem *item = new ClockMenuItem(*this);
    item->setCommand(saverc);
    menu.insertItem(item);
    FbTk::RefCount<FbTk::Command<void> > editformat_cmd(new EditClockFormatCmd());
    menu.insertCommand(_FB_XTEXT(Toolbar, ClockEditFormat,   "Edit Clock Format",   "edit Clock Format") , editformat_cmd);

    updateTime();
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
    updateTime();
}

void ClockTool::themeReconfigured() {

    // we replace only numbers with zeros because everything else should be
    // relatively static. if we replace all text with zeros then widths of
    // proportional fonts with some strftime formats will be considerably off.

    const FbTk::FbString& t = m_button.text().logical();
    const size_t s = t.size();
    size_t i;
    FbTk::FbString text(s + 2, '0'); // +2 for extra padding

    for (i = 0; i < s; ++i) {
        if (!isdigit(t[i]))
            text[i] = t[i];
    }

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

    // time() might result in a different seconds-since-epoch than
    // gettimeofday() due to the fact that time() might be implemented by just
    // returning the amount of elapsed full seconds without taking into
    // account the sum of accumulated sub-seconds might be bigger than a
    // second. in this situation time() is 1s behind gettimeofday() which would
    // result in having the same 'text' and thus fluxbox would skip this
    // round. reference:
    //
    //    http://stackoverflow.com/questions/22917318/time-and-gettimeofday-return-different-seconds/23597725#23597725

    uint64_t now = FbTk::FbTime::system();
    time_t t = static_cast<time_t>(now / 1000000L);

    if (t != static_cast<time_t>(-1)) {

        char            buf[255];
        int             len;
        struct tm*      type;
        FbTk::FbString  text;

        if ((type = localtime(&t)) == 0)
            goto restart_timer;

#ifdef HAVE_STRFTIME

        len = strftime(buf, sizeof(buf), m_timeformat->c_str(), type);
        if (len == 0)
            goto restart_timer;

        text = m_stringconvertor.recode(buf);
        if (m_button.text().logical() == text) {
            goto restart_timer;
        }

#else // dont have strftime so we have to set it to hour:minut
        //        sprintf(time_string, "%d:%d", );
#endif // HAVE_STRFTIME

        m_button.setText(text);
        themeReconfigured();
    }

restart_timer:
    m_timer.setTimeout(calcNextTimeout(*m_timeformat), true);
}

// Just change things that affect the size
void ClockTool::updateSizing() {
    m_button.setBorderWidth(m_theme->border().width());
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
