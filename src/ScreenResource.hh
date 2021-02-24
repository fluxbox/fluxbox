#ifndef SCREEN_RESOURCE_HH
#define SCREEN_RESOURCE_HH

// ScreenResource.hh for Fluxbox Window Manager
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

#include "FbWinFrame.hh"
#include "FbTk/Resource.hh"
#include <string>

struct ScreenResource {

    ScreenResource(FbTk::ResourceManager &rm,
            const std::string &scrname, const std::string &altscrname);

    FbTk::Resource<bool> opaque_move,
       opaque_resize,
       full_max,
       max_ignore_inc, 
       max_disable_move,
       max_disable_resize,
       workspace_warping,
       workspace_warping_vertical,
       show_window_pos,
       auto_raise,
       click_raises;

    FbTk::Resource<std::string> default_deco;
    FbTk::Resource<FbWinFrame::TabPlacement> tab_placement;
    FbTk::Resource<std::string> windowmenufile;
    FbTk::Resource<unsigned int> typing_delay;
    FbTk::Resource<int> workspaces,
        edge_snap_threshold,
        edge_resize_snap_threshold,
        focused_alpha,
        unfocused_alpha,
        menu_alpha,
        menu_delay,
        tab_width,
        tooltip_delay,
       workspace_warping_vertical_offset;
    FbTk::Resource<bool> allow_remote_actions;
    FbTk::Resource<bool> clientmenu_use_pixmap;
    FbTk::Resource<bool> tabs_use_pixmap;
    FbTk::Resource<bool> max_over_tabs;
    FbTk::Resource<bool> default_internal_tabs;
    FbTk::Resource<unsigned int> opaque_resize_delay;
};

#endif
