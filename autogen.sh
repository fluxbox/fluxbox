#!/bin/sh
required_version=52
echo -n "Checking for autoconf version >= "${required_version}"......"
ac_version=`autoconf --version|head -n 1|cut -d . -f 2`
if [ ${ac_version} -lt ${required_version} ]; then
	echo "No."
	echo "You need to install autoconf version >= "${required_version}
	exit 1
fi
echo "Ok."

libtoolize --copy --force --automake || exit 1
rm -f config.cache
aclocal
autoheader
automake -a
autoconf
echo "Done."


