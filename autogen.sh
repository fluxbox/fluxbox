#!/bin/sh
libtoolize --copy --force --automake
rm -f config.cache
aclocal -I .
autoheader
automake -a
autoconf
echo "Done."


