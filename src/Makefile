# Makefile

SHELL = /bin/sh
MAKE = make

THIS_DIR = src
TAR_DIR = boc2

ARCH_FLAGS =
# for amusement
# ARCH_FLAGS += -arch ppc
CXX = g++
# CXX = clang++
CXXCPPFLAGS = -I. -I/sw/include
CXXFLAGS = -g -O3 -Wall -W -Wshadow -Werror -ansi -pedantic-errors $(ARCH_FLAGS)
LINK = ld -r
CXXLD = $(CXX)
# LD = ld
LDFLAGS = -L/sw/lib
LIBS = -lreadline
TARGETS = boc2 bjswoc bjgrader
TARBALL = boc2.tar.gz

.SUFFIXES: .cc .o

.cc.o:
	$(CXX) $(CXXCPPFLAGS) $(CXXFLAGS) -c $< -o $@

all: $(TARGETS)

blackjack.o: blackjack.hh card_state.hh

libutil_OBJ = util/tokenize.o util/string.o util/readline_wrap.o
util/libutil.a: $(libutil_OBJ)
	$(LINK) $(LDFLAGS) $(libutil_OBJ) $(LIBS) -o $@

libbj_OBJ = card_state.o blackjack.o
libbj.a: $(libbj_OBJ)
	$(LINK) $(LDFLAGS) $(libbj_OBJ) -o $@

BOC_OBJ = boc2.o
BOC_LIBS = libbj.a util/libutil.a
boc2:	$(BOC_OBJ) $(BOC_LIBS)
	$(CXXLD) $(LDFLAGS) $(BOC_OBJ) $(BOC_LIBS) $(LIBS) -o $@
boc2.o: blackjack.hh card_state.hh

BJSWOC_OBJ = bjswoc.o
BJSWOC_LIBS = $(BOC_LIBS)
bjswoc:	$(BJSWOC_OBJ) $(BOC_LIBS)
	$(CXXLD) $(LDFLAGS) $(BJSWOC_OBJ) $(BJSWOC_LIBS) $(LIBS) -o $@
bjswoc.o: blackjack.hh card_state.hh

BJG_OBJ = bjgrader.o
BJG_LIBS = libbj.a util/libutil.a
bjgrader: $(BJG_OBJ) $(BJG_LIBS)
	$(CXXLD) $(LDFLAGS) $(BJG_OBJ) $(BJG_LIBS) $(LIBS) -o $@
bjgrader.o: blackjack.hh card_state.hh util/command.tcc util/command.hh

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

