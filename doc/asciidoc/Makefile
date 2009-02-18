#
# create manpages from the asciidoc files
#

SRC1 = fluxbox.txt fluxbox-remote.txt
SRC5 = fluxbox-keys.txt fluxbox-apps.txt fluxbox-style.txt fluxbox-menu.txt

MAN = $(SRC1:.txt=.1.in) $(SRC5:.txt=.5.in)

all: $(MAN)

install-above: all
	cp $(MAN) ../

clean:
	rm -fv $(MAN)

%.1 : %.xml
	xmlto man $<

%.5 : %.xml
	xmlto man $<

%.1.in: %.1
	cp $< $@

%.5.in: %.5
	cp $< $@

%.xml : %.txt
	asciidoc -b docbook -d manpage -o $@ $<

# Dependencies:
#
# fluxbox-apps(5) and fluxbox-keys(5) both include client-patterns.txt
fluxbox-keys.5 fluxbox-apps.5: client-patterns.txt
