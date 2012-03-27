# "text/Makefile.am"

SUFFIXES =
include $(srcdir)/Make.latex

pdfdir = $(docdir)/pdf
dist_pdf_DATA = bj-math.pdf

BOOK_INPUTS = bj-math.tex intro.tex basic.tex counting.tex bj-math.bib
EXTRA_DIST = $(BOOK_INPUTS)

BASIC_STRATEGY = ../test/basic-strategy-all.bj-out

$(BASIC_STRATEGY): ../src/bjgrader
	+pushd `dirname $@` && $(MAKE) `basename $@` && popd && test -f $@

# dependencies on auto-generated files
bj-math.pdf: $(BOOK_INPUTS) tables/win-lose-push.tex

TABS_TO_TABULAR = $(SED) -e 's|[	]|\&|g' -e 's|$$| \\\\ \\hline|' -e '1s|$$| \\hline|'

tables/win-lose-push.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) "\\\\begin{tabular}{|c||c|c|c|c|c|c|c|c|}\n\\hline\n" ; \
	$(SED) -n '/final state outcomes/,/^$$/p' $< | $(SED) -e '1d' -e '$$d' | \
		$(SED) -e '1s|P.D|$$\\frac{\\textrm{dealer}\\rightarrow}{\\textrm{player}\\downarrow}$$|' -e '2s|<=|$$&$$|' | \
		$(TABS_TO_TABULAR) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

-include bj-math.pdfdepend

