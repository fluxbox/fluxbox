// TypeAhead.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef FBTK_TYPEAHEAD_HH
#define FBTK_TYPEAHEAD_HH

#include "ITypeAheadable.hh"
#include "SearchResult.hh"

namespace FbTk {

template <typename Items, typename Item_Type>
class TypeAhead {
#if 0

// a class template can't be split into separate interface + implementation files, an interface summary is given here:

public:
    void init(Items const &items);

// accessors:
    int stringSize() const { return m_searchstr.size(); }
    Items matched() const;

// modifiers:
    Items putCharacter(char ch);
    void putBackSpace();
    void reset()

private:
    SearchResults m_search_results;
    std::string m_searchstr;
    Items const *m_ref;

// helper
    void fillValues(BaseItems const &search, ValueVec &fillin) const;

// reverts to searchstate before current
    void revert();

// search performs iteration and sets state
    void search(char char_to_test);
    void doSearch(char to_test,
            Items const &items,
            SearchResult &mySearchResult) const;
    void doSearch(char to_test,
            BaseItems const &search,
            SearchResult &mySearchResult) const;
#endif

public:
    typedef std::vector < ITypeAheadable* > BaseItems;
    typedef BaseItems::const_iterator BaseItemscIt;
    typedef std::vector < SearchResult > SearchResults;
    typedef typename Items::const_iterator ItemscIt;

    void init(Items const &items) { m_ref = &items; }

    size_t stringSize() const { return m_searchstr.size(); }

    void seek() {
        if (!m_search_results.empty())
            m_searchstr = m_search_results.back().seekedString();
    }

    Items putCharacter(char ch) {
        if (isprint(ch))
            search(ch);
        return matched();
    }

    void putBackSpace() {
        if (!m_search_results.empty())
            revert();
    }

    void reset() {
        m_searchstr.clear();
        m_search_results.clear();
    }

    Items matched() const {
        Items last_matched;

        if (!m_search_results.empty())
            fillValues(m_search_results.back().result(), last_matched);
        else
            return *m_ref;
        return last_matched;
    }

private:
    SearchResults m_search_results;
    std::string m_searchstr;
    Items const *m_ref; // reference to vector we are operating on

    void fillValues(BaseItems const &search, Items &fillin) const {
        for (BaseItemscIt it = search.begin(); it != search.end(); it++) {
            Item_Type tmp = dynamic_cast<Item_Type>(*it);
            if (tmp)
                fillin.push_back(tmp);
        }
    }

    void revert() {
        m_search_results.pop_back();
        if (m_search_results.empty())
            m_searchstr.clear();
        else
            m_searchstr = m_search_results.back().seekedString();
    }

    void search(char char_to_test) {
        SearchResult mySearchResult(m_searchstr + char_to_test);
        size_t num_items = m_ref->size();

        // check if we have already a searched set
        if (m_search_results.empty())
            doSearch(char_to_test, *m_ref, mySearchResult);
        else {
            num_items = m_search_results.back().size();
            doSearch(char_to_test, m_search_results.back().result(),
                     mySearchResult);
        }

        if (mySearchResult.size() > 0 ) {
            if (mySearchResult.size() < num_items) {
                mySearchResult.seek();
                m_search_results.push_back(mySearchResult);
            }
            m_searchstr += char_to_test;
        }
    }

    // iteration based on original list of items
    void doSearch(char to_test, Items const &items,
                  SearchResult &mySearchResult) const {
        for (ItemscIt it = items.begin(); it != items.end(); it++) {
            if ((*it)->iTypeCompareChar(to_test, stringSize()) && (*it)->isEnabled())
                mySearchResult.add(*it);
        }
    }

    // iteration based on last SearchResult
    void doSearch(char to_test, BaseItems const &search,
                  SearchResult &mySearchResult) const {
        for (BaseItemscIt it = search.begin(); it != search.end(); it++) {
            if ((*it)->iTypeCompareChar(to_test, stringSize()) && (*it)->isEnabled())
                mySearchResult.add(*it);
        }
    }

}; // end Class TypeAhead

} // end namespace FbTk

#endif // FBTK_TYPEAHEAD_HH
