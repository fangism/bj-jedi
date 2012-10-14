# "src/Makefile.am"

AUTOMAKE_OPTIONS = 1.9 gnu subdir-objects

bin_PROGRAMS = apps/boc2 apps/bjswoc apps/bjgrader \
	apps/bj-dealer-odds
noinst_PROGRAMS = apps/bj-splitgen
pkglib_LTLIBRARIES = util/libutil.la \
	bj/core/libbjcore.la \
	bj/ui/stdio/libbjstdio.la

AM_CPPFLAGS = $(RL_INCLUDE)

bj_core_libbjcore_la_SOURCES = \
	config.h \
	bj/devel_switches.hh \
	bj/core/analysis.cc bj/core/analysis.hh \
	bj/core/blackjack.cc bj/core/blackjack.hh \
	bj/core/card_state.cc bj/core/card_state.hh \
	bj/core/counter.cc bj/core/counter.hh \
	bj/core/deck.cc bj/core/deck.hh \
	bj/core/deck_state.cc bj/core/deck_state.hh \
	bj/core/enums.hh \
	bj/core/expectations.cc bj/core/expectations.hh \
	bj/core/hand.cc bj/core/hand.hh \
	bj/core/num.hh \
	bj/core/outcome.cc bj/core/outcome.hh \
	bj/core/player_action.cc bj/core/player_action.hh \
	bj/core/statistics.cc bj/core/statistics.hh \
	bj/core/strategy.cc bj/core/strategy.hh \
	bj/core/variation.cc bj/core/variation.hh

# TODO: readline function only needed with CLI library
bj_core_libbjcore_la_LIBADD = util/libutil.la

bj_ui_stdio_libbjstdio_la_SOURCES = \
	bj/ui/stdio/bookmark.cc bj/ui/stdio/bookmark.hh \
	bj/ui/stdio/grader.cc bj/ui/stdio/grader.hh \
	bj/ui/stdio/play_options.cc bj/ui/stdio/play_options.hh \
	bj/ui/stdio/lobby.cc bj/ui/stdio/lobby.hh
bj_ui_stdio_libbjstdio_la_LIBADD = bj/core/libbjcore.la

util_libutil_la_SOURCES = \
	util/attributes.h \
	util/algorithm.hh \
	util/array.hh \
	util/array.tcc \
	util/command.hh \
	util/command.tcc \
	util/configure_option.hh \
	util/cppcat.h \
	util/getopt_mapped.hh \
	util/getopt_mapped.tcc \
	util/getopt_mapped.cc \
	util/getopt_portable.h \
	util/iosfmt_saver.hh \
	util/member_select_iterator.hh \
	util/probability.hh \
	util/probability.tcc \
	util/readline.h \
	util/readline_wrap.cc \
	util/readline_wrap.hh \
	util/stacktrace.cc \
	util/stacktrace.hh \
	util/string.cc \
	util/string.hh \
	util/string.tcc \
	util/tokenize.cc \
	util/tokenize.hh \
	util/value_saver.hh
# needs readline
util_libutil_la_LDFLAGS = $(RL_LDPATH)
util_libutil_la_LIBADD = $(RL_LIB)

apps_boc2_SOURCES = apps/boc2.cc
apps_boc2_LDADD = bj/core/libbjcore.la

apps_bjswoc_SOURCES = apps/bjswoc.cc
apps_bjswoc_LDADD = bj/core/libbjcore.la

apps_bjgrader_SOURCES = apps/bjgrader.cc
apps_bjgrader_LDADD = bj/ui/stdio/libbjstdio.la

apps_bj_splitgen_SOURCES = apps/bj-splitgen.cc
apps_bj_splitgen_LDADD = bj/core/libbjcore.la

apps_bj_dealer_odds_SOURCES = apps/bj-dealer-odds.cc
apps_bj_dealer_odds_LDADD = bj/core/libbjcore.la

include $(top_srcdir)/globals.mk
