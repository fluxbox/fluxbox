// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// Copyright (c) 1997 - 2000 Brad Hughes <bhughes at trolltech.com>
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

// $Id: bsetroot.hh,v 1.4 2002/08/18 09:47:03 fluxgen Exp $

#ifndef BSETROOT_HH
#define BSETROOT_HH

#include "../src/BaseDisplay.hh"

class BImageControl;

class bsetroot : public BaseDisplay {
public:
	bsetroot(int argc, char **argv, char * dpy_name= 0);
	~bsetroot();

	inline virtual bool handleSignal(int num) { return False; }

	void gradient();
	void modula(int, int);
	void solid();
	void usage(int = 0);
	void setRootAtoms(Pixmap pixmap, int screen);

protected:
	void handleEvent(XEvent *xe) { }

private:
	BImageControl **img_ctrl;
	Pixmap *pixmaps;

	char *fore, *back, *grad;
	Display *display;
	int num_screens;

};


#endif // BSETROOT_HH
