# "text/Makefile.am"

SUFFIXES =
include $(srcdir)/Make.latex

pdfdir = $(docdir)/pdf
dist_pdf_DATA = bj-math.pdf

BOOK_INPUTS = bj-math.tex intro.tex basic.tex counting.tex bj-math.bib
EXTRA_DIST = $(BOOK_INPUTS) scantexdepend.awk state-machine-tabular.awk \
	transpose-matrix.awk scale.awk

BASIC_STRATEGY = ../test/basic-strategy-all.bj-out
S17_STRATEGY = ../test/basic-S17-all.bj-out

EVAL_STRATEGY_SCRIPT = pushd `dirname $@` && $(MAKE) `basename $@` && popd && test -f $@

$(BASIC_STRATEGY): ../src/bjgrader
	+$(EVAL_STRATEGY_SCRIPT)
$(S17_STRATEGY): ../src/bjgrader
	+$(EVAL_STRATEGY_SCRIPT)

GENERATED_TABLES = \
	tables/win-lose-push.tex \
	tables/dealer-hit-H17.tex \
	tables/dealer-hit-S17.tex \
	tables/dealer-final-H17-pre-peek.tex \
	tables/dealer-final-H17-post-peek.tex \
	tables/dealer-final-S17-pre-peek.tex \
	tables/dealer-final-S17-post-peek.tex \
	tables/player-hit.tex
CLEANFILES = $(GENERATED_TABLES)

tables: $(GENERATED_TABLES)
clean-local: clean-tables
clean-tables:
	rm -f $(GENERATED_TABLES)

# dependencies on auto-generated files
bj-math.pdf: $(BOOK_INPUTS) $(GENERATED_TABLES)

# use 'tr'?
TABS_TO_TABULAR = $(SED) -e 's|[	]|\&|g' -e 's|$$| \\\\ \\hline|'
TABULAR_HEADER_HLINE = $(SED) -e '1s|$$| \\hline|'
SM_TO_TABULAR = $(AWK) -f $(srcdir)/state-machine-tabular.awk
TRANSPOSE = $(AWK) -f $(srcdir)/transpose-matrix.awk
SCALE_PERCENT = $(AWK) -f $(srcdir)/scale.awk -v scalefactor=100

# how to generate each table
tables/win-lose-push.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) "\\\\begin{tabular}{|c||c|c|c|c|c|c|c|c|}\n\\hline\n" ; \
	$(SED) -n '/final state outcomes/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SED) -e '1s|P.D|$$\\frac{\\textrm{dealer}\\rightarrow}{\\textrm{player}\\downarrow}$$|' -e '2s|<=|$$&$$|' | \
		$(TABS_TO_TABULAR) | $(TABULAR_HEADER_HLINE) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

tables/dealer-hit-H17.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{next card}\\rightarrow}{\\textrm{dealer}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Dealer.*action table/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SM_TO_TABULAR) | $(TABULAR_HEADER_HLINE) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

tables/dealer-hit-S17.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{next card}\\rightarrow}{\\textrm{dealer}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Dealer.*action table/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SM_TO_TABULAR) | $(TABULAR_HEADER_HLINE) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

tables/player-hit.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{next card}\\rightarrow}{\\textrm{player}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Player.*hit transition/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SM_TO_TABULAR) | $(TABULAR_HEADER_HLINE) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

SCALE_DEALER_FINAL = $(SCALE_PERCENT)

tables/dealer-final-H17-pre-peek.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{reveal}\\rightarrow}{\\textrm{end state}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Dealer.*final state odds.*pre-peek/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

tables/dealer-final-H17-post-peek.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{reveal}\\rightarrow}{\\textrm{end state}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Dealer.*final state odds.*post-peek/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

tables/dealer-final-S17-pre-peek.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{reveal}\\rightarrow}{\\textrm{end state}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Dealer.*final state odds.*pre-peek/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

tables/dealer-final-S17-post-peek.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) '\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n' ; \
	$(PRINTF) '$$\\frac{\\textrm{reveal}\\rightarrow}{\\textrm{end state}\\downarrow}$$&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n' ; \
	$(SED) -n '/Dealer.*final state odds.*post-peek/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(PRINTF) "\\\\end{tabular}\n" ;} > $@

-include bj-math.pdfdepend

