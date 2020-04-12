whats this? this is the attempt to write the documentation for 
fluxbox in ascii-doc format. how does it work?

well, just edit fluxbox.txt from this directory. save it.
then we can produce pretty much any format we like: man, pdf, docbook, html,
etc.

For simplicity the Makefile here is set up to refresh the man pages that will be
installed with fluxbox.  Just run 'make dist' in this directory. It requires
that you have a recent asciidoc package installed from
http://www.methods.co.nz/asciidoc/ and xmlto from
https://pagure.io/xmlto. The result will be new fluxbox.1.in
files in the parent directory.  These and your altered .txt files should be
checked into git.

Here are the nuts and bolts to create other formats:

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
