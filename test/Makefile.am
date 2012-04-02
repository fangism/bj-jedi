# "test/Makefile.am"

AUTOMAKE_OPTIONS = 1.9 gnu

SUFFIXES = .bj-in .bj-out .bj-diff .bj-exp .bj-test

BJ_EXE = ../src/bjgrader

SHELL_INTERPRETER = "\#!$(SHELL)"

.bj-in.bj-out:
	$(BJ_EXE) < $< > $@

.bj-out.bj-diff:
	@exp=$(srcdir)/`echo $@ | $(SED) 's|\.bj-diff|.bj-exp|'` && \
	$(RM) $@ && \
	test -f $$exp || { $(ECHO) "$$exp not found." ; exit 1 ;} && \
	$(ECHO) "$(DIFF) -u $$exp $< > $@" && \
	$(DIFF) -u $$exp $< > $@ || :

.bj-diff.bj-test:
	@$(ECHO) "Summarizing $@ ..." && \
	{ $(ECHO) $(SHELL_INTERPRETER) && \
	$(ECHO) "# \"$@\"" && \
	if test -s $< ; then \
	  $(ECHO) "$(ECHO) $< is non-empty!" && \
	  $(ECHO) "exit 1" ; \
	fi ;} > $@ && \
	$(CHMOD) +x $@


TEST_CASES = defaults \
	session-000 session-001 session-002 session-003 session-004 \
	session-005 session-006 session-007 session-008 session-009 \
	session-basic-000 session-basic-001 session-basic-002 \
	session-basic-003 session-basic-004 session-basic-005 \
	session-basic-006 session-basic-007 session-basic-008 \
	session-basic-009 \
	basic-strategy-all \
	basic-6to5-all \
	basic-S17-all \
	basic-no-surr-all \
	basic-noDAS-all \
	dynamic-strategy-1-less-2 dynamic-strategy-1-more-2 \
	dynamic-strategy-1-less-3 dynamic-strategy-1-more-3 \
	dynamic-strategy-1-less-4 dynamic-strategy-1-more-4 \
	dynamic-strategy-1-less-5 dynamic-strategy-1-more-5 \
	dynamic-strategy-1-less-6 dynamic-strategy-1-more-6 \
	dynamic-strategy-1-less-7 dynamic-strategy-1-more-7 \
	dynamic-strategy-1-less-8 dynamic-strategy-1-more-8 \
	dynamic-strategy-1-less-9 dynamic-strategy-1-more-9 \
	dynamic-strategy-1-less-T dynamic-strategy-1-more-T \
	dynamic-strategy-1-less-A dynamic-strategy-1-more-A
# need to re-analyze/fix:
#	basic-hit-split-As-all
#	basic-nosplit-all
#	basic-resplit-all
#	basic-resplit-aces-all

TESTS = $(TEST_CASES:=.bj-test)

TEST_OUTPUTS = $(TEST_CASES:=.bj-out)

TEST_DIFFS = \
	$(TEST_CASES:=.bj-diff) \
	$(TEST_CASES:=.bj-test)

test-summaries: force
	-cat *.bj-diff > $@

$(TEST_OUTPUTS): $(BJ_EXE)

CLEANFILES = $(TEST_OUTPUTS) $(TEST_DIFFS)

keep-files: $(TEST_OUTPUTS) $(TEST_DIFFS)

EXTRA_DIST = \
	$(TEST_CASES:=.bj-in) \
	$(TEST_CASES:=.bj-exp) \
	update-tests.sh

-include test.autodepend
test.autodepend: Makefile
	for f in $(TEST_CASES) ; \
	do $(ECHO) "$$f.bj-diff: $$f.bj-exp" ; \
	done > $@

DISTCLEANFILES = test.autodepend

include $(top_srcdir)/globals.mk

