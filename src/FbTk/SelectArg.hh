// SelectArg.hh for FbTk
// Copyright (c) 2010 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef FBTK_STLUTIL_SELECT_ARG_HH
#define FBTK_STLUTIL_SELECT_ARG_HH

#include "STLUtil.hh"

namespace FbTk {

namespace STLUtil {

/**
 * Selects a single argument from a maximum set of three arguments at compile time.
 * For example:
 * \begincode
 * SelectArg<0>()( 10, 20, "hello" ); // returns first argument ( 10 )
 * SelectArg<1>()( 10, "hello", 30 ); // returns second argument ( "hello" )
 * SelectArg<2>()( 10, "hello", 30 ); // returns third argument ( 30 )
 * \endcode
 *
 * The selection of argument 1 and 2 can return the last argument if the
 * arguments are less or equal to two. For instance:
 * \begincode
 * SelectArg<1>()(10); // returns 10
 * SelectArg<2>()(10, 20); // returns 20
 * SelectArg<2>()(10); // returns 10
 * \endcode
 */
template < int N >
struct SelectArg {

    template <typename Type1, typename Type2>
    typename IfThenElse<N==0, Type1, Type2>::ResultType& operator ()(Type1& a, Type2& b){
        return IfThenElse<N==0, Type1, Type2>()(a, b);
    }

    template <typename Type1, typename Type2, typename Type3>
    typename IfThenElse<N==0, Type1,
                        typename IfThenElse<N==1, Type2, Type3>::ResultType>::ResultType&
     operator () (Type1& a, Type2& b, Type3& c) {
        return IfThenElse<N==0, Type1,
            typename IfThenElse<N==1, Type2, Type3>::ResultType>()
            (a, IfThenElse<N==1, Type2, Type3>() (b, c) );
    }

    template < typename Type1 >
    Type1 operator() (Type1 a) {
        return a;
    }
};

} // end namespace STLUtil
} // end namespace FbTk

#endif // FBTK_STLUTIL_SELECT_ARG_HH
