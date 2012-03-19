# "test/Makefile.am"

AUTOMAKE_OPTIONS = 1.9 gnu

SUFFIXES = .bj-in .bj-out .bj-diff .bj-exp .bj-test

BJ_EXE = ../src/bjgrader

SHELL_INTERPRETER = "\#!$(SHELL)"

.bj-in.bj-out:
	$(BJ_EXE) < $< > $@

.bj-out.bj-diff:
	@exp=$(srcdir)/`echo $@ | $(SED) 's|\.bj-diff|.bj-exp|'` && \
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


TEST_CASES = defaults basic-strategy-all

TESTS = $(TEST_CASES:=.bj-test)

TEST_OUTPUTS = $(TEST_CASES:=.bj-out)

TEST_DIFFS = \
	$(TEST_CASES:=.bj-diff) \
	$(TEST_CASES:=.bj-test)

$(TEST_OUTPUTS): $(BJ_EXE)

CLEANFILES = $(TEST_OUTPUTS) $(TEST_DIFFS)

keep-files: $(TEST_OUTPUTS) $(TEST_DIFFS)

EXTRA_DIST = \
	$(TEST_CASES:=.bj-in) \
	$(TEST_CASES:=.bj-exp)

-include test.autodepend
test.autodepend: Makefile
	for f in $(TEST_CASES) ; \
	do $(ECHO) "$$f.bj-diff: $$f.bj-exp" ; \
	done > $@

DISTCLEANFILES = test.autodepend

