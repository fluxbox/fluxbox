" File Name: fluxkeys.vim
" Maintainer: Mathias Gumz <akira at fluxbox dot org>
" Original Date: 2004-01-27
" Changelog:
"   * 2010-09-19 update from Segaja
"   * 2009-11-01 update to current fluxbox syntax (thanx to Harry Bullen)
"   * 2005-06-24
" Description: fluxbox key syntax
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

" quit when a syntax file was already loaded
if exists("b:current_syntax")
    finish
endif

syntax case ignore

" COMMANDS

" Mouse Commands
syntax keyword fbActionNames StartMoving StartResizing StartTabbing  contained

" Window Commands
syntax keyword fbActionNames Minimize MinimizeWindow Iconify  contained
syntax keyword fbActionNames Maximize MaximizeWindow MaximizeHorizontal MaximizeVertical Fullscreen  contained
syntax keyword fbActionNames Raise Lower RaiseLayer LowerLayer SetLayer  contained
syntax keyword fbActionNames Close Kill KillWindow  contained
syntax keyword fbActionNames Shade ShadeWindow ShadeOn ShadeOff  contained
syntax keyword fbActionNames Stick StickWindow  contained
syntax keyword fbActionNames SetDecor ToggleDecor  contained
syntax keyword fbActionNames NextTab PrevTab Tab MoveTabRight MoveTabLeft DetachClient  contained
syntax keyword fbActionNames ResizeTo Resize ResizeHorizontal ResizeVertical  contained
syntax keyword fbActionNames MoveTo Move MoveRight MoveLeft MoveUp MoveDown  contained
syntax keyword fbActionNames TakeToWorkspace SendToWorkspace  contained
syntax keyword fbActionNames TakeToNextWorkspace TakeToPrevWorkspace SendToNextWorkspace SendToPrevWorkspace  contained
syntax keyword fbActionNames SetAlpha  contained
syntax keyword fbActionNames SetHead SendToNexthead SendToPrevHead  contained
syntax keyword fbActionNames ActivateTab  contained

" Workspace Commands
syntax keyword fbActionNames AddWorkspace RemoveLastWorkspace  contained
syntax keyword fbActionNames NextWorkspace PrevWorkspace RightWorkspace LeftWorkspace Workspace  contained
syntax keyword fbActionNames NextWindow PrevWindow Next\Group PrevGroup GotoWindow  contained
syntax keyword fbActionNames Activate Focus Attach FocusLeft FocusRight FocusUp FocusDown  contained
syntax keyword fbActionNames ArrangeWindows ShowDesktop Deiconify CloseAllWindows  contained
syntax keyword fbActionNames ArrangeWindowsHorizontal ArrangeWindowsVertical  contained
syntax keyword fbActionNames ArrangeWindowsStackLeft ArrangeWindowsStackRight  contained
syntax keyword fbActionNames ArrangeWindowsStackTop ArrangeWindowsStackBottom  contained
syntax keyword fbActionNames SetWorkspaceName SetWorkspaceNameDialog  contained

" Menu Commands
syntax keyword fbActionNames RootMenu WorkspaceMenu WindowMenu ClientMenu CustomMenu HideMenus  contained

" Window Manager Commands
syntax keyword fbActionNames Restart Quit Exit  contained
syntax keyword fbActionNames Reconfig Reconfigure  contained
syntax keyword fbActionNames SetStyle ReloadStyle  contained
syntax keyword fbActionNames ExecCommand Exec Execute CommandDialog  contained
syntax keyword fbActionNames SetEnv Export  contained
syntax keyword fbActionNames SetResourceValue SetResourceValueDialog  contained

" Special Commands
syntax keyword fbActionNames MacroCmd Delay ToggleCmd  contained
syntax keyword fbActionNames BindKey KeyMode  contained
syntax keyword fbActionNames ForEach Map  contained
syntax keyword fbActionNames If Cond  contained


" MODIFIERS
syntax keyword fbModifierNames Control Shift Double  contained
syntax keyword fbModifierNames Mod1 Mod2 Mod3 Mod4 Mod5  contained
syntax keyword fbModifierNames None  contained
syntax keyword fbModifierNames OnDesktop OnToolbar OnTitlebar OnTab OnWindow OnWindowBorder OnLeftGrip OnRightGrip contained

" reference corners
syntax keyword fbParameterNames UpperLeft LeftTop TopLeft contained
syntax keyword fbParameterNames Upper Top TopCenter contained
syntax keyword fbParameterNames UpperRight RightTop TopRight contained
syntax keyword fbParameterNames Left LeftCenter contained
syntax keyword fbParameterNames Center WinCenter contained
syntax keyword fbParameterNames Right RightCenter contained
syntax keyword fbParameterNames LowerLeft LeftBottom BottomLeft contained
syntax keyword fbParameterNames Lower Bottom BottomCenter contained
syntax keyword fbParameterNames LowerRight RightBottom BottomRight contained
" deiconfiy
syntax keyword fbParameterNames LastWorkspace Last All AllWorkspace OriginQuiet contained

" parameter numbers
syntax match   fbParameterNumber /\([+-]\)*\d\+/ contained


"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" match the right parts
syntax match   fbParameter /.*/ contained contains=fbParameterNames,fbParameterNumber

" anything with an unknown ActionName is colored Error
syntax match   fbAction /\w\+/ contained contains=fbActionNames nextgroup=fbParameter
syntax match   fbExecAction /Exec\(ute\|Command\)*\s\+.*$/ contained contains=fbActionNames

" stuff for macro and toggle magic
syntax match   fbMTParameter /.\{-\}}/ contained contains=fbParameterNames,fbParameterNumber
syntax match   fbMTAction /\w\+/ contained contains=fbActionNames nextgroup=fbMTParameter
syntax region  fbMTExecAction start=/{Exec\(ute\|Command\)*\s\+/hs=s+1 end=/.\{-}}/he=e-1 contained contains=fbActionNames oneline

" macro magic
syntax region  fbMacro start=/{/ end=/.\{-}}/ contained contains=fbMTExecAction,fbMTAction oneline nextgroup=fbMacro skipwhite
syntax match   fbMacroStart /MacroCmd\s\+/ contained contains=fbActionNames nextgroup=fbMacro

" toggle magic
syntax match   fbToggleError /.$/ contained skipwhite
syntax match   fbToggle2 /{.\{-}}/ contained contains=fbMTExecAction,fbMTAction nextgroup=fbToggleError skipwhite
syntax match   fbToggle1 /{.\{-}}/ contained contains=fbMTExecAction,fbMTAction nextgroup=fbToggle2 skipwhite
syntax match   fbToggleStart /ToggleCmd\s\+/ contained contains=fbActionNames nextgroup=fbToggle1

" anything but a valid modifier is colored Error
syntax match   fbKeyStart /^\w\+/  contained contains=fbModifierNames

" anything but a comment or a valid key line is colored Error
syntax match   fbNoKeyline /.\+$/ display skipwhite
syntax region  fbKeys start=/\w\+/ end=/.\{-}:/he=e-1 contains=fbKeyStart,fbModifierNames nextgroup=fbMacroStart,fbToggleStart,fbExecAction,fbAction oneline
syntax match   fbComment /[#!].*$/ display 

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" coloring
highlight link fbNoKeyline Error
highlight link fbAction Error
highlight link fbKeyStart Error
highlight link fbToggleError Error

highlight link fbComment Comment
highlight link fbKeys Number
highlight link fbExecAction String
highlight link fbMTExecAction String
highlight link fbActionNames Type 
highlight link fbModifierNames Macro
highlight link fbParameter Number
highlight link fbParameterNames Function
highlight link fbParameterNumber Conditional

syntax sync fromstart

let b:current_syntax = 'fluxkeys'
