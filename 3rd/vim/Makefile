PLUGIN = fluxbox
SOURCE = syntax/fluxapps.vim \
		 syntax/fluxkeys.vim \
		 syntax/fluxmenu.vim \
		 syntax/fluxstyle.vim \
		 ftdetect/fluxbox.vim

${PLUGIN}.vba: $(SOURCE) compile_vba.sh Makefile
	sh ./compile_vba.sh $(SOURCE) > $@

clean:
	rm ${PLUGIN}.vba
