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
#include <X11/Xutil.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


bool g_gotError = false;
static int HandleIPCError(Display *disp, XErrorEvent*ptr)
{
	// ptr->error_code contains the actual error flags
	g_gotError = true;
	return( 0 );
}

typedef int (*xerror_cb_t)(Display*,XErrorEvent*);


int main(int argc, char **argv) {

    int         rc;
    Display*    disp;
    Window      root;
    Atom        atom_fbcmd; 
    Atom        atom_result;
    xerror_cb_t error_cb;
    char*       cmd;

    if (argc <= 1) {
        printf("fluxbox-remote <fluxbox-command>\n");
        return EXIT_SUCCESS;
    }

    disp = XOpenDisplay(NULL);
    if (!disp) {
        perror("error, can't open display.");
        rc = EXIT_FAILURE;
        return rc;
    }

    cmd = argv[1];
    atom_fbcmd = XInternAtom(disp, "_FLUXBOX_ACTION", False);
    atom_result = XInternAtom(disp, "_FLUXBOX_ACTION_RESULT", False);
    root = DefaultRootWindow(disp);

    // assign the custom handler, clear the flag, sync the data,
    // then check it for success/failure
    error_cb = XSetErrorHandler(HandleIPCError);


    if (strcmp(cmd, "result") == 0) {
        XTextProperty text_prop;
        if (XGetTextProperty(disp, root, &text_prop, atom_result) != 0
            && text_prop.value != 0
            && text_prop.nitems > 0) {

            printf("%s", text_prop.value);
            XFree(text_prop.value);
        }
    } else {
        XChangeProperty(disp, root, atom_fbcmd,
                              XA_STRING, 8, PropModeReplace,
                              (unsigned char *)cmd, strlen(cmd));
        XSync(disp, false);
    }

    rc = (g_gotError ? EXIT_FAILURE : EXIT_SUCCESS);

    XSetErrorHandler(error_cb);
    XCloseDisplay(disp);

    return rc;
}

