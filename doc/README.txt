Guide to Fluxbox documentation
==============================

The bulk of the documentation for Fluxbox is in the (venerable) form of man
pages for the *fluxbox* executable itself, for several associated executables,
and for the file formats for the various configuration files.

The definitive source for these man pages consists of files in ascii-doc
format, contained in the asciidoc subdirectory. The Makefile in the main
directory will be constructed to massage these in various steps into man
pages, although you will need to install various prerequisites (see below).
You can also use the asciidoc system to generate the documentation in other
formats (for which also see below).

In addition, there are a couple of files in this directory primarily of
interest to Fluxbox developers:

  CODESTYLE -- conventions to use in writing/formatting Fluxbox source code
  the ewmh-support spreadsheet -- details the status of Fluxbox's support
                                  for various Extended Window Manager Hints.

These files are not processed in any way in building Fluxbox or installed
anywhere.

Note that there are also German and Spanish versions of the main fluxbox man
page in subdirectories of asciidoc, but they are not currently processed by
the top-level `make` of Fluxbox. It is also not entirely clear whether they
have been kept completely up to date.

Prerequisites to build
----------------------
what do we need? well, at least:

  http://www.methods.co.nz/asciidoc/
  http://cyberelk.net/tim/xmlto/

and the rest of the docbook-family + maybe pdftex.

Producing the docs in other formats
-----------------------------------
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

