// Button.hh for FbTk - Fluxbox Toolkit
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Button.hh,v 1.8 2003/12/16 17:06:49 fluxgen Exp $

#ifndef FBTK_BUTTON_HH
#define FBTK_BUTTON_HH

#include "EventHandler.hh"
#include "NotCopyable.hh"
#include "RefCount.hh"
#include "FbWindow.hh"
#include "Command.hh"
#include "Color.hh"

#include <X11/Xlib.h>
#include <memory>

namespace FbTk {

class Button:public FbTk::FbWindow, public EventHandler, 
             private NotCopyable {
public:
    Button(int screen_num, int x, int y, unsigned int width, unsigned int height);
    Button(const FbWindow &parent, int x, int y, unsigned int width, unsigned int height);
    virtual ~Button();
	
    /// sets action when the button is clicked with #button mouse btn
    void setOnClick(RefCount<Command> &com, int button = 1);

    /// sets foreground pixmap 
    void setPixmap(Pixmap pm);
    /// sets the pixmap to be viewed when the button is pressed
    void setPressedPixmap(Pixmap pm);
    void setPressedColor(const FbTk::Color &color);
    /// sets graphic context for drawing
    void setGC(GC gc) { m_gc = gc; }
    /// sets background pixmap, this will override background color
    virtual void setBackgroundPixmap(Pixmap pm);
    /// sets background color
    virtual void setBackgroundColor(const Color &color);

    /**
       @name eventhandlers
     */
    //@{
    virtual void buttonPressEvent(XButtonEvent &event);
    virtual void buttonReleaseEvent(XButtonEvent &event);
    virtual void exposeEvent(XExposeEvent &event);
    //@}

    /// @return true if the button is pressed, else false
    bool pressed() const { return m_pressed; }

    GC gc() const { return m_gc; }
    Pixmap backgroundPixmap() const { return m_background_pm; }
    const Color &backgroundColor() const { return m_background_color; }
    Pixmap foregroundPixmap() const { return m_foreground_pm; }
private:
    Pixmap m_foreground_pm; ///< foreground pixmap
    Pixmap m_background_pm; ///< background pixmap
    Color m_background_color; ///< background color
    Pixmap m_pressed_pm; ///< pressed pixmap
    Color m_pressed_color;
    GC m_gc; ///< graphic context for button
    bool m_pressed; ///< if the button is pressed
    RefCount<Command> m_onclick[5]; ///< what to do when this button is clicked with button num
};

} // namespace FbTk

#endif // FBTK_BUTTON_HH
