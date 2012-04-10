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
	tables/player-hit.tex \
	tables/player-final-split.tex \
	tables/player-resplit.tex \
	tables/player-stand-H17-pre-peek.tex \
	tables/player-stand-H17-post-peek.tex \
	tables/player-stand-S17-pre-peek.tex \
	tables/player-stand-S17-post-peek.tex

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
REORDER_ACE_LAST_ROW = $(SED) -e '/^A/h' -e '/^A/d' -e '/^10/G'

# header for top-row: A,2,3,4,5,6,7,8,9,10
NEXT_CARD_TABULAR = \
	$(PRINTF) "\\\\begin{tabular}{|c||c|c|c|c|c|c|c|c|c|c|}\n\\hline\n"
NEXT_CARD_HEADER = \
	$(PRINTF) '&A&2&3&4&5&6&7&8&9&10 \\\\ \\hline \\hline\n'
# header for top-row: 2,3,4,5,6,7,8,9,10,A
DEALER_REVEAL_TABULAR = \
	$(PRINTF) "\\\\begin{tabular}{|c||c|c|c|c|c||c|c|c|c|c|}\n\\hline\n"
DEALER_REVEAL_HEADER = \
	$(PRINTF) '&2&3&4&5&6&7&8&9&10&A \\\\ \\hline \\hline\n'

END_TABULAR = $(PRINTF) "\\\\end{tabular}\n"

# how to generate each table
tables/win-lose-push.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(PRINTF) "\\\\begin{tabular}{|c||c|c|c|c|c|c|c|c|}\n\\hline\n" ; \
	$(SED) -n '/final state outcomes/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SED) -e '1s|P.D|$$\\frac{\\textrm{dealer}\\rightarrow}{\\textrm{player}\\downarrow}$$|' -e '2s|<=|$$&$$|' | \
		$(TABS_TO_TABULAR) | $(TABULAR_HEADER_HLINE) ; \
	$(END_TABULAR) ;} > $@

HOME_NEXT_CARD_DEALER = \
	$(PRINTF) '$$\\frac{\\textrm{next card}\\rightarrow}{\\textrm{dealer}\\downarrow}$$'
HOME_NEXT_CARD_PLAYER = \
	$(PRINTF) '$$\\frac{\\textrm{next card}\\rightarrow}{\\textrm{player}\\downarrow}$$'

tables/dealer-hit-H17.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(NEXT_CARD_TABULAR) ; \
	$(HOME_NEXT_CARD_DEALER) ; \
	$(NEXT_CARD_HEADER) ; \
	$(SED) -n '/Dealer.*action table/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SM_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/dealer-hit-S17.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(NEXT_CARD_TABULAR) ; \
	$(HOME_NEXT_CARD_DEALER) ; \
	$(NEXT_CARD_HEADER) ; \
	$(SED) -n '/Dealer.*action table/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SM_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/player-hit.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(NEXT_CARD_TABULAR) ; \
	$(HOME_NEXT_CARD_PLAYER) ; \
	$(NEXT_CARD_HEADER) ; \
	$(SED) -n '/Player.*hit transition/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(SM_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/player-final-split.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(NEXT_CARD_TABULAR) ; \
	$(HOME_NEXT_CARD_PLAYER) ; \
	$(NEXT_CARD_HEADER) ; \
	{ $(SED) -n '/Player.*hit transition/,/^$$/p' $< | \
		$(SED) -e '1d' -e '/^35/,$$d' ; \
	$(SED) -n '/Player.*split.*final.*transition/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(AWK) '{print $$1+35 "\t" $$2 "\t" $$3;}' ;} | \
		$(SM_TO_TABULAR) | $(TAIL) -n 10 ; \
	$(END_TABULAR) ;} > $@

tables/player-resplit.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(NEXT_CARD_TABULAR) ; \
	$(HOME_NEXT_CARD_PLAYER) ; \
	$(NEXT_CARD_HEADER) ; \
	{ $(SED) -n '/Player.*hit transition/,/^$$/p' $< | \
		$(SED) -e '1d' -e '/^35/,$$d' ; \
	$(SED) -n '/Player.*resplit.*transition/,/^$$/p' $< | \
		$(SED) -e '1d' -e '$$d' | \
		$(AWK) '{print $$1+35 "\t" $$2 "\t" $$3;}' ;} | \
		$(SM_TO_TABULAR) | $(TAIL) -n 10 ; \
	$(END_TABULAR) ;} > $@

SCALE_DEALER_FINAL = $(SCALE_PERCENT)
HOME_DEALER_FINAL = \
	$(PRINTF) '$$\\frac{\\textrm{reveal}\\rightarrow}{\\textrm{end state}\\downarrow}$$'

tables/dealer-final-H17-pre-peek.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_DEALER_FINAL) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Dealer.*final state odds.*pre-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/dealer-final-H17-post-peek.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_DEALER_FINAL) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Dealer.*final state odds.*post-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/dealer-final-S17-pre-peek.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_DEALER_FINAL) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Dealer.*final state odds.*pre-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/dealer-final-S17-post-peek.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_DEALER_FINAL) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Dealer.*final state odds.*post-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' | $(SCALE_DEALER_FINAL) | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

HOME_PLAYER_STAND = \
	$(PRINTF) '$$\\frac{\\textrm{reveal}\\rightarrow}{\\textrm{$$E[S]$$}\\downarrow}$$'

tables/player-stand-H17-pre-peek.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_PLAYER_STAND) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Player.*stand edges.*pre-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' -e 's|<=|$$&$$|g' | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/player-stand-H17-post-peek.tex: $(BASIC_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_PLAYER_STAND) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Player.*stand edges.*post-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' -e 's|<=|$$&$$|g' | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/player-stand-S17-pre-peek.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_PLAYER_STAND) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Player.*stand edges.*pre-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' -e 's|<=|$$&$$|g' | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@

tables/player-stand-S17-post-peek.tex: $(S17_STRATEGY)
	{ $(ECHO) "% \"$@\" -- auto-generated" ; \
	$(DEALER_REVEAL_TABULAR) ; \
	$(HOME_PLAYER_STAND) ; \
	$(DEALER_REVEAL_HEADER) ; \
	$(SED) -n '/Player.*stand edges.*post-peek/,/^$$/p' $< | \
		$(REORDER_ACE_LAST_ROW) | \
		$(SED) -e '1d' -e '$$d' -e 's|<=|$$&$$|g' | \
		$(TRANSPOSE) | $(SED) -e '1d' | $(TABS_TO_TABULAR) ; \
	$(END_TABULAR) ;} > $@


-include bj-math.pdfdepend

include $(top_srcdir)/globals.mk

