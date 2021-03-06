#!/bin/sh -e
# "autogen.sh" 
# Maintainer script for regenerating Makefile.ins and configure scripts.
# picks up AUTOCONF, AUTOMAKE, etc. from environment, which is used
# to override the default program names.

# this regenerates some critical files
# run this after "make maintainer-clean" or "make distclean"
# to really clean everything, run scripts/maintainerclobber

PROJECT=bj-jedi
TEST_TYPE=-f
FILE=AUTHORS
# where our m4/autoconf macros reside (relative to top_srcdir)
PROJECT_MACROS=config

# defaults
test "$AUTOCONF" || AUTOCONF=autoconf
test "$AUTOHEADER" || AUTOHEADER=autoheader
test "$AUTOMAKE" || AUTOMAKE=automake
test "$ACLOCAL" || ACLOCAL=aclocal
test "$LIBTOOLIZE" || LIBTOOLIZE=libtoolize
# ACLOCAL_FLAGS is also passable from the environment, save it, append later
USER_ACLFLAGS="$ACLOCAL_FLAGS"

DIE=0

# TODO: version compare

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to bootstrap $PROJECT from CVS."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at: "
	echo "	http://ftp.gnu.org/gnu/autoconf/autoconf-2.63.tar.gz"
        DIE=1
}

($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have libtool installed to bootstrap $PROJECT from CVS."
	echo "Get either of the following:"
	echo "	ftp://ftp.gnu.org/gnu/libtool/libtool-2.4.2.tar.gz"
        echo "(or a newer version if it is available)"
        DIE=1
}

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have automake installed to bootstrap $PROJECT from CVS."
	echo "Get any of the following:"
        echo "	http://ftp.gnu.org/gnu/automake/automake-1.10.2.tar.gz"
        echo "	http://ftp.gnu.org/gnu/automake/automake-1.11.tar.gz"
        echo "(or a newer version if it is available)"
        DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

# prepare ACLOCAL_FLAGS
ACLOCAL_FLAGS="-I $PROJECT_MACROS"
# check libtool version
# lt_vers=`$LIBTOOLIZE --version | head -n 1 | sed 's/^[^0-9]*//'`
# lt_vers_maj=`echo $lt_vers | cut -d. -f1`
# test "$lt_vers_maj" != 2 || ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I libltdl/m4"
test -z "$USER_ACLFLAGS" || ACLOCAL_FLAGS="$ACLOCAL_FLAGS $USER_ACLFLAGS"

echo "using:"
echo "AUTOCONF = $AUTOCONF"
echo "AUTOHEADER = $AUTOHEADER"
echo "AUTOMAKE = $AUTOMAKE"
echo "ACLOCAL = $ACLOCAL"
echo "LIBTOOLIZE = $LIBTOOLIZE"
echo "ACLOCAL_FLAGS = $ACLOCAL_FLAGS"

test $TEST_TYPE $FILE || {
        echo "You must run this script in the top-level $PROJECT directory"
        exit 1
}

run_aclocal () {
echo "Running $ACLOCAL..."
$ACLOCAL $ACLOCAL_FLAGS
}

run_autoheader () {
# optionally feature autoheader
echo "Running $AUTOHEADER..."
($AUTOHEADER --version)  < /dev/null > /dev/null 2>&1 && $AUTOHEADER $ACLOCAL_FLAGS
}

run_automake () {
echo "Running $AUTOMAKE..."
$AUTOMAKE --copy --add-missing --gnu $am_opt
# --verbose if needed
}

run_autoconf () {
echo "Running $AUTOCONF..."
$AUTOCONF $ACLOCAL_FLAGS
}

# function, no arguments
regen () {
printf "In directory: "
pwd
rm -rf autom4te.cache
run_aclocal
run_autoheader
run_automake
run_autoconf
# really, 'autoreconf' with flags should suffice
}

# clean out config directories first
if false
then
for d in config libltdl
do
	pushd $d
	../scripts/maintainerclobber -k
	# -v for verbose
	popd
done
fi

echo "Running $LIBTOOLIZE..."
# wipe it first
$LIBTOOLIZE --copy --force
# --ltdl
# don't need ltdl yet

if false
then
# In the freshly installed libltdl directory:
# we forcibly remove some autotool files to force them to be regenerated
# using the local versions of the autotools, in case of version mismatch.
# Also point the config dir to the top-level one to share common files.
(cd libltdl && \
	mkdir -p removed && \
	mkdir -p config && \
	mv -f configure.ac configure.ac.bkp && \
	sed -e "/^AC_CONFIG_AUX_DIR/s|(.*)|([../$PROJECT_MACROS])|" \
		-e '/LT_CONFIG/a\
AC_ARG_VAR(ACLOCAL_FLAGS, [Additional aclocal flags, for bootstrap only.])' \
		configure.ac.bkp > configure.ac && \
	mv -f configure.ac.bkp removed && \
	mv -f Makefile.in configure aclocal.m4 removed ; \
	mv -f config.* install-sh ltmain.sh missing removed || : ; \
	echo "moving m4/* to ../$PROJECT_MACROS ..." ; \
	if test -d m4 ; then mv -f m4/*.m4 ../$PROJECT_MACROS ; \
		rm -rf m4 ; fi ; \
	sed -i.bkp -e "/ACLOCAL_AMFLAGS/s|m4|../$PROJECT_MACROS \$(ACLOCAL_FLAGS)|" Makefile.am ; \
	mv -f Makefile.am.bkp removed ; \
	echo "cvsignore:" >> Makefile.am ; \
	regen )
fi

# back to top_srcdir, should regenerate libltdl's files, recursively
regen

echo "Done bootstrapping.  You may now 'configure'."

# may not want verbose for autoconf, aclocal, and autoheader

