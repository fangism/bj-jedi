# Makefile

SHELL = /bin/sh
MAKE = make

THIS_DIR = src
TAR_DIR = boc2

ARCH_FLAGS =
# for amusement
# ARCH_FLAGS += -arch ppc
CXX = g++
CXXCPPFLAGS = -I.
CXXFLAGS = -O3 -Wall -W -Werror -ansi -pedantic-errors $(ARCH_FLAGS)
LD = $(CXX)
# LD = ld
LDFLAGS = $(ARCH_FLAGS)
TARGETS = boc2
TARBALL = boc2.tar.gz

.SUFFIXES: .cc .o

.cc.o:
	$(CXX) $(CXXCPPFLAGS) $(CXXFLAGS) -c $< -o $@

all: $(TARGETS)

BOC_OBJ = boc2.o card_state.o blackjack.o
boc2:	$(BOC_OBJ)
	$(LD) $(LDFLAGS) $(BOC_OBJ) -o $@

clean:
	-rm -f *.o
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

