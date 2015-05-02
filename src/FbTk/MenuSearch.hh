#ifndef _MENU_SEARCH_HH_
#define _MENU_SEARCH_HH_

#include <vector>
#include <string>
#include <cstddef>

namespace FbTk {

class MenuItem;


// a small helper which applies search operations on a list of MenuItems*.
// the former incarnation of this class was FbTk::TypeAhead in combination with
// the now non-existent FbTk::SearchResults, but the complexity of these
// are not needed for our use case. as a bonus, we have less lose parts
// flying around.
//
// MenuSearch is case insensitive.
class MenuSearch {
public:

    enum Mode {
        NOWHERE,
        ITEMSTART,
        SOMEWHERE,

        DEFAULT = ITEMSTART
    };

    static void setMode(Mode m);


    MenuSearch(const std::vector<FbTk::MenuItem*>& items);

    size_t size() const;
    void clear();
    void add(char c);
    void backspace();

    // is 'pattern' matching something?
    bool has_match();

    // would 'the_pattern' match something?
    bool would_match(const std::string& the_pattern);

    size_t num_matches();

    // returns true if m_text matches against m_items[i] and stores
    // the position where it matches in the string
    bool get_match(size_t i, size_t& idx);

    std::string pattern;
private:
    const std::vector<FbTk::MenuItem*>& m_items;
};

}


#endif
