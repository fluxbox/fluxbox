#include "MenuSearch.hh"
#include "MenuItem.hh"
#include "StringUtil.hh"
#include "Resource.hh"

namespace {

size_t search_str_nowhere(const std::string& text, const std::string& pattern) {
    return std::string::npos;
}

// finds 'pattern' at beginning of 'text'
size_t search_str_textstart(const std::string& text, const std::string& pattern) {

    size_t l = std::min(text.size(), pattern.size());
    if (l == 0) {
        return std::string::npos;
    }

    size_t i;
    for (i = l; i > 0; i--) {
        if (std::tolower(text[i-1]) != pattern[i-1]) {
            return std::string::npos;
        }
    }

    return i;
}

// finds 'pattern' in 'text', case insensitive.
// returns position or std::string::npos if not found.
//
// implements Boyer–Moore–Horspool
size_t search_str_bmh(const std::string& text, const std::string& pattern) {

    if (pattern.empty()) {
        return 0;
    }
    if (text.empty() || pattern.size() > text.size()) {
        return std::string::npos;
    }

    const size_t tlen = text.size();
    const size_t plen = pattern.size();
    size_t t; // index in text

    // simple case, no need to be too clever
    if (plen == 1) {
        int b = pattern[0];
        for (t = 0; t < tlen; t++) {
            if (b == std::tolower(text[t])) {
                return t;
            }
        }
        return std::string::npos;
    }

    // prepare skip-table
    //
    size_t       skip[256];
    const size_t pe = plen - 1; // end index in pattern
    size_t       p;             // index in pattern

    for (p = 0; p < sizeof(skip)/sizeof(skip[0]); p++) {
        skip[p] = plen;
    }
    for (p = 0; p < pe; p++) {
        skip[pattern[p]] = pe - p;
    }

    // match
    //
    for (t = 0; (t+pe) < tlen; ) {
        for (p = pe; std::tolower(text[t+p]) == pattern[p]; p--) {
            if (p == 0) {
                return t;
            }
        }
        t += skip[std::tolower(text[t+pe])];
    }

    return std::string::npos;
}


// actually search function, depends on Mode
size_t (*search_str)(const std::string&, const std::string&) = search_str_textstart;

} // anonymous



namespace FbTk {


void MenuSearch::setMode(MenuSearch::Mode m) {
    if (m == NOWHERE) {
        search_str = search_str_nowhere;
    } else if (m == SOMEWHERE) {
        search_str = search_str_bmh;
    } else {
        search_str = search_str_textstart;
    }
}



MenuSearch::MenuSearch(const std::vector<FbTk::MenuItem*>& items) : 
    m_items(items) {
}

size_t MenuSearch::size() const { 
    return pattern.size();
}

void MenuSearch::clear() { 
    pattern.clear();
}

void MenuSearch::add(char c) {
    pattern.push_back(std::tolower(c));
}

void MenuSearch::backspace() {
    size_t s = pattern.size();
    if (s > 0) {
        pattern.erase(s - 1, 1);
    }
}

// is 'pattern' matching something?
bool MenuSearch::has_match() {
    size_t l = m_items.size();
    size_t i;
    for (i = 0; i < l; i++) {
        if (!m_items[i]->isEnabled())
            continue;
        if (search_str(m_items[i]->iTypeString(), pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// would 'the_pattern' match something?
bool MenuSearch::would_match(const std::string& the_pattern) {
    size_t l = m_items.size();
    size_t i;
    for (i = 0; i < l; i++) {
        if (!m_items[i]->isEnabled())
            continue;
        if (search_str(m_items[i]->iTypeString(), the_pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

size_t MenuSearch::num_matches() {
    size_t l = m_items.size();
    size_t i, n;
    for (i = 0, n = 0; i < l; i++) {
        if (!m_items[i]->isEnabled())
            continue;
        if (search_str(m_items[i]->iTypeString(), pattern) != std::string::npos) {
            n++;
        }
    }
    return n;
}


// returns true if m_text matches against m_items[i] and stores
// the position where it matches in the string. an empty
// 'pattern' always matches
bool MenuSearch::get_match(size_t i, size_t& idx) {
    if (i > m_items.size()) {
        return false;
    }

    if (pattern.empty())
        return true;

    idx = search_str(m_items[i]->iTypeString(), pattern);
    return idx != std::string::npos;
}




//
// resource-implementation related

template<>
std::string FbTk::Resource<FbTk::MenuSearch::Mode>::getString() const {

    switch (m_value) {
    case FbTk::MenuSearch::NOWHERE:
        return "nowhere";
    case FbTk::MenuSearch::SOMEWHERE:
        return "somewhere";
    default:
        return "itemstart";
    };
}

template<>
void FbTk::Resource<FbTk::MenuSearch::Mode>::setFromString(const char *strval) {

    std::string val = FbTk::StringUtil::toLower(strval);
    if (val == "nowhere") {
        m_value = FbTk::MenuSearch::NOWHERE;
    } else if (val == "somewhere") {
        m_value = FbTk::MenuSearch::SOMEWHERE;
    } else {
        setDefaultValue();
    }
}

}
