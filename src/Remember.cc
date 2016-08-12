// Remember.cc for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                     and Simon Bowden    (rathnor at users.sourceforge.net)
// Copyright (c) 2002 Xavier Brouckaert
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

#include "Remember.hh"
#include "ClientPattern.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "FbMenu.hh"
#include "MenuCreator.hh"
#include "FbCommands.hh"
#include "fluxbox.hh"
#include "Layer.hh"
#include "Debug.hh"

#include "FbTk/I18n.hh"
#include "FbTk/FbString.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/App.hh"
#include "FbTk/stringstream.hh"
#include "FbTk/Transparent.hh"
#include "FbTk/AutoReloadHelper.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Util.hh"

#include <cstring>
#include <set>


using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::set;
using std::make_pair;
using std::ifstream;
using std::ofstream;
using std::hex;
using std::dec;

using FbTk::StringUtil::getStringBetween;
using FbTk::StringUtil::removeFirstWhitespace;
using FbTk::StringUtil::removeTrailingWhitespace;
using FbTk::StringUtil::toLower;
using FbTk::StringUtil::toLower;
using FbTk::StringUtil::extractNumber;
using FbTk::StringUtil::expandFilename;

namespace {

inline bool isComment(std::string& line) {
    removeFirstWhitespace(line);
    removeTrailingWhitespace(line);
    if (line.size() == 0 || line[0] == '#')
        return true;
    return false;
}

}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

class Application {
public:
    Application(bool transient, bool grouped, ClientPattern *pat = 0);
    void reset();
    void forgetWorkspace() { workspace_remember = false; }
    void forgetHead() { head_remember = false; }
    void forgetDimensions() { dimensions_remember = false; }
    void forgetPosition() { position_remember = false; }
    void forgetShadedstate() { shadedstate_remember = false; }
    void forgetTabstate() { tabstate_remember = false; }
    void forgetDecostate() { decostate_remember = false; }
    void forgetFocusHiddenstate() { focushiddenstate_remember= false; }
    void forgetIconHiddenstate() { iconhiddenstate_remember= false; }
    void forgetStuckstate() { stuckstate_remember = false; }
    void forgetFocusProtection() { focusprotection_remember = false; }
    void forgetJumpworkspace() { jumpworkspace_remember = false; }
    void forgetLayer() { layer_remember = false; }
    void forgetSaveOnClose() { save_on_close_remember = false; }
    void forgetAlpha() { alpha_remember = false; }
    void forgetMinimizedstate() { minimizedstate_remember = false; }
    void forgetMaximizedstate() { maximizedstate_remember = false; }
    void forgetFullscreenstate() { fullscreenstate_remember = false; }

    void rememberWorkspace(int ws)
        { workspace = ws; workspace_remember = true; }
    void rememberHead(int h)
        { head = h; head_remember = true; }
    void rememberDimensions(int width, int height, bool is_w_relative, bool is_h_relative)
        {
          dimension_is_w_relative = is_w_relative;
          dimension_is_h_relative = is_h_relative;
          w = width; h = height;
          dimensions_remember = true;
        }
    void rememberFocusHiddenstate(bool state)
        { focushiddenstate= state; focushiddenstate_remember= true; }
    void rememberIconHiddenstate(bool state)
        { iconhiddenstate= state; iconhiddenstate_remember= true; }
    void rememberPosition(int posx, int posy, bool is_x_relative, bool is_y_relative,
                 FluxboxWindow::ReferenceCorner rfc = FluxboxWindow::LEFTTOP)
        {
          position_is_x_relative = is_x_relative;
          position_is_y_relative = is_y_relative;
          x = posx; y = posy;
          refc = rfc;
          position_remember = true;
        }
    void rememberShadedstate(bool state)
        { shadedstate = state; shadedstate_remember = true; }
    void rememberTabstate(bool state)
        { tabstate = state; tabstate_remember = true; }
    void rememberDecostate(unsigned int state)
        { decostate = state; decostate_remember = true; }
    void rememberStuckstate(bool state)
        { stuckstate = state; stuckstate_remember = true; }
    void rememberFocusProtection(unsigned int protect)
        { focusprotection = protect; focusprotection_remember = true; }
    void rememberJumpworkspace(bool state)
        { jumpworkspace = state; jumpworkspace_remember = true; }
    void rememberLayer(int layernum)
        { layer = layernum; layer_remember = true; }
    void rememberSaveOnClose(bool state)
        { save_on_close = state; save_on_close_remember = true; }
    void rememberAlpha(int focused_a, int unfocused_a)
        { focused_alpha = focused_a; unfocused_alpha = unfocused_a; alpha_remember = true; }
    void rememberMinimizedstate(bool state)
        { minimizedstate = state; minimizedstate_remember = true; }
    void rememberMaximizedstate(int state)
        { maximizedstate = state; maximizedstate_remember = true; }
    void rememberFullscreenstate(bool state)
        { fullscreenstate = state; fullscreenstate_remember = true; }

    bool workspace_remember;
    unsigned int workspace;

    bool head_remember;
    int head;

    bool dimensions_remember;
    int w,h; // width, height
    bool dimension_is_w_relative;
    bool dimension_is_h_relative;

    bool ignoreSizeHints_remember;

    bool position_remember;
    int x,y;
    bool position_is_x_relative;
    bool position_is_y_relative;
    FluxboxWindow::ReferenceCorner refc;

    bool alpha_remember;
    int focused_alpha;
    int unfocused_alpha;

    bool shadedstate_remember;
    bool shadedstate;

    bool tabstate_remember;
    bool tabstate;

    bool decostate_remember;
    unsigned int decostate;

    bool stuckstate_remember;
    bool stuckstate;

    bool focusprotection_remember;
    unsigned int focusprotection;

    bool focushiddenstate_remember;
    bool focushiddenstate;

    bool iconhiddenstate_remember;
    bool iconhiddenstate;

    bool jumpworkspace_remember;
    bool jumpworkspace;

    bool layer_remember;
    int layer;

    bool save_on_close_remember;
    bool save_on_close;

    bool minimizedstate_remember;
    bool minimizedstate;

    bool maximizedstate_remember;
    int maximizedstate;

    bool fullscreenstate_remember;
    bool fullscreenstate;

    bool is_transient, is_grouped;
    FbTk::RefCount<ClientPattern> group_pattern;

};






Application::Application(bool transient, bool grouped, ClientPattern *pat):
    is_transient(transient), is_grouped(grouped), group_pattern(pat)
{
    reset();
}

void Application::reset() {
    decostate_remember =
        dimensions_remember =
        focushiddenstate_remember =
        iconhiddenstate_remember =
        jumpworkspace_remember =
        layer_remember  =
        position_remember =
        shadedstate_remember =
        stuckstate_remember =
        focusprotection_remember =
        tabstate_remember =
        workspace_remember =
        head_remember =
        alpha_remember =
        minimizedstate_remember =
        maximizedstate_remember =
        fullscreenstate_remember =
        save_on_close_remember = false;
}

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

namespace {

// replace special chars like ( ) and [ ] with \( \) and \[ \]
string escapeRememberChars(const string& str) {
    if (str.empty())
        return str;

    string escaped_str;
    escaped_str.reserve(str.capacity());

    string::const_iterator i;
    for (i = str.begin(); i != str.end(); ++i) {
        switch (*i) {
            case '(': case ')': case '[': case ']':
                escaped_str += '\\';
            default:
                escaped_str += *i;
                break;
        }
    }

    return escaped_str;
}

class RememberMenuItem : public FbTk::MenuItem {
public:
    RememberMenuItem(const FbTk::BiDiString &label,
                     const Remember::Attribute attrib) :
        FbTk::MenuItem(label),
        m_attrib(attrib) {
        setToggleItem(true);
        setCloseOnClick(false);
    }

    bool isSelected() const {
        if (FbMenu::window() == 0)
            return false;

        if (FbMenu::window()->numClients()) // ensure it HAS clients
            return Remember::instance().isRemembered(FbMenu::window()->winClient(), m_attrib);
        return false;
    }

    bool isEnabled() const {
        if (FbMenu::window() == 0)
            return false;

        if (m_attrib != Remember::REM_JUMPWORKSPACE)
            return true;
        else if (FbMenu::window()->numClients())
            return (Remember::instance().isRemembered(FbMenu::window()->winClient(), Remember::REM_WORKSPACE));
        return false;
    }

    void click(int button, int time, unsigned int mods) {
        // reconfigure only does stuff if the apps file has changed
        Remember& r = Remember::instance();
        r.checkReload();
        if (FbMenu::window() != 0) {
            WinClient& wc = FbMenu::window()->winClient();
            if (isSelected()) {
                r.forgetAttrib(wc, m_attrib);
            } else {
                r.rememberAttrib(wc, m_attrib);
            }
        }
        r.save();
        FbTk::MenuItem::click(button, time, mods);
    }

private:
    Remember::Attribute m_attrib;
};

FbTk::Menu *createRememberMenu(BScreen &screen) {

    _FB_USES_NLS;

    static const struct { bool is_alpha; const FbTk::BiDiString label; Remember::Attribute attr; } _entries[] = {
        { false, _FB_XTEXT(Remember, Workspace, "Workspace", "Remember Workspace"), Remember::REM_WORKSPACE },
        { false, _FB_XTEXT(Remember, JumpToWorkspace, "Jump to workspace", "Change active workspace to remembered one on open"), Remember::REM_JUMPWORKSPACE },
        { false, _FB_XTEXT(Remember, Head, "Head", "Remember Head"), Remember::REM_HEAD},
        { false, _FB_XTEXT(Remember, Dimensions, "Dimensions", "Remember Dimensions - with width and height"), Remember::REM_DIMENSIONS},
        { false, _FB_XTEXT(Remember, Position, "Position", "Remember position - window co-ordinates"), Remember::REM_POSITION},
        { false, _FB_XTEXT(Remember, Sticky, "Sticky", "Remember Sticky"), Remember::REM_STUCKSTATE},
        { false, _FB_XTEXT(Remember, Decorations, "Decorations", "Remember window decorations"), Remember::REM_DECOSTATE},
        { false, _FB_XTEXT(Remember, Shaded, "Shaded", "Remember shaded"), Remember::REM_SHADEDSTATE},
        { false, _FB_XTEXT(Remember, Minimized, "Minimized", "Remember minimized"), Remember::REM_MINIMIZEDSTATE},
        { false, _FB_XTEXT(Remember, Maximized, "Maximized", "Remember maximized"), Remember::REM_MAXIMIZEDSTATE},
        { false, _FB_XTEXT(Remember, Fullscreen, "Fullscreen", "Remember fullscreen"), Remember::REM_FULLSCREENSTATE},
        { true,  _FB_XTEXT(Remember, Alpha, "Transparency", "Remember window tranparency settings"), Remember::REM_ALPHA},
        { false, _FB_XTEXT(Remember, Layer, "Layer", "Remember Layer"), Remember::REM_LAYER},
        { false, _FB_XTEXT(Remember, SaveOnClose, "Save on close", "Save remembered attributes on close"), Remember::REM_SAVEONCLOSE}
    };
    bool needs_alpha = (FbTk::Transparent::haveComposite() || FbTk::Transparent::haveRender());

    // each fluxboxwindow has its own windowmenu
    // so we also create a remember menu just for it...
    FbTk::Menu *menu = MenuCreator::createMenu("Remember", screen);
    size_t i;
    for (i = 0; i < sizeof(_entries)/sizeof(_entries[0]); i++) {
        if (_entries[i].is_alpha && !needs_alpha) { // skip alpha-entry when not needed
            continue;
        }
        menu->insertItem(new RememberMenuItem(_entries[i].label, _entries[i].attr));
    }

    menu->updateMenu();
    return menu;
}

// offset is the offset in the string that we start looking from
// return true if all ok, false on error
bool handleStartupItem(const string &line, int offset) {

    Fluxbox* fb = Fluxbox::instance();
    unsigned int screen = fb->keyScreen()->screenNumber();
    int next = 0;
    string str;

    // accept some options, for now only "screen=NN"
    // these option are given in parentheses before the command
    next = getStringBetween(str, line.c_str() + offset, '(', ')');
    if (next > 0) {
        // there are some options
        string option;
        int pos = str.find('=');
        bool error = false;
        if (pos > 0) {
            option = str.substr(0, pos);
            if (strcasecmp(option.c_str(), "screen") == 0) {
                error = !extractNumber(str.c_str() + pos + 1, screen);
            } else {
                error = true;
            }
        } else {
            error = true;
        }
        if (error) {
            cerr<<"Error parsing startup options."<<endl;
            return false;
        }
    } else {
        next = 0;
    }

    next = getStringBetween(str, line.c_str() + offset + next, '{', '}');

    if (next <= 0) {
        cerr<<"Error parsing [startup] at column "<<offset<<" - expecting {command}."<<endl;
        return false;
    }

    // don't run command if fluxbox is restarting
    if (fb->findScreen(screen)->isRestart())
        // the line was successfully read; we just didn't use it
        return true;

    FbCommands::ExecuteCmd *tmp_exec_cmd = new FbCommands::ExecuteCmd(str, screen);

    fbdbg<<"Executing startup Command<void> '"<<str<<"' on screen "<<screen<<endl;

    tmp_exec_cmd->execute();
    delete tmp_exec_cmd;
    return true;
}



// returns number of lines read
// optionally can give a line to read before the first (lookahead line)
int parseApp(ifstream &file, Application &app, string *first_line = 0) {
    string line;
    _FB_USES_NLS;
    Focus::Protection protect = Focus::NoProtection;
    bool remember_protect = false;
    int row = 0;
    while (! file.eof()) {
        if (!(first_line || getline(file, line))) {
            continue;
        }

        if (first_line) {
            line = *first_line;
            first_line = 0;
        }

        row++;
        if (isComment(line)) {
            continue;
        }

        string str_key, str_option, str_label;
        int parse_pos = 0;
        int err = getStringBetween(str_key, line.c_str(), '[', ']');
        if (err > 0) {
            int tmp = getStringBetween(str_option, line.c_str() + err, '(', ')');
            if (tmp > 0)
                err += tmp;
        }
        if (err > 0 ) {
            parse_pos += err;
            getStringBetween(str_label, line.c_str() + parse_pos, '{', '}');
        } else
            continue; //read next line

        bool had_error = false;

        if (str_key.empty())
            continue; //read next line

        str_key = toLower(str_key);
        str_label = toLower(str_label);

        if (str_key == "workspace") {
            unsigned int w;
            if (extractNumber(str_label, w))
                app.rememberWorkspace(w);
            else
                had_error = true;
        } else if (str_key == "head") {
            unsigned int h;
            if (extractNumber(str_label, h))
                app.rememberHead(h);
            else
                had_error = true;
        } else if (str_key == "layer") {
            int l = ResourceLayer::getNumFromString(str_label);
            had_error = (l == -1);
            if (!had_error)
                app.rememberLayer(l);
        } else if (str_key == "dimensions") {
            std::vector<string> tokens;
            FbTk::StringUtil::stringtok<std::vector<string> >(tokens, str_label);
            if (tokens.size() == 2) {
                unsigned int h, w;
                bool h_relative, w_relative, ignore;
                w = FbTk::StringUtil::parseSizeToken(tokens[0], w_relative, ignore);
                h = FbTk::StringUtil::parseSizeToken(tokens[1], h_relative, ignore);
                app.rememberDimensions(w, h, w_relative, h_relative);
            } else
                had_error = true;
        } else if (str_key == "ignoresizehints") {
            app.ignoreSizeHints_remember = str_label == "yes";
        } else if (str_key == "position") {
            FluxboxWindow::ReferenceCorner r = FluxboxWindow::LEFTTOP;
            // more info about the parameter
            // in ::rememberPosition

            if (str_option.length())
                r = FluxboxWindow::getCorner(str_option);
            if (!(had_error = (r == FluxboxWindow::ERROR))) {
                std::vector<string> tokens;
                FbTk::StringUtil::stringtok<std::vector<string> >(tokens, str_label);
                if (tokens.size() == 2) {
                    int x, y;
                    bool x_relative, y_relative, ignore;
                    x = FbTk::StringUtil::parseSizeToken(tokens[0], x_relative, ignore);
                    y = FbTk::StringUtil::parseSizeToken(tokens[1], y_relative, ignore);
                    app.rememberPosition(x, y, x_relative, y_relative, r);
                } else
                    had_error = true;
            }
        } else if (str_key == "shaded") {
            app.rememberShadedstate(str_label == "yes");
        } else if (str_key == "tab") {
            app.rememberTabstate(str_label == "yes");
        } else if (str_key == "focushidden") {
            app.rememberFocusHiddenstate(str_label == "yes");
        } else if (str_key == "iconhidden") {
            app.rememberIconHiddenstate(str_label == "yes");
        } else if (str_key == "hidden") {
            app.rememberIconHiddenstate(str_label == "yes");
            app.rememberFocusHiddenstate(str_label == "yes");
        } else if (str_key == "deco") {
            int deco = WindowState::getDecoMaskFromString(str_label);
            if (deco == -1)
                had_error = 1;
            else
                app.rememberDecostate((unsigned int)deco);
        } else if (str_key == "alpha") {
            int focused_a, unfocused_a;
            switch (sscanf(str_label.c_str(), "%i %i", &focused_a, &unfocused_a)) {
            case 1: // 'alpha <focus>'
                unfocused_a = focused_a;
            case 2: // 'alpha <focus> <unfocus>'
                focused_a = FbTk::Util::clamp(focused_a, 0, 255);
                unfocused_a = FbTk::Util::clamp(unfocused_a, 0, 255);
                app.rememberAlpha(focused_a, unfocused_a);
                break;
            default:
                had_error = true;
                break;
            }
        } else if (str_key == "sticky") {
            app.rememberStuckstate(str_label == "yes");
        } else if (str_key == "focusnewwindow") {
            remember_protect = true;
            if (!(protect & (Focus::Gain|Focus::Refuse))) { // cut back on contradiction
                if (str_label == "yes")
                    protect |= Focus::Gain;
                else
                    protect |= Focus::Refuse;
            }
        } else if (str_key == "focusprotection") {
            remember_protect = true;
            std::list<std::string> labels;
            FbTk::StringUtil::stringtok(labels, str_label, ", ");
            std::list<std::string>::iterator it = labels.begin();
            for (; it != labels.end(); ++it) {
                if (*it == "lock")
                    protect = (protect & ~Focus::Deny) | Focus::Lock;
                else if (*it == "deny")
                    protect = (protect & ~Focus::Lock) | Focus::Deny;
                else if (*it == "gain")
                    protect = (protect & ~Focus::Refuse) | Focus::Gain;
                else if (*it == "refuse")
                    protect = (protect & ~Focus::Gain) | Focus::Refuse;
                else if (*it == "none")
                    protect = Focus::NoProtection;
                else
                    had_error = 1;
            }
        } else if (str_key == "minimized") {
            app.rememberMinimizedstate(str_label == "yes");
        } else if (str_key == "maximized") {
            WindowState::MaximizeMode m = WindowState::MAX_NONE;
            if (str_label == "yes")
                m = WindowState::MAX_FULL;
            else if (str_label == "horz")
                m = WindowState::MAX_HORZ;
            else if (str_label == "vert")
                m = WindowState::MAX_VERT;
            app.rememberMaximizedstate(m);
        } else if (str_key == "fullscreen") {
            app.rememberFullscreenstate(str_label == "yes");
        } else if (str_key == "jump") {
            app.rememberJumpworkspace(str_label == "yes");
        } else if (str_key == "close") {
            app.rememberSaveOnClose(str_label == "yes");
        } else if (str_key == "end") {
            break;
        } else {
            cerr << _FB_CONSOLETEXT(Remember, Unknown, "Unknown apps key", "apps entry type not known")<<" = " << str_key << endl;
        }
        if (had_error) {
            cerr<<"Error parsing apps entry: ("<<line<<")"<<endl;
        }
    }
    if (remember_protect)
        app.rememberFocusProtection(protect);
    return row;
}



/*
  This function is used to search for old instances of the same pattern
  (when reloading apps file). More than one pattern might match, but only
  if the application is the same (also note that they'll be adjacent).
  We REMOVE and delete any matching patterns from the old list, as they're
  effectively moved into the new
*/

Application* findMatchingPatterns(ClientPattern *pat, Remember::Patterns *patlist, bool transient, bool is_group, ClientPattern *match_pat = 0) {

    Remember::Patterns::iterator it = patlist->begin();
    Remember::Patterns::iterator it_end = patlist->end();

    for (; it != it_end; ++it) {
        if (*it->first == *pat && is_group == it->second->is_grouped &&
            transient == it->second->is_transient &&
            ((match_pat == 0 && it->second->group_pattern == 0) ||
             (match_pat && *match_pat == *it->second->group_pattern))) {

            Application *ret = it->second;

            if (!is_group) return ret;
            // find the rest of the group and remove it from the list

            // rewind
            Remember::Patterns::iterator tmpit = it;
            while (tmpit != patlist->begin()) {
                --tmpit;
                if (tmpit->second == ret)
                    it = tmpit;
                else
                    break;
            }

            // forward
            for(; it != it_end && it->second == ret; ++it) {
                delete it->first;
            }
            patlist->erase(patlist->begin(), it);

            return ret;
        }
    }

    return 0;
}

} // end anonymous namespace

/*------------------------------------------------------------------*\
\*------------------------------------------------------------------*/

Remember *Remember::s_instance = 0;

Remember::Remember():
    m_pats(new Patterns()),
    m_reloader(new FbTk::AutoReloadHelper()) {

    setName("remember");

    if (s_instance != 0)
        throw string("Can not create more than one instance of Remember");

    s_instance = this;
    enableUpdate();

    m_reloader->setReloadCmd(FbTk::RefCount<FbTk::Command<void> >(new FbTk::SimpleCommand<Remember>(*this, &Remember::reload)));
    reconfigure();
}

Remember::~Remember() {

    // free our resources

    // the patterns free the "Application"s
    // the client mapping shouldn't need cleaning
    Patterns::iterator it;
    set<Application *> all_apps; // no duplicates
    while (!m_pats->empty()) {
        it = m_pats->begin();
        delete it->first; // ClientPattern
        all_apps.insert(it->second); // Application, not necessarily unique
        m_pats->erase(it);
    }

    set<Application *>::iterator ait = all_apps.begin(); // no duplicates
    for (; ait != all_apps.end(); ++ait) {
        delete (*ait);
    }

    delete(m_reloader);

    s_instance = 0;
}

Application* Remember::find(WinClient &winclient) {
    // if it is already associated with a application, return that one
    // otherwise, check it against every pattern that we've got
    Clients::iterator wc_it = m_clients.find(&winclient);
    if (wc_it != m_clients.end())
        return wc_it->second;
    else {
        Patterns::iterator it = m_pats->begin();
        for (; it != m_pats->end(); ++it)
            if (it->first->match(winclient) &&
                it->second->is_transient == winclient.isTransient()) {
                it->first->addMatch();
                m_clients[&winclient] = it->second;
                return it->second;
            }
    }
    // oh well, no matches
    return 0;
}

Application * Remember::add(WinClient &winclient) {
    ClientPattern *p = new ClientPattern();
    Application *app = new Application(winclient.isTransient(), false);

    // by default, we match against the WMClass of a window (instance and class strings)
    string win_name  = ::escapeRememberChars(p->getProperty(ClientPattern::NAME,  winclient));
    string win_class = ::escapeRememberChars(p->getProperty(ClientPattern::CLASS, winclient));
    string win_role  = ::escapeRememberChars(p->getProperty(ClientPattern::ROLE,  winclient));

    p->addTerm(win_name,  ClientPattern::NAME);
    p->addTerm(win_class, ClientPattern::CLASS);
    if (!win_role.empty())
        p->addTerm(win_role, ClientPattern::ROLE);
    m_clients[&winclient] = app;
    p->addMatch();
    m_pats->push_back(make_pair(p, app));
    return app;
}




void Remember::reconfigure() {
    m_reloader->setMainFile(Fluxbox::instance()->getAppsFilename());
}

void Remember::checkReload() {
    m_reloader->checkReload();
}

void Remember::reload() {

    Fluxbox& fb = *Fluxbox::instance();
    string apps_string = expandFilename(fb.getAppsFilename());
    bool ok = true;

    fbdbg<<"("<<__FUNCTION__<<"): Loading apps file ["<<apps_string<<"]"<<endl;

    ifstream apps_file(apps_string.c_str());

    // we merge the old patterns with new ones
    Patterns *old_pats = m_pats.release();
    set<Application *> reused_apps;
    m_pats.reset(new Patterns());
    m_startups.clear();

    if (apps_file.fail()) {
        ok = false;
        cerr << "failed to open apps file " << apps_string << endl;
    }

    if (ok && apps_file.eof()) {
        ok = false;
        fbdbg<<"("<<__FUNCTION__<< ") Empty apps file" << endl;
    }

    if (ok) {
        string line;
        int row = 0;
        bool in_group = false;
        ClientPattern *pat = 0;
        list<ClientPattern *> grouped_pats;
        while (getline(apps_file, line) && ! apps_file.eof()) {
            row++;

            if (isComment(line)) {
                continue;
            }

            string key;
            int err=0;
            int pos = getStringBetween(key, line.c_str(), '[', ']');
            string lc_key = toLower(key);

            if (pos > 0 && (lc_key == "app" || lc_key == "transient")) {
                ClientPattern *pat = new ClientPattern(line.c_str() + pos);
                if (!in_group) {
                    if ((err = pat->error()) == 0) {
                        bool transient = (lc_key == "transient");
                        Application *app = findMatchingPatterns(pat,
                                               old_pats, transient, false);
                        if (app) {
                            app->reset();
                            reused_apps.insert(app);
                        } else {
                            app = new Application(transient, false);
                        }

                        m_pats->push_back(make_pair(pat, app));
                        row += parseApp(apps_file, *app);
                    } else {
                        cerr<<"Error reading apps file at line "<<row<<", column "<<(err+pos)<<"."<<endl;
                        delete pat; // since it didn't work
                    }
                } else {
                    grouped_pats.push_back(pat);
                }
            } else if (pos > 0 && lc_key == "startup" && fb.isStartup()) {
                if (!handleStartupItem(line, pos)) {
                    cerr<<"Error reading apps file at line "<<row<<"."<<endl;
                }
                // save the item even if it was bad (aren't we nice)
                m_startups.push_back(line.substr(pos));
            } else if (pos > 0 && lc_key == "group") {
                in_group = true;
                if (line.find('(') != string::npos)
                    pat = new ClientPattern(line.c_str() + pos);
            } else if (in_group) {
                // otherwise assume that it is the start of the attributes
                Application *app = 0;
                // search for a matching app
                list<ClientPattern *>::iterator it = grouped_pats.begin();
                list<ClientPattern *>::iterator it_end = grouped_pats.end();
                for (; !app && it != it_end; ++it) {
                    app = findMatchingPatterns(*it, old_pats, false, in_group, pat);
                }

                if (!app)
                    app = new Application(false, in_group, pat);
                else
                    reused_apps.insert(app);

                while (!grouped_pats.empty()) {
                    // associate all the patterns with this app
                    m_pats->push_back(make_pair(grouped_pats.front(), app));
                    grouped_pats.pop_front();
                }

                // we hit end... probably don't have attribs for the group
                // so finish it off with an empty application
                // otherwise parse the app
                if (!(pos>0 && lc_key == "end")) {
                    row += parseApp(apps_file, *app, &line);
                }
                in_group = false;
            } else
                cerr<<"Error in apps file on line "<<row<<"."<<endl;
        }
    }

    // Clean up old state
    // can't just delete old patterns list. Need to delete the
    // patterns themselves, plus the applications!

    Patterns::iterator it;
    set<Application *> old_apps; // no duplicates
    while (!old_pats->empty()) {
        it = old_pats->begin();
        delete it->first; // ClientPattern
        if (reused_apps.find(it->second) == reused_apps.end())
            old_apps.insert(it->second); // Application, not necessarily unique
        old_pats->erase(it);
    }

    // now remove any client entries for the old apps
    Clients::iterator cit = m_clients.begin();
    Clients::iterator cit_end = m_clients.end();
    while (cit != cit_end) {
        if (old_apps.find(cit->second) != old_apps.end()) {
            Clients::iterator tmpit = cit;
            ++cit;
            m_clients.erase(tmpit);
        } else {
            ++cit;
        }
    }

    set<Application *>::iterator ait = old_apps.begin(); // no duplicates
    for (; ait != old_apps.end(); ++ait) {
        delete (*ait);
    }

    delete old_pats;
}

void Remember::save() {

    string apps_string = FbTk::StringUtil::expandFilename(Fluxbox::instance()->getAppsFilename());

    fbdbg<<"("<<__FUNCTION__<<"): Saving apps file ["<<apps_string<<"]"<<endl;

    ofstream apps_file(apps_string.c_str());

    // first of all we output all the startup commands
    Startups::iterator sit = m_startups.begin();
    Startups::iterator sit_end = m_startups.end();
    for (; sit != sit_end; ++sit) {
        apps_file<<"[startup] "<<(*sit)<<endl;
    }

    Patterns::iterator it = m_pats->begin();
    Patterns::iterator it_end = m_pats->end();

    set<Application *> grouped_apps; // no duplicates

    for (; it != it_end; ++it) {
        Application &a = *it->second;
        if (a.is_grouped) {
            // if already processed
            if (grouped_apps.find(&a) != grouped_apps.end())
                continue;
            grouped_apps.insert(&a);
            // otherwise output this whole group
            apps_file << "[group]";
            if (a.group_pattern)
                apps_file << " " << a.group_pattern->toString();
            apps_file << endl;

            Patterns::iterator git = m_pats->begin();
            Patterns::iterator git_end = m_pats->end();
            for (; git != git_end; ++git) {
                if (git->second == &a) {
                    apps_file << (a.is_transient ? " [transient]" : " [app]") <<
                                 git->first->toString()<<endl;
                }
            }
        } else {
            apps_file << (a.is_transient ? "[transient]" : "[app]") <<
                         it->first->toString()<<endl;
        }
        if (a.workspace_remember) {
            apps_file << "  [Workspace]\t{" << a.workspace << "}" << endl;
        }
        if (a.head_remember) {
            apps_file << "  [Head]\t{" << a.head << "}" << endl;
        }
        if (a.dimensions_remember) {
            apps_file << "  [Dimensions]\t{" <<
                a.w << (a.dimension_is_w_relative ? "% " : " ") <<
                a.h << (a.dimension_is_h_relative ? "%}" : "}") << endl;
        }
        if (a.position_remember) {
            apps_file << "  [Position]\t(";
            switch(a.refc) {
            case FluxboxWindow::CENTER:
                apps_file << "CENTER";
                break;
            case FluxboxWindow::LEFTBOTTOM:
                apps_file << "LOWERLEFT";
                break;
            case FluxboxWindow::RIGHTBOTTOM:
                apps_file << "LOWERRIGHT";
                break;
            case FluxboxWindow::RIGHTTOP:
                apps_file << "UPPERRIGHT";
                break;
            case FluxboxWindow::LEFT:
                apps_file << "LEFT";
                break;
            case FluxboxWindow::RIGHT:
                apps_file << "RIGHT";
                break;
            case FluxboxWindow::TOP:
                apps_file << "TOP";
                break;
            case FluxboxWindow::BOTTOM:
                apps_file << "BOTTOM";
                break;
            default:
                apps_file << "UPPERLEFT";
            }
            apps_file << ")\t{" <<
                a.x << (a.position_is_x_relative ? "% " : " ") <<
                a.y << (a.position_is_y_relative ? "%}" : "}") << endl;
        }
        if (a.shadedstate_remember) {
            apps_file << "  [Shaded]\t{" << ((a.shadedstate)?"yes":"no") << "}" << endl;
        }
        if (a.tabstate_remember) {
            apps_file << "  [Tab]\t\t{" << ((a.tabstate)?"yes":"no") << "}" << endl;
        }
        if (a.decostate_remember) {
            switch (a.decostate) {
            case (0) :
                apps_file << "  [Deco]\t{NONE}" << endl;
                break;
            case (0xffffffff):
            case (WindowState::DECOR_NORMAL):
                apps_file << "  [Deco]\t{NORMAL}" << endl;
                break;
            case (WindowState::DECOR_TOOL):
                apps_file << "  [Deco]\t{TOOL}" << endl;
                break;
            case (WindowState::DECOR_TINY):
                apps_file << "  [Deco]\t{TINY}" << endl;
                break;
            case (WindowState::DECOR_BORDER):
                apps_file << "  [Deco]\t{BORDER}" << endl;
                break;
            case (WindowState::DECOR_TAB):
                apps_file << "  [Deco]\t{TAB}" << endl;
                break;
            default:
                apps_file << "  [Deco]\t{0x"<<hex<<a.decostate<<dec<<"}"<<endl;
                break;
            }
        }

        if (a.focushiddenstate_remember || a.iconhiddenstate_remember) {
            if (a.focushiddenstate_remember && a.iconhiddenstate_remember &&
                a.focushiddenstate == a.iconhiddenstate)
                apps_file << "  [Hidden]\t{" << ((a.focushiddenstate)?"yes":"no") << "}" << endl;
            else if (a.focushiddenstate_remember) {
                apps_file << "  [FocusHidden]\t{" << ((a.focushiddenstate)?"yes":"no") << "}" << endl;
            } else if (a.iconhiddenstate_remember) {
                apps_file << "  [IconHidden]\t{" << ((a.iconhiddenstate)?"yes":"no") << "}" << endl;
            }
        }
        if (a.stuckstate_remember) {
            apps_file << "  [Sticky]\t{" << ((a.stuckstate)?"yes":"no") << "}" << endl;
        }
        if (a.focusprotection_remember) {
            apps_file << "  [FocusProtection]\t{";
            if (a.focusprotection == Focus::NoProtection) {
                apps_file << "none";
            } else {
                bool b = false;
                if (a.focusprotection & Focus::Gain) {
                    apps_file << (b?",":"") << "gain";
                    b = true;
                }
                if (a.focusprotection & Focus::Refuse) {
                    apps_file << (b?",":"") << "refuse";
                    b = true;
                }
                if (a.focusprotection & Focus::Lock) {
                    apps_file << (b?",":"") << "lock";
                    b = true;
                }
                if (a.focusprotection & Focus::Deny) {
                    apps_file << (b?",":"") << "deny";
                    b = true;
                }
            }
            apps_file << "}" << endl;
        }
        if (a.minimizedstate_remember) {
            apps_file << "  [Minimized]\t{" << ((a.minimizedstate)?"yes":"no") << "}" << endl;
        }
        if (a.maximizedstate_remember) {
            apps_file << "  [Maximized]\t{";
            switch (a.maximizedstate) {
            case WindowState::MAX_FULL:
                apps_file << "yes" << "}" << endl;
                break;
            case WindowState::MAX_HORZ:
                apps_file << "horz" << "}" << endl;
                break;
            case WindowState::MAX_VERT:
                apps_file << "vert" << "}" << endl;
                break;
            case WindowState::MAX_NONE:
            default:
                apps_file << "no" << "}" << endl;
                break;
            }
        }
        if (a.fullscreenstate_remember) {
            apps_file << "  [Fullscreen]\t{" << ((a.fullscreenstate)?"yes":"no") << "}" << endl;
        }
        if (a.jumpworkspace_remember) {
            apps_file << "  [Jump]\t{" << ((a.jumpworkspace)?"yes":"no") << "}" << endl;
        }
        if (a.layer_remember) {
            apps_file << "  [Layer]\t{" << a.layer << "}" << endl;
        }
        if (a.save_on_close_remember) {
            apps_file << "  [Close]\t{" << ((a.save_on_close)?"yes":"no") << "}" << endl;
        }
        if (a.alpha_remember) {
            if (a.focused_alpha == a.unfocused_alpha)
                apps_file << "  [Alpha]\t{" << a.focused_alpha << "}" << endl;
            else
                apps_file << "  [Alpha]\t{" << a.focused_alpha << " " << a.unfocused_alpha << "}" << endl;
        }
        apps_file << "[end]" << endl;
    }
    apps_file.close();
    // update timestamp to avoid unnecessary reload
    m_reloader->addFile(Fluxbox::instance()->getAppsFilename());
}

bool Remember::isRemembered(WinClient &winclient, Attribute attrib) {
    Application *app = find(winclient);
    if (!app) return false;
    switch (attrib) {
    case REM_WORKSPACE:
        return app->workspace_remember;
        break;
    case REM_HEAD:
        return app->head_remember;
        break;
    case REM_DIMENSIONS:
        return app->dimensions_remember;
        break;
    case REM_IGNORE_SIZEHINTS:
        return app->ignoreSizeHints_remember;
        break;
    case REM_POSITION:
        return app->position_remember;
        break;
    case REM_FOCUSHIDDENSTATE:
        return app->focushiddenstate_remember;
        break;
    case REM_ICONHIDDENSTATE:
        return app->iconhiddenstate_remember;
        break;
    case REM_STUCKSTATE:
        return app->stuckstate_remember;
        break;
    case REM_FOCUSPROTECTION:
        return app->focusprotection_remember;
        break;
    case REM_MINIMIZEDSTATE:
        return app->minimizedstate_remember;
        break;
    case REM_MAXIMIZEDSTATE:
        return app->maximizedstate_remember;
        break;
    case REM_FULLSCREENSTATE:
        return app->fullscreenstate_remember;
        break;
    case REM_DECOSTATE:
        return app->decostate_remember;
        break;
    case REM_SHADEDSTATE:
        return app->shadedstate_remember;
        break;
        //    case REM_TABSTATE:
        //        return app->tabstate_remember;
        //        break;
    case REM_JUMPWORKSPACE:
        return app->jumpworkspace_remember;
        break;
    case REM_LAYER:
        return app->layer_remember;
        break;
    case REM_SAVEONCLOSE:
        return app->save_on_close_remember;
        break;
    case REM_ALPHA:
        return app->alpha_remember;
    case REM_LASTATTRIB:
    default:
        return false; // should never get here
    }
}

void Remember::rememberAttrib(WinClient &winclient, Attribute attrib) {
    FluxboxWindow *win = winclient.fbwindow();
    if (!win) return;
    Application *app = find(winclient);
    if (!app) {
        app = add(winclient);
        if (!app) return;
    }
    int head, percx, percy;
    switch (attrib) {
    case REM_WORKSPACE:
        app->rememberWorkspace(win->workspaceNumber());
        break;
    case REM_HEAD:
        app->rememberHead(win->screen().getHead(win->fbWindow()));
        break;
    case REM_DIMENSIONS: {
        head = win->screen().getHead(win->fbWindow());
        percx = win->screen().calRelativeDimensionWidth(head, win->normalWidth());
        percy = win->screen().calRelativeDimensionHeight(head, win->normalHeight());
        app->rememberDimensions(percx, percy, true, true);
        break;
    }
    case REM_POSITION: {
        head = win->screen().getHead(win->fbWindow());
        percx = win->screen().calRelativePositionWidth(head, win->normalX());
        percy = win->screen().calRelativePositionHeight(head, win->normalY());
        app->rememberPosition(percx, percy, true, true);
        break;
    }
    case REM_FOCUSHIDDENSTATE:
        app->rememberFocusHiddenstate(win->isFocusHidden());
        break;
    case REM_ICONHIDDENSTATE:
        app->rememberIconHiddenstate(win->isIconHidden());
        break;
    case REM_SHADEDSTATE:
        app->rememberShadedstate(win->isShaded());
        break;
    case REM_DECOSTATE:
        app->rememberDecostate(win->decorationMask());
        break;
    case REM_STUCKSTATE:
        app->rememberStuckstate(win->isStuck());
        break;
    case REM_FOCUSPROTECTION:
        app->rememberFocusProtection(win->focusProtection());
        break;
    case REM_MINIMIZEDSTATE:
        app->rememberMinimizedstate(win->isIconic());
        break;
    case REM_MAXIMIZEDSTATE:
        app->rememberMaximizedstate(win->maximizedState());
        break;
    case REM_FULLSCREENSTATE:
        app->rememberFullscreenstate(win->isFullscreen());
        break;
    case REM_ALPHA:
        app->rememberAlpha(win->frame().getAlpha(true), win->frame().getAlpha(false));
        break;
        //    case REM_TABSTATE:
        //        break;
    case REM_JUMPWORKSPACE:
        app->rememberJumpworkspace(true);
        break;
    case REM_LAYER:
        app->rememberLayer(win->layerNum());
        break;
    case REM_SAVEONCLOSE:
        app->rememberSaveOnClose(true);
        break;
    case REM_LASTATTRIB:
    default:
        // nothing
        break;
    }
}

void Remember::forgetAttrib(WinClient &winclient, Attribute attrib) {
    FluxboxWindow *win = winclient.fbwindow();
    if (!win) return;
    Application *app = find(winclient);
    if (!app) {
        app = add(winclient);
        if (!app) return;
    }
    switch (attrib) {
    case REM_WORKSPACE:
        app->forgetWorkspace();
        break;
    case REM_HEAD:
        app->forgetHead();
        break;
    case REM_DIMENSIONS:
        app->forgetDimensions();
        break;
    case REM_IGNORE_SIZEHINTS:
        app->ignoreSizeHints_remember = false;
        break;
    case REM_POSITION:
        app->forgetPosition();
        break;
    case REM_FOCUSHIDDENSTATE:
        app->forgetFocusHiddenstate();
        break;
    case REM_ICONHIDDENSTATE:
        app->forgetIconHiddenstate();
        break;
    case REM_STUCKSTATE:
        app->forgetStuckstate();
        break;
    case REM_FOCUSPROTECTION:
        app->forgetFocusProtection();
        break;
    case REM_MINIMIZEDSTATE:
        app->forgetMinimizedstate();
        break;
    case REM_MAXIMIZEDSTATE:
        app->forgetMaximizedstate();
        break;
    case REM_FULLSCREENSTATE:
        app->forgetFullscreenstate();
        break;
    case REM_DECOSTATE:
        app->forgetDecostate();
        break;
    case REM_SHADEDSTATE:
        app->forgetShadedstate();
        break;
    case REM_ALPHA:
        app->forgetAlpha();
        break;
//    case REM_TABSTATE:
//        break;
    case REM_JUMPWORKSPACE:
        app->forgetJumpworkspace();
        break;
    case REM_LAYER:
        app->forgetLayer();
        break;
    case REM_SAVEONCLOSE:
        app->forgetSaveOnClose();
        break;
    case REM_LASTATTRIB:
    default:
        // nothing
        break;
    }
}

void Remember::setupFrame(FluxboxWindow &win) {
    WinClient &winclient = win.winClient();
    Application *app = find(winclient);
    if (app == 0)
        return; // nothing to do

    // first, set the options that aren't preserved as window properties on
    // restart, then return if fluxbox is restarting -- we want restart to
    // disturb the current window state as little as possible

    if (app->focushiddenstate_remember)
        win.setFocusHidden(app->focushiddenstate);
    if (app->iconhiddenstate_remember)
        win.setIconHidden(app->iconhiddenstate);
    if (app->layer_remember)
        win.moveToLayer(app->layer);
    if (app->decostate_remember)
        win.setDecorationMask(app->decostate);

    if (app->alpha_remember) {
        win.frame().setDefaultAlpha();
        win.frame().setAlpha(true,app->focused_alpha);
        win.frame().setAlpha(false,app->unfocused_alpha);
    }

    BScreen &screen = winclient.screen();

    // now check if fluxbox is restarting
    if (screen.isRestart())
        return;

    if (app->workspace_remember) {
        // we use setWorkspace and not reassoc because we're still initialising
        win.setWorkspace(app->workspace);
        if (app->jumpworkspace_remember && app->jumpworkspace)
            screen.changeWorkspaceID(app->workspace);
    }

    if (app->head_remember) {
        win.setOnHead(app->head);
    }

    if (app->dimensions_remember) {

        int win_w = app->w;
        int win_h = app->h;
        int head = screen.getHead(win.fbWindow());
        int border_w = win.frame().window().borderWidth();

        if (app->dimension_is_w_relative)
            win_w = screen.calRelativeWidth(head, win_w);
        if (app->dimension_is_h_relative)
            win_h = screen.calRelativeHeight(head, win_h);

        win.resize(win_w - 2 * border_w, win_h - 2 * border_w);
    }

    if (app->position_remember) {

        int newx = app->x;
        int newy = app->y;
        int head = screen.getHead(win.fbWindow());

        if (app->position_is_x_relative)
            newx = screen.calRelativeWidth(head, newx);
        if (app->position_is_y_relative)
            newy = screen.calRelativeHeight(head, newy);

        win.translateCoords(newx, newy, app->refc);
        win.move(newx, newy);
    }

    if (app->shadedstate_remember)
        // if inconsistent...
        if ((win.isShaded() && !app->shadedstate) ||
            (!win.isShaded() && app->shadedstate))
            win.shade(); // toggles

    // external tabs aren't available atm...
    //if (app->tabstate_remember) ...

    if (app->stuckstate_remember)
        // if inconsistent...
        if ((win.isStuck() && !app->stuckstate) ||
            (!win.isStuck() && app->stuckstate))
            win.stick(); // toggles

    if (app->focusprotection_remember) {
        win.setFocusProtection(app->focusprotection);
    }

    if (app->minimizedstate_remember) {
        // if inconsistent...
        // this one doesn't actually work, but I can't imagine needing it
        if (win.isIconic() && !app->minimizedstate)
            win.deiconify();
        else if (!win.isIconic() && app->minimizedstate)
            win.iconify();
    }

    // I can't really test the "no" case of this
    if (app->maximizedstate_remember)
        win.setMaximizedState(app->maximizedstate);

    // I can't really test the "no" case of this
    if (app->fullscreenstate_remember)
        win.setFullscreen(app->fullscreenstate);
}

void Remember::setupClient(WinClient &winclient) {

    // leave windows alone on restart
    if (winclient.screen().isRestart())
        return;

    // check if apps file has changed
    checkReload();

    Application *app = find(winclient);
    if (app == 0)
        return; // nothing to do

    FluxboxWindow *group;
    if (winclient.fbwindow() == 0 && app->is_grouped &&
        (group = findGroup(app, winclient.screen()))) {
        group->attachClient(winclient);
        if (app->jumpworkspace_remember && app->jumpworkspace)
            // jump to window, not saved workspace
            winclient.screen().changeWorkspaceID(group->workspaceNumber());
    }
}

FluxboxWindow *Remember::findGroup(Application *app, BScreen &screen) {
    if (!app || !app->is_grouped)
        return 0;

    // find the first client associated with the app and return its fbwindow
    Clients::iterator it = m_clients.begin();
    Clients::iterator it_end = m_clients.end();
    for (; it != it_end; ++it) {
        if (it->second == app && it->first->fbwindow() &&
            &screen == &it->first->screen() &&
            (!app->group_pattern || app->group_pattern->match(*it->first)))
            return it->first->fbwindow();
    }

    // there weren't any open, but that's ok
    return 0;
}

void Remember::updateDecoStateFromClient(WinClient& winclient) {

    Application* app= find(winclient);

    if ( app && isRemembered(winclient, REM_DECOSTATE)) {
        winclient.fbwindow()->setDecorationMask(app->decostate);
    }
}

void Remember::updateClientClose(WinClient &winclient) {
    checkReload(); // reload if it's changed
    Application *app = find(winclient);

    if (app) {
        Patterns::iterator it = m_pats->begin();
        for (; it != m_pats->end(); ++it) {
            if (it->second == app) {
                it->first->removeMatch();
                break;
            }
        }
    }

    if (app && (app->save_on_close_remember && app->save_on_close)) {

        for (int attrib = 0; attrib < REM_LASTATTRIB; attrib++) {
            if (isRemembered(winclient, (Attribute) attrib)) {
                rememberAttrib(winclient, (Attribute) attrib);
            }
        }

        save();
    }

    // we need to get rid of references to this client
    Clients::iterator wc_it = m_clients.find(&winclient);

    if (wc_it != m_clients.end()) {
        m_clients.erase(wc_it);
    }

}

void Remember::initForScreen(BScreen &screen) { }

FbTk::Menu* Remember::createMenu(BScreen& screen) {

    return createRememberMenu(screen);
}

