// AtomHandler for fluxbox
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@fluxbox.org)
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

// $Id: AtomHandler.hh,v 1.3 2002/10/16 19:01:41 fluxgen Exp $

#ifndef ATOMHANDLER_HH
#define ATOMHANDLER_HH

#include <X11/Xlib.h>

class FluxboxWindow;
class BScreen;

class AtomHandler {
public:
	virtual ~AtomHandler() { }
	
	virtual void initForScreen(const BScreen &screen) = 0;
	virtual void setupWindow(FluxboxWindow &win) = 0;
	
	virtual void updateClientList(const BScreen &screen) = 0;
	virtual void updateWorkspaceNames(const BScreen &screen) = 0;
	virtual void updateCurrentWorkspace(const BScreen &screen) = 0;
	virtual void updateWorkspaceCount(const BScreen &screen) = 0;
	
	virtual void updateWorkspace(FluxboxWindow &win) = 0;
	virtual void updateState(FluxboxWindow &win) = 0;
	virtual void updateHints(FluxboxWindow &win) = 0;
	
	virtual bool checkClientMessage(const XClientMessageEvent &ce, 
		BScreen * const screen, FluxboxWindow * const win) = 0;
	
	/// should this object be updated or not?
	bool update() const { return m_update; }
protected:
	void disableUpdate() { m_update = false; }
	void enableUpdate() { m_update = true; }	
private:
	bool m_update; ///< do we get update or not
};

#endif // ATOMHANDLER_HH
