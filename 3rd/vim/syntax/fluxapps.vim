" File Name: fluxapps.vim
" Maintainer: M.Gumz aka ak|ra (#fluxbox on freenode) <akira at fluxbox.org>
" Original Date: 2004-02-06
" Last Update: 2011-01-23
" Description: fluxbox apps-file syntax

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syntax case ignore

syntax keyword fbStartTag app startup transient group contained
syntax keyword fbEndTag end contained

syntax keyword fbRemember Workspace Jump Head Layer Dimensions
syntax keyword fbRemember Position Deco Shaded Tab
syntax keyword fbRemember FocusHidden IconHidden Hidden
syntax keyword fbRemember Sticky Minimized Maximized Fullscreen
syntax keyword fbRemember Close Alpha

syntax keyword fbValue UPPERRIGHT UPPERLEFT LOWERRIGHT LOWERLEFT WINCENTER CENTER

syntax keyword fbPropertyName Name Class Title Role Transient Maximize Minimize contained
syntax keyword fbPropertyName Shaded Stuck FocusHidden IconHidden Urgent contained
syntax keyword fbPropertyName Workspace WorkspaceName Head Layer contained

syntax match fbRegexp /[-0-9A-Za-z_\.]\+/ contained
syntax match fbSeparator /\>!\?=\</ contained
syntax match fbClientPattern /(\w\{-}.\{1,2}[-0-9A-Za-z_\.]\{-})/hs=s+1,he=e-1 contained contains=fbPropertyName,fbSeparator,fbRegexp skipwhite nextgroup=fbClientPattern
syntax match fbAppStart /\[\w\+\]/ contains=fbStartTag,fbRemember skipwhite nextgroup=fbClientPattern
syntax match fbAppEnd /\[\w\+\]$/ contains=fbEndTag
syntax match fbValue /{.*}/hs=s+1,he=e-1
syntax match fbComment /[#].*$/

highlight link fbStartTag Type
highlight link fbEndTag Type
highlight link fbRemember Macro
highlight link fbComment Comment
highlight link fbValue String
highlight link fbPropertyName Number
highlight link fbSeparator Function
highlight link fbRegexp Constant
highlight link fbClientPattern Error
syntax sync fromstart

let b:current_syntax = 'fluxapps'
