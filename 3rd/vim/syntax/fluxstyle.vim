" File Name: fluxstyle.vim
" Maintainer: Jason Carpenter <argonaut.linux@gmail.com>
" Original Date: June 30, 2019
" Last Update: June 30, 2019
" Description: fluxbox style syntax file

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" turn case on
syn case match

syn match   fbStyleLabel +^[^:]\{-}:+he=e-1   contains=fbStylePunct,fbStyleSpecial,fbStyleLineEnd

syn region  fbStyleValue   keepend start=+:+lc=1 skip=+\\+ end=+$+ contains=fbStyleSpecial,fbStyleLabel,fbStyleLineEnd

syn match   fbStyleSpecial    contained +#override+
syn match   fbStyleSpecial    contained +#augment+
syn match   fbStylePunct      contained +[.*:]+
syn match   fbStyleLineEnd    contained +\\$+
syn match   fbStyleLineEnd    contained +\\n\\$+
syn match   fbStyleLineEnd    contained +\\n$+

syn match   fbStyleComment "^!.*$"    contains=fbStyleTodo,@Spell
syn region  fbStyleComment start="/\*" end="\*/"       contains=fsStyleTodo,@Spell

syn keyword fbStyleTodo contained TODO FIXME XXX display

highlight link fbStyleLabel     Type
highlight link fbStyleValue   Constant
highlight link fbStyleComment   Comment
highlight link fbStyleSpecial   Statement
highlight link fbStylePunct     Normal
highlight link fbStyleLineEnd   Special
highlight link fbStyleTodo      Todo

syntax sync fromstart

let b:current_syntax = 'fluxstyle'
