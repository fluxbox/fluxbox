" recognize fluxbox files
if has("autocmd")
    autocmd BufNewFile,BufRead */.fluxbox/apps setf fluxapps
    autocmd BufNewFile,BufRead */.fluxbox/keys setf fluxkeys
    autocmd BufNewFile,BufRead */.fluxbox/menu setf fluxmenu
    autocmd BufNewfile,BufRead */.fluxbox/styles/* setf fluxstyle
endif
