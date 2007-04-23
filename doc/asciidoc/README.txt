whats this? this is the attempt to write the documentation for 
fluxbox in ascii-doc format. how does it work?

well, just edit fluxbox.txt from this directory. save it.
then we can produce pretty much any format we like:

man:

  $> asciidoc -b docbook -d manpage fluxbox.txt
  $> xmlto man fluxbox.xml

pdf:

  $> asciidoc -b docbook -d manpage fluxbox.txt
  $> docbook2pdf fluxbox.xml

docbook:

  $> asciidoc -b docbook-sgml -d manpage fluxbox.txt

html:

  $> asciidoc -b xhtml -d manpage fluxbox.txt

and many many more ways to do it.
what do we need? well, at least:

  http://www.methods.co.nz/asciidoc/
  http://cyberelk.net/tim/xmlto/

and the rest of the docbook-family + maybe pdftex.
