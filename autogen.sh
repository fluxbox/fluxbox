#!/bin/sh
libtoolize --copy --force --automake || exit 1
rm -f config.cache
aclocal
autoheader
automake
autoconf



