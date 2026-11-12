// RefCount.cc for FbTk - Fluxbox ToolKit
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// The pointer-type independent part of RefCount<T>; see RefCount.hh.

#include "RefCount.hh"

namespace FbTk {

namespace RefCountImpl {

unsigned int *newCounter() {
    return new unsigned int(1);
}

void retain(unsigned int *counter) {
    if (counter == 0)
        return;
    (*counter)++;
}

bool release(unsigned int *&counter) {
    if (counter == 0)
        return false;
    if (*counter == 0) { // already zero, then delete the counter only
        delete counter;
        counter = 0;
        return false;
    }
    (*counter)--;
    // nobody else is using the object: the caller deletes it, then calls
    // freeCounter() (same order as before this was moved out of the template)
    return *counter == 0;
}

void freeCounter(unsigned int *&counter) {
    delete counter;
    counter = 0;
}

} // namespace RefCountImpl

} // namespace FbTk
