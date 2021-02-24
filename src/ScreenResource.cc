// ScreenResource.cc for Fluxbox Window Manager
// Copyright (c) 2015 - Mathias Gumz <akira@fluxbox.org>
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

#include "ScreenResource.hh"
#include "fluxbox.hh"
#include "FbTk/Util.hh"
#include <cstring>

namespace {

struct TabPlacementString {
    FbWinFrame::TabPlacement placement;
    const char* str;
};

const TabPlacementString _PLACEMENT_STRINGS[] = {
    { FbWinFrame::TOPLEFT, "TopLeft" },
    { FbWinFrame::TOP, "Top" },
    { FbWinFrame::TOPRIGHT, "TopRight" },
    { FbWinFrame::BOTTOMLEFT, "BottomLeft" },
    { FbWinFrame::BOTTOM, "Bottom" },
    { FbWinFrame::BOTTOMRIGHT, "BottomRight" },
    { FbWinFrame::LEFTBOTTOM, "LeftBottom" },
    { FbWinFrame::LEFT, "Left" },
    { FbWinFrame::LEFTTOP, "LeftTop" },
    { FbWinFrame::RIGHTBOTTOM, "RightBottom" },
    { FbWinFrame::RIGHT, "Right" },
    { FbWinFrame::RIGHTTOP, "RightTop" }
};

}

namespace FbTk {

template<>
std::string FbTk::Resource<FbWinFrame::TabPlacement>::
getString() const {

    size_t i = (m_value == FbTk::Util::clamp(m_value, FbWinFrame::TOPLEFT, FbWinFrame::RIGHTTOP)
                ? m_value 
                : FbWinFrame::DEFAULT) - 1;
    return _PLACEMENT_STRINGS[i].str;
}

template<>
void FbTk::Resource<FbWinFrame::TabPlacement>::
setFromString(const char *strval) {

    size_t i;
    for (i = 0; i < sizeof(_PLACEMENT_STRINGS)/sizeof(_PLACEMENT_STRINGS[0]); ++i) {
        if (strcasecmp(strval, _PLACEMENT_STRINGS[i].str) == 0) {
            m_value = _PLACEMENT_STRINGS[i].placement;
            return;
        }
    }
    setDefaultValue();
}

} // end namespace FbTk




ScreenResource::ScreenResource(FbTk::ResourceManager& rm,
        const std::string& scrname,
        const std::string& altscrname):
    opaque_move(rm, true, scrname + ".opaqueMove", altscrname+".OpaqueMove"),
    opaque_resize(rm, false, scrname + ".opaqueResize", altscrname+".OpaqueResize"),
    full_max(rm, false, scrname+".fullMaximization", altscrname+".FullMaximization"),
    max_ignore_inc(rm, true, scrname+".maxIgnoreIncrement", altscrname+".MaxIgnoreIncrement"),
    max_disable_move(rm, false, scrname+".maxDisableMove", altscrname+".MaxDisableMove"),
    max_disable_resize(rm, false, scrname+".maxDisableResize", altscrname+".MaxDisableResize"),
    workspace_warping(rm, true, scrname+".workspacewarping", altscrname+".WorkspaceWarping"),
    workspace_warping_vertical(rm, true, scrname+".workspacewarpingvertical", altscrname+".WorkspaceWarpingVertical"),
    workspace_warping_vertical_offset(rm, 1, scrname+".workspacewarpingverticaloffset", altscrname+".WorkspaceWarpingVerticalOffset"),
    show_window_pos(rm, false, scrname+".showwindowposition", altscrname+".ShowWindowPosition"),
    auto_raise(rm, true, scrname+".autoRaise", altscrname+".AutoRaise"),
    click_raises(rm, true, scrname+".clickRaises", altscrname+".ClickRaises"),
    default_deco(rm, "NORMAL", scrname+".defaultDeco", altscrname+".DefaultDeco"),
    tab_placement(rm, FbWinFrame::TOPLEFT, scrname+".tab.placement", altscrname+".Tab.Placement"),
    windowmenufile(rm, Fluxbox::instance()->getDefaultDataFilename("windowmenu"), scrname+".windowMenu", altscrname+".WindowMenu"),
    typing_delay(rm, 0, scrname+".noFocusWhileTypingDelay", altscrname+".NoFocusWhileTypingDelay"),
    workspaces(rm, 4, scrname+".workspaces", altscrname+".Workspaces"),
    edge_snap_threshold(rm, 10, scrname+".edgeSnapThreshold", altscrname+".EdgeSnapThreshold"),
    edge_resize_snap_threshold(rm, 0, scrname+".edgeResizeSnapThreshold", altscrname+".EdgeResizeSnapThreshold"),
    focused_alpha(rm, 255, scrname+".window.focus.alpha", altscrname+".Window.Focus.Alpha"),
    unfocused_alpha(rm, 255, scrname+".window.unfocus.alpha", altscrname+".Window.Unfocus.Alpha"),
    menu_alpha(rm, 255, scrname+".menu.alpha", altscrname+".Menu.Alpha"),
    menu_delay(rm, 200, scrname + ".menuDelay", altscrname+".MenuDelay"),
    tab_width(rm, 64, scrname + ".tab.width", altscrname+".Tab.Width"),
    tooltip_delay(rm, 500, scrname + ".tooltipDelay", altscrname+".TooltipDelay"),
    allow_remote_actions(rm, false, scrname+".allowRemoteActions", altscrname+".AllowRemoteActions"),
    clientmenu_use_pixmap(rm, true, scrname+".clientMenu.usePixmap", altscrname+".ClientMenu.UsePixmap"),
    tabs_use_pixmap(rm, true, scrname+".tabs.usePixmap", altscrname+".Tabs.UsePixmap"),
    max_over_tabs(rm, false, scrname+".tabs.maxOver", altscrname+".Tabs.MaxOver"),
    default_internal_tabs(rm, true /* TODO: autoconf option? */ , scrname+".tabs.intitlebar", altscrname+".Tabs.InTitlebar"),
    opaque_resize_delay(rm, 50, scrname + ".opaqueResizeDelay", altscrname+".OpaqueResizeDelay")    {

}
