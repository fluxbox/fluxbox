#!/bin/sh
# autogen script for fluxbox.

dothis() {
    echo "Executing:  $*"
    echo
    if ! $* ;then
        echo -e '\n ERROR: Carefully read the error message and'
        echo      '        try to figure out what went wrong.'
        exit 1
    fi
}

libtoolize --copy --force --automake
rm -f config.cache
dothis aclocal -I .
dothis autoheader
dothis automake -a
dothis autoconf

echo 'Succes, now continue with ./configure'
echo 'Use configure --help for detailed help.'
