#
# create manpages from the asciidoc files
#

SRC1 = fluxbox.txt fluxstyle.txt fluxbox-keys.txt fluxbox-remote.txt
SRC5 = fluxbox-keys.txt fluxbox-apps.txt

MAN = $(SRC1:.txt=.1) $(SRC5:.txt=.5)

all : $(MAN)

clean:
	rm -fv $(MAN)

%.xml : %.txt
	asciidoc -b docbook -d manpage -o $@ $<

%.1 : %.xml
	xmlto man $<

%.5 : %.xml
	xmlto man $<

# Dependencies:
#
# fluxbox-apps(5) and fluxbox-keys(5) both include client-patterns.txt
fluxbox-keys.5 fluxbox-apps.5: client-patterns.txt
