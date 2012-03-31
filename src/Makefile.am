# "src/Makefile.am"

AUTOMAKE_OPTIONS = 1.9 gnu

bin_PROGRAMS = boc2 bjswoc bjgrader
pkglib_LTLIBRARIES = util/libutil.la libbj.la

AM_CPPFLAGS = $(RL_INCLUDE)

libbj_la_SOURCES = \
	blackjack.cc blackjack.hh \
	card_state.cc card_state.hh \
	config.h \
	deck.cc deck.hh \
	deck_state.cc deck_state.hh \
	enums.hh \
	expectations.cc expectations.hh \
	grader.cc grader.hh \
	hand.cc hand.hh \
	lobby.cc lobby.hh \
	play_options.cc play_options.hh \
	statistics.cc statistics.hh \
	strategy.cc strategy.hh \
	variation.cc variation.hh

libbj_la_LIBADD = util/libutil.la

util_libutil_la_SOURCES = \
	util/attributes.h \
	util/array.hh \
	util/array.tcc \
	util/command.hh \
	util/command.tcc \
	util/configure_option.hh \
	util/probability.hh \
	util/probability.tcc \
	util/readline.h \
	util/readline_wrap.cc \
	util/readline_wrap.hh \
	util/string.cc \
	util/string.hh \
	util/string.tcc \
	util/tokenize.cc \
	util/tokenize.hh \
	util/value_saver.hh
# needs readline
util_libutil_la_LDFLAGS = $(RL_LDPATH)
util_libutil_la_LIBADD = $(RL_LIB)

boc2_SOURCES = boc2.cc
boc2_LDADD = libbj.la

bjswoc_SOURCES = bjswoc.cc
bjswoc_LDADD = libbj.la

bjgrader_SOURCES = bjgrader.cc
bjgrader_LDADD = libbj.la

cvsdiffs:
	-cvs diff -u > $@

.PHONY: cvsdiffs

