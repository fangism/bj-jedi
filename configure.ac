# "configure.ac"
# Process this file with "autoconf" to generate the "configure" script.

AC_PREREQ(2.59)
dnl also works with autoconf 2.60 now
AC_INIT(bj-jedi, 0.0.1, fangism@users.sourceforge.net)
dnl prefer to push this into src directory, 
dnl and have "src/config.h" wrap around this.  
AC_CONFIG_HEADERS([src/__config__.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_SRCDIR([src/bj/core/strategy.cc])

# Propagate the make warning flags recursively
AM_INIT_AUTOMAKE([1.9 -Wall -Werror])

# Enable or disable automatic reconfiguring.  
# AM_MAINTAINER_MODE

AC_CANONICAL_HOST

AC_CHECK_PROG([SH], sh, sh)
AC_PATH_PROG(SH_PATH, sh)
AC_CHECK_PROG([CHMOD], chmod, chmod)
AC_CHECK_PROG([DIFF], diff, diff)
AC_CHECK_PROG([FIND], find, find)
AC_CHECK_PROG([MV], mv, mv)
AC_CHECK_PROG([GREP], grep, grep)
AC_CHECK_PROG([PRINTF], printf, printf)
dnl AC_CHECK_PROG([SORT], sort, sort)
dnl AC_CHECK_PROG([CUT], cut, cut)
dnl AC_CHECK_PROG([PASTE], paste, paste)
dnl AC_CHECK_PROG([HEAD], head, head)
AC_CHECK_PROG([TAIL], tail, tail)
dnl AC_CHECK_PROG([TEE], tee, tee)
dnl AC_CHECK_PROG([TR], tr, tr)
AC_CHECK_PROG([SED], sed, sed)
AC_CHECK_PROG([TR], tr, tr)
AC_CHECK_PROG([TOUCH], touch, touch)

# documentation support, macros efined in "config/documentation.m4"
dnl AC_CHECK_PROG([LATEX], latex, latex)
AC_CHECK_PROG([PDFLATEX], pdflatex, pdflatex)
AC_CHECK_PROG([MAKEINDEX], makeindex, makeindex)
AC_CHECK_PROG([BIBTEX], bibtex, bibtex)
dnl already checked and defined by texinfos.am
dnl DOC_CHECK_PROG_DVIPS
dnl AC_CHECK_PROG([INFO], info, info)
dnl AM_CONDITIONAL(HAVE_INFO, test -n "$INFO" )
dnl AC_CHECK_PROG([MAKEINFO], makeinfo, makeinfo)
dnl covered by texinfos.am already
dnl AC_CHECK_PROG([TEXI2DVI], texi2dvi, texi2dvi)
dnl AC_CHECK_PROG([TEXI2PDF], texi2pdf, texi2pdf)
dnl AC_CHECK_PROG([TEXI2HTML], texi2html, texi2html)

dnl check for graphviz
AC_CHECK_PROG([DOT], dot, dot)

dnl checks for xfig, transfig, fig2dev
AC_CHECK_PROG([FIG2DEV], fig2dev, fig2dev)

dnl future: gtk+2 will come from pkgconfig
dnl AC_CHECK_PROG([PKG_CONFIG], pkg-config, pkg-config)
dnl AM_CONDITIONAL(HAVE_PKG_CONFIG, test -n "$PKG_CONFIG" )

dnl AC_CHECK_PROG([SWIG], swig, swig)
dnl AM_CONDITIONAL(HAVE_SWIG, test -n "$SWIG" )

# This takes care of many standard C[++] compiler configuration checks.  
AC_PROG_RANLIB

dnl -------------------------- LIBTOOL SECTION --------------------------------
dnl MAINTAINER INSTRUCTIONS: 
dnl uncomment the macros in either the 1.5.x section or the 2.x section
dnl need ltdl: dlopen-ing plug-ins and module shared objects/libraries

dnl - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
m4_ifdef([LT_INIT],[
dnl libtool-2.x setup
dnl AC_CONFIG_MACRO_DIR([libltdl/m4])	dnl had trouble with this
AC_CONFIG_MACRO_DIR([config])
LT_INIT
dnl LT_INIT([dlopen])
dnl AC_LIBLTDL_INSTALLABLE
AC_DISABLE_STATIC
dnl LTDL_INIT
dnl LT_MODULE_PATH_VAR=$lt_cv_module_path_var
LT_OBJDIR=$lt_cv_objdir
dnl - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
],[
dnl libtool-1.5.x setup
dnl AC_LIBLTDL_CONVENIENCE
dnl AC_LIB_LTDL
dnl AC_LIBTOOL_DLOPEN
AC_DISABLE_STATIC
dnl AC_LIBLTDL_INSTALLABLE
AC_PROG_LIBTOOL
dnl AC_CONFIG_SUBDIRS(libltdl)
dnl need these variables to augment test environment
dnl some of these are mapped over from libtool 1.5 to 2.x
dnl LTDL_SHLIBPATH_VAR=$libltdl_cv_shlibpath_var
dnl LT_MODULE_PATH_VAR=$LTDL_SHLIBPATH_VAR
LTDL_OBJDIR=$libltdl_cv_objdir
LT_OBJDIR=$LTDL_OBJDIR
])

dnl - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
dnl AC_SUBST(LTDLINCL)
dnl AC_SUBST(LIBLTDL)
# Automatically update if libtool script is outdated.  
# Adds corresponding dependencies to "Makefile.in".  
AC_SUBST(LIBTOOL_DEPS)
dnl need these variables to augment test environment
dnl LT_MODULE_EXT=$libltdl_cv_shlibext
dnl AC_SUBST(LT_MODULE_EXT)
dnl AC_SUBST(LT_MODULE_PATH_VAR)
AC_SUBST(LT_OBJDIR)

dnl ----------------------- end LIBTOOL SECTION -------------------------------
dnl we move this down here b/c libtool.m4:LT_INIT redefines ECHO with 
dnl its own improperly quoted "printf '%s\n'", which comes out wrong
dnl in Makefiles after substitution, due to quote stripping.  
ECHO=
AC_CHECK_PROG([ECHO], echo, echo)

AC_ARG_VAR(ACLOCAL_FLAGS, [Additional aclocal flags, for bootstrap only.])

FANG_ARG_WITH_READLINE_EDITLINE

dnl AC_ARG_ENABLE for building documentation (on by default)
dnl from "config/documentation.m4"
dnl DOC_ARG_ENABLE_DOCS

dnl define WORDS_BIGENDIAN if is big endian
dnl not actually needed in this project yet
AC_C_BIGENDIAN

dnl check for __typeof__ operator, or equivalent
AC_C_TYPEOF
AC_PROG_CXX

dnl from "config/cxx.m4"
dnl FANG_CXX_VERSION

dnl checkpoint
AC_CACHE_SAVE

dnl we need these anal flags for checking for certain charateristics
dnl TODO: what are the equivalent flags for other compilers?
dnl from "config/cxx.m4"
dnl FANG_ANAL_COMPILE_FLAGS
dnl FANG_AM_FLAGS

dnl which set of pre-determined compiler flags to apply?
dnl FANG_CXX_COMPILER
dnl TODO: add other conditionals for other common compilers
dnl XLC++, ICC, HPCC, etc...

AM_PROG_CC_C_O
AC_ISC_POSIX

dnl some compiler support
FANG_CXX_ATTRIBUTE_UNUSED
FANG_CXX_ATTRIBUTE_ALIGNED
FANG_CXX_ATTRIBUTE_ALIGNED_SIZE
FANG_CXX_ATTRIBUTE_VISIBILITY
FANG_CXX_ATTRIBUTE_MALLOC

dnl checks tons of standard C++ headers in various locations
dnl from "config/cxx_STL.m4"
dnl from this point to the end, we also apply ANAL_FLAGS before CFLAGS
dnl and CXXFLAGS to reflect what will be used during the build process.  
dnl FANG_HEADER_STDCXX

AC_CACHE_SAVE

# additional header checks
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h stdint.h string.h unistd.h getopt.h])
AC_CHECK_HEADERS([ctype.h])
AC_HEADER_DIRENT
dnl AC_CHECK_HEADERS([dirent.h sys/dirent.h])

AC_CACHE_SAVE

dnl we deliberaly spare these basic check from Fang's anal compile flags
dnl suggestions from autoscan
dnl AC_FUNC_ALLOCA			dnl wanted by the y.tab.c generated
dnl AC_FUNC_CLOSEDIR_VOID		dnl wanted by src/util/dirent.cc
dnl AC_FUNC_STAT			dnl wanted by src/util/file_status.h

# compiler characteristic checks
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_C_VOLATILE

# library functions
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([calloc free])

top_saved_CFLAGS="$CFLAGS"
top_saved_CXXFLAGS="$CXXFLAGS"
CFLAGS="$CFLAGS -Wno-error"
CXXFLAGS="$CXXFLAGS -Wno-error"
dnl temporarily override ac_compile and ac_link to force checking in C++ mode
dnl language mode doesn't really matter because these test by linking
AC_LANG_PUSH(C++)
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_TYPES([std::ptrdiff_t])
AC_CHECK_FUNCS(
dnl	[getopt strsep strtok strtok_r strdup \]
dnl <ctype.h>
	[isascii isspace isdigit isalpha isalnum islower isupper \]
	[tolower toupper \]
	[abs labs llabs fabs fabsf fabsl \]
	[drand48 erand48 lrand48 nrand48 \]
	[mrand48 jrand48 srand48 seed48 lcong48 \]
	[system exec]
)
AC_LANG_POP(C++)
dnl but keep errors for type checks.  
CFLAGS="$top_saved_CFLAGS"
CXXFLAGS="$top_saved_CXXFLAGS"

# sizeof checks, useful for checking ILP model
AC_LANG_PUSH(C++)
dnl testing in C++ mode coerces test to use CXXFLAGS
AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(uchar)
AC_CHECK_SIZEOF(u_char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(ushort)
AC_CHECK_SIZEOF(u_short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(uint)
AC_CHECK_SIZEOF(u_int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(ulong)
AC_CHECK_SIZEOF(u_long)
dnl NOTE: o some systems, system headers protect long long, which causes
dnl the following test to *unexpectedly* pass even with -pedantic-errors
dnl This inconsistency between configure and use is quite annoying and 
dnl motivates us to automatically apply -Wno-long-long.
dnl Below, we call FANG_CXX_AUTO_NO_LONG_LONG after CXXFLAGS is restored.
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(ulonglong)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(void*)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(int16)
AC_CHECK_SIZEOF(uint16)
AC_CHECK_SIZEOF(u_int16)
AC_CHECK_SIZEOF(int16_t)
AC_CHECK_SIZEOF(uint16_t)
AC_CHECK_SIZEOF(u_int16_t)
AC_CHECK_SIZEOF(int32)
AC_CHECK_SIZEOF(uint32)
AC_CHECK_SIZEOF(u_int32)
AC_CHECK_SIZEOF(int32_t)
AC_CHECK_SIZEOF(uint32_t)
AC_CHECK_SIZEOF(u_int32_t)
AC_CHECK_SIZEOF(int64)
AC_CHECK_SIZEOF(uint64)
AC_CHECK_SIZEOF(u_int64)
AC_CHECK_SIZEOF(int64_t)
AC_CHECK_SIZEOF(uint64_t)
AC_CHECK_SIZEOF(u_int64_t)
AC_LANG_POP(C++)

CFLAGS="$top_saved_CFLAGS"
CXXFLAGS="$top_saved_CXXFLAGS"

AC_CACHE_SAVE

dnl PACKAGE CHECKS
dnl readline or editline support
FANG_CHECK_READLINE
FANG_CHECK_EDITLINE
dnl check prototype of readline() if applicable
FANG_CHECK_READLINE_ARG_CONST

dnl check for X11 includes and libraries
hackt_have_x11=no
AC_PATH_X
if test -d "$ac_x_includes" ; then
  X11_INCLUDE="-I$ac_x_includes"
  if test -d "$ac_x_libraries" ; then
    hackt_have_x11=yes
  fi
fi
if test -d "$ac_x_libraries" ; then
  X11_LDPATH="-L$ac_x_libraries"
fi
AC_SUBST(X11_INCLUDE)
AC_SUBST(X11_LDPATH)
if test "$hackt_have_x11" = "yes" ; then
    AC_DEFINE(HAVE_X11, 1, [Define to 1 if we X11 includes/libs found])
fi
AM_CONDITIONAL(HAVE_X11, test "$hackt_have_x11" = "yes")


dnl check for guile-config
dnl FANG_GUILE

dnl TODO: AC_SUBST these flags into separate options
dnl TODO: xdr
dnl for automake consumption
dnl order would matter if there are cross-conflicting headers installed
AUTO_CPPFLAGS="$EL_INCLUDE $RL_INCLUDE $NCURSES_INCLUDE $GMP_INCLUDE"
dnl order might matter in the presence of cross-conflicting libraries
AUTO_LDFLAGS="$EL_LDPATH $RL_LDPATH $NCURSES_LDPATH $GMP_LDPATH"
dnl order matters! we are taking responsibility for this
dnl rather than letting LIBS accumulate over checks.  
AUTO_LIBS="$EL_LIB $RL_LIB $NCURSES_LIB $GMP_LIB"
LIBS="$AUTO_LIBS"
dnl may decide not to use these, depending on how much control is needed
AC_SUBST(AUTO_CPPFLAGS)
AC_SUBST(AUTO_LDFLAGS)
dnl AC_SUBST(AUTO_LIBS)

dnl CONFIGDATE=`$DATE`
dnl AC_SUBST(CONFIGDATE)

cat <<ACEOF
configuration summary:
dnl	CONFIG_SHELL:		$CONFIG_SHELL
dnl	AWK:			$AWK
	CXX:			$CXX_VERSION
	CPPFLAGS:		$CPPFLAGS
	CXXFLAGS:		$CXXFLAGS
	LDFLAGS:		$LDFLAGS
dnl	LEX:			$LEX_VERSION
dnl	YACC:			$YACC_VERSION
dnl	ltdl include:		$LTDLINCL
dnl	ltdl libs:		$LIBLTDL
	readline include:	$RL_INCLUDE
	readline LDFLAGS:	$RL_LDPATH
	readline libs:		$RL_LIB
	editline include:	$EL_INCLUDE
	editline LDFLAGS:	$EL_LDPATH
	editline libs:		$EL_LIB
	ncurses include:	$NCURSES_INCLUDE
	ncurses LDFLAGS:	$NCURSES_LDPATH
	ncurses libs:		$NCURSES_LDPATH
dnl	GMP include:		$GMP_INCLUDE
dnl	GMP LDFLAGS:		$GMP_LDPATH
dnl	GMP libs:		$GMP_LIB
	X11 include:		$X11_INCLUDE
	X11 LDFLAGS:		$X11_LDPATH
dnl	guile:		 	$GUILE [[$GUILE_PATH]]
dnl	guile config:	 	$GUILE_CONFIG_VERSION
dnl	guile CPPFLAGS: 	$GUILE_CPPFLAGS
dnl	guile LDFLAGS: 		$GUILE_LDFLAGS
ACEOF

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	test/Makefile
	text/Makefile
])
dnl AC_CONFIG_FILES([pre-inst-env], [chmod +x pre-inst-env])
dnl AC_CONFIG_FILES([install_transform.sh], [chmod +x install_transform.sh])

dnl extra dirs
$MKDIR_P text/tables/dealer
$MKDIR_P text/figs

AC_OUTPUT

