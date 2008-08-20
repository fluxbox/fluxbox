#
# create manpages from the asciidoc files
#

SRC = fluxbox.txt fluxbox.de.txt fluxbox.es.txt \
	  fluxstyle.txt fluxbox-keys.txt fluxbox-remote.txt

MAN = $(SRC:.txt=.1)

all : $(MAN)
clean:
	rm -fv $(MAN)

%.xml : %.txt
	asciidoc -b docbook -d manpage -o $@ $<

%.1 : %.xml
	xmlto man $<

