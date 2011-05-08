// fluxbox-remote.cc
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool g_gotError;
static int HandleIPCError(Display *disp, XErrorEvent*ptr)
{
	// ptr->error_code contains the actual error flags
	g_gotError=true;
	return( 0 );
}

int main(int argc, char **argv) {

    if (argc <= 1) {
        printf("fluxbox-remote <fluxbox-command>\n");
        return EXIT_SUCCESS;
    }

    Display *disp = XOpenDisplay(NULL);
    if (!disp) {
        perror("error, can't open display.");
        return EXIT_FAILURE;
    }

    Atom fbcmd_atom = XInternAtom(disp, "_FLUXBOX_ACTION", False);
    Window root = DefaultRootWindow(disp);

    char *str = argv[1];

    typedef int (*x_error_handler_t)(Display*,XErrorEvent*);

    // assign the custom handler, clear the flag, sync the data, then check it for success/failure
    x_error_handler_t handler = XSetErrorHandler( HandleIPCError );
    g_gotError=false;
    XChangeProperty(disp, root, fbcmd_atom,
                              XA_STRING, 8, PropModeReplace,
                              (unsigned char *) str, strlen(str));
    XSync(disp,False);
    int ret=(g_gotError?EXIT_FAILURE:EXIT_SUCCESS);
    XSetErrorHandler(handler);

    XCloseDisplay(disp);

    return ret;
}

