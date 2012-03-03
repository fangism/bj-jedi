# Makefile

SHELL = /bin/sh
MAKE = make

THIS_DIR = src
TAR_DIR = boc2

ARCH_FLAGS =
# for amusement
# ARCH_FLAGS += -arch ppc
# ARCH_FLAGS += -arch i386
CXX = g++
# CXX = clang++
CXXCPPFLAGS = -I. -I/sw/include
# CXXCPPFLAGS = -I. -I/sw64/include
CXXFLAGS = -g -O3 -Wall -W -Wshadow -Werror -Wno-unused -ansi -pedantic-errors $(ARCH_FLAGS)
LINK = ld -r
CXXLD = $(CXX)
# LD = ld
LDFLAGS = $(ARCH_FLAGS) -L/sw/lib
# LDFLAGS = $(ARCH_FLAGS) -L/sw64/lib
LIBS = -lreadline
TARGETS = boc2 bjswoc bjgrader
TARBALL = boc2.tar.gz

.SUFFIXES: .cc .o

.cc.o:
	$(CXX) $(CXXCPPFLAGS) $(CXXFLAGS) -c $< -o $@

all: $(TARGETS)

BLACKJACK_HEADERS = blackjack.hh card_state.hh deck.hh deck_state.hh enums.hh grader.hh hand.hh lobby.hh strategy.hh variation.hh

# for now, all objects depend on all headers
# eventually automake will take care of this
blackjack.o: $(BLACKJACK_HEADERS)
strategy.o: $(BLACKJACK_HEADERS)
variation.o: $(BLACKJACK_HEADERS)
grader.o: $(BLACKJACK_HEADERS)
# others don't change as oftem

libutil_OBJ = util/tokenize.o util/string.o util/readline_wrap.o
util/libutil.a: $(libutil_OBJ)
	$(LINK) $(LDFLAGS) $(libutil_OBJ) $(LIBS) -o $@

libbj_OBJ = deck.o card_state.o deck_state.o grader.o hand.o blackjack.o lobby.o variation.o strategy.o
libbj.a: $(libbj_OBJ)
	$(LINK) $(LDFLAGS) $(libbj_OBJ) -o $@

BOC_OBJ = boc2.o
BOC_LIBS = libbj.a util/libutil.a
boc2:	$(BOC_OBJ) $(BOC_LIBS)
	$(CXXLD) $(LDFLAGS) $(BOC_OBJ) $(BOC_LIBS) $(LIBS) -o $@
boc2.o: $(BLACKJACK_HEADERS)

BJSWOC_OBJ = bjswoc.o
BJSWOC_LIBS = $(BOC_LIBS)
bjswoc:	$(BJSWOC_OBJ) $(BOC_LIBS)
	$(CXXLD) $(LDFLAGS) $(BJSWOC_OBJ) $(BJSWOC_LIBS) $(LIBS) -o $@
bjswoc.o: $(BLACKJACK_HEADERS)

BJG_OBJ = bjgrader.o
BJG_LIBS = libbj.a util/libutil.a
bjgrader: $(BJG_OBJ) $(BJG_LIBS)
	$(CXXLD) $(LDFLAGS) $(BJG_OBJ) $(BJG_LIBS) $(LIBS) -o $@
bjgrader.o: $(BLACKJACK_HEADERS) util/command.tcc util/command.hh

clean:
	-rm -f *.a *.o
	-rm -f */*.a */*.o
	rm -f $(TARBALL)

clobber: clean
	-rm -f $(TARGETS)

dist: tarball
tarball: clobber
	-rm -f $(TARBALL) ; \
	cd .. && \
	rm -f $(TAR_DIR) && \
	ln -fs $(THIS_DIR) $(TAR_DIR) && \
	tar chzvf $(TARBALL) $(TAR_DIR)

cvsdiffs:
	-cvs diff -u > $@

.PHONY: all clean clobber tarball cvsdiffs

