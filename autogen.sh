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

# recent gettext installs its m4 macros (AM_ICONV, ...) next to its own
# data instead of into aclocal's directory
gettext_m4="$(dirname "$(command -v autopoint)")/../share/gettext/m4"
if [ -d "$gettext_m4" ]; then
    ACLOCAL_FLAGS="-I $gettext_m4 ${ACLOCAL_FLAGS}"
fi

dothis aclocal -I m4 ${ACLOCAL_FLAGS}
dothis autoheader
dothis automake -a -c
dothis autoconf

echo 'Success, now continue with ./configure'
echo 'Use configure --help for detailed help.'
