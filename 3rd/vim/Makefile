PLUGIN = fluxbox
SOURCE = syntax/fluxapps.vim \
		 syntax/fluxkeys.vim \
		 syntax/fluxmenu.vim

${PLUGIN}.vba: $(SOURCE) compile_vba.sh Makefile
	sh ./compile_vba.sh $(SOURCE) > $@

clean:
	rm ${PLUGIN}.vba
