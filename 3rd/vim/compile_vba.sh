#!/bin/sh
#
#   about:
#
#   compile_vba.sh - create a vimball from the given files
#
#  author: mathias gumz <akira at fluxbox org>
# licence: MIT
#
#   usage:
#
#     compile_vba.sh infile1 infile2 infile3
#
#     the vimball is dumped to stdout, to create a .vba.gz 
#     just pipe the output to gzip

cat <<EOF
" Vimball Archiver compile_vba.sh of fluxbox
UseVimball
finish
EOF

while [ $# -gt 0 ]
do
    echo "${1}	[[[1"
    # count line numbers via 'sed', out of 'wc' had to be
    # treated anyway
    sed -n '$=' $1
    cat $1
    shift
done
