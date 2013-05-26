#
# create manpages from the asciidoc files
#

SRC1 = \
	fbrun.txt \
	fbsetbg.txt \
	fbsetroot.txt \
	fluxbox-remote.txt \
	fluxbox.txt \
	startfluxbox.txt

SRC5 = \
	fluxbox-apps.txt \
	fluxbox-keys.txt \
	fluxbox-menu.txt \
	fluxbox-style.txt

MAN = \
	$(SRC1:.txt=.1) \
	$(SRC5:.txt=.5)

MANdotIN = $(addsuffix .in,$(MAN))
DISTFILES = $(addprefix ../,$(MANdotIN))

all: $(MAN)

dist: $(DISTFILES)

clean:
	rm -fv $(MAN)

%.1 : %.xml
	xmlto man $<

%.5 : %.xml
	xmlto man $<

../%.1.in: %.1
	cp $< $@

../%.5.in: %.5
	cp $< $@

%.xml : %.txt
	asciidoc -b docbook -d manpage -o $@ $<

# Dependencies:
#
# fluxbox-apps(5) and fluxbox-keys(5) both include client-patterns.txt
fluxbox-keys.5 fluxbox-apps.5: client-patterns.txt
