// TooltipWindow.hh
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
// DEALINGS IN THE SOFTWARE.#ifndef TOOLTIPWINDOW_HH_
#ifndef TOOLTIPWINDOW_HH_
#define TOOLTIPWINDOW_HH_

#include "OSDWindow.hh"
#include "FbTk/Command.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Timer.hh"
#include "FbTk/SimpleCommand.hh"


class TooltipWindow : public OSDWindow  {
public:
    TooltipWindow(const FbTk::FbWindow &parent, BScreen &screen,
                  FbTk::ThemeProxy<FbWinFrameTheme> &theme);

    void showText(const std::string &text);
    void setDelay(int iDelay) { 
        delay = iDelay; 
        timer.setTimeout(delay);
    }
    void hide() ;    

private:
    void raiseTooltip();
    void show();    
    int delay;
    std::string lastText;
    FbTk::Timer timer;
};



#endif /*TOOLTIPWINDOW_HH_*/
