#!/bin/sh
rm -f config.cache
automake
autoheader
aclocal
autoconf
