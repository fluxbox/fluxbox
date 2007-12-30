// SearchResult.hh for FbTk - Fluxbox Toolkit
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

#ifndef FBTK_SEARCHRESULT_HH
#define FBTK_SEARCHRESULT_HH

#include <vector>
#include <string>

namespace FbTk {

class ITypeAheadable;

class SearchResult {
public:
    typedef std::vector < ITypeAheadable* > BaseItems;
    typedef BaseItems::iterator BaseItemsIt;

    SearchResult(const std::string &to_search_for):
        m_seeked_string(to_search_for) { }

    void add(ITypeAheadable* item) { m_results.push_back(item); }
    size_t size() const { return m_results.size(); }
    const BaseItems& result() const { return m_results; }
    const std::string& seekedString() const { return m_seeked_string; }

    void seek();

private:
    BaseItems m_results;
    std::string m_seeked_string;

};

} // end namespace FbTk

#endif // FBTK_SEARCHRESULT_HH
