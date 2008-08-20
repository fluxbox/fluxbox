#
# create manpages from the asciidoc files
#

SRC1 = fluxbox.txt
SRC5 =

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

