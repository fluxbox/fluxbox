#!/bin/sh
libtoolize --copy --force --automake
rm -f config.cache
aclocal
autoheader
automake -a
autoconf
echo "Done."


