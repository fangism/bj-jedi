# "Makefile.am"
#
# Process this file with "automake" to generate "Makefile.in".
#
#	$Id: Makefile.am,v 1.48 2009/09/28 06:56:50 fang Exp $

# include $(top_srcdir)/Make.stddef

AUTOMAKE_OPTIONS = 1.9 gnu subdir-objects dist-bzip2

SUBDIRS = . src test text

# When to update the libtool script:
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

# don't need $(srcdir) because cd $(top_srcdir) is prior to exec. aclocal
ACLOCAL_AMFLAGS = -I config @ACLOCAL_FLAGS@
# in future, may need -I libltdl/m4

DISTCLEANFILES = config.in.h~ distcheck.log
DISTCLEANDIRS = autom4te.cache

doc_DATA = AUTHORS COPYING NEWS README TODO

# bin_SCRIPTS =
# noinst_SCRIPTS = install_transform.sh

# Many of these are already in DIST_COMMON
EXTRA_DIST = autogen.sh
#	Make.stddef Make.global installdirs.mk

# distcheck is a top-level only target
# doesn't work, tries to override previous definition without accumulating
distcheck.log: force
	@if test -f $@ ; then $(MV) $@ $@~ ; fi
	@$(ECHO) "Logging \"make distcheck\" to $@ ..."
	@$(ECHO) Tests started `date` > $@.tmp
	-$(MAKE) DISTCHECK_CONFIGURE_FLAGS="$(DISTCHECK_CONFIGURE_FLAGS)" distcheck >> $@.tmp 2>&1
	-$(MV) $@.tmp $@
	@$(ECHO) "Tests finished `date`" >> $@
	$(TAIL) -n 6 $@

# don't include git directories in distribution
dist-hook:
	$(RM) -r `$(FIND) $(distdir) -name .git -type d`

help-local:
	@$(ECHO) "Local make targets:" ; \
	$(ECHO) "    all: build all enabled targets" ; \
	$(ECHO) "    check: run tests" ; \
	$(ECHO) "    distcheck: checks self-containedness of source tree (important)" ; \
	$(ECHO) "    distcheck.log: logs distcheck" ; \
	$(ECHO) "    install: installs built targets (recursive)" ; \
	$(ECHO) "    installcheck: checks install (recursive)" ; \
	$(ECHO) "    uninstall: uninstalls this package (recursive)" ; \
	$(ECHO) "    distclean: removes configure targets and cleans (recursive)" ; \
	$(ECHO) "    maintainer-clean: removes bootstrap targets and distcleans" ; \
	$(ECHO) "    libtool: regenerates the configured libtool script"

include $(top_srcdir)/globals.mk

