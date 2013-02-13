#!/bin/sh
# autogen script for fluxbox.

dothis() {
    echo "Executing:  $*"
    echo
    $*
    if [ $? -ne 0 ]; then
        echo -e '\n ERROR: Carefully read the error message and'
        echo      '        try to figure out what went wrong.'
        exit 1
    fi
}

libtoolize --copy --force --automake
rm -f config.cache
dothis aclocal -I m4 ${ACLOCAL_FLAGS}
dothis autoheader
dothis automake -a -c
dothis autoconf

echo 'Success, now continue with ./configure'
echo 'Use configure --help for detailed help.'
