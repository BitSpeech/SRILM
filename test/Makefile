#
# Makefile for SRILM test suite 
#
# $Header: /home/srilm/devel/test/RCS/Makefile,v 1.8 2005/09/30 23:13:26 stolcke Exp $
#

TEST=*

all:
	@for testdir in tests/$(TEST); do \
		./go.run-test $$testdir; \
	done

profile:
	@PATH=$$SRILM/lm/bin/$${MACHINE_TYPE}_p:$$SRILM/lattice/bin/$${MACHINE_TYPE}_p$$SRILM/flm/bin/$${MACHINE_TYPE}_p:$$SRILM/bin/$$MACHINE_TYPE:$$PATH; export PATH; \
	for testdir in tests/$(TEST); do \
		./go.run-test $$testdir; \
	done

pure:
	@PATH=$$SRILM/lm/bin/$${MACHINE_TYPE}_pure:$$SRILM/lattice/bin/$${MACHINE_TYPE}_pure:$SRILM/flm/bin/$${MACHINE_TYPE}_pure:$$SRILM/bin/$$MACHINE_TYPE:$$PATH; export PATH; \
	for testdir in tests/$(TEST); do \
		./go.run-test $$testdir; \
	done

try:
	@PATH=$$SRILM/lm/bin/$${MACHINE_TYPE}${OPTION}:$$SRILM/lattice/bin/$${MACHINE_TYPE}${OPTION}:$$SRILM/flm/bin/$${MACHINE_TYPE}${OPTION}:$$SRILM/utils/src:$$SRILM/bin/$$MACHINE_TYPE:$$PATH; export PATH; \
	for testdir in tests/$(TEST); do \
		./go.run-test $$testdir; \
	done

try-output:
	@PATH=$$SRILM/lm/bin/$${MACHINE_TYPE}${OPTION}:$$SRILM/lattice/bin/$${MACHINE_TYPE}${OPTION}:$$SRILM/flm/bin/$${MACHINE_TYPE}${OPTION}:$$SRILM/utils/src:$$SRILM/bin/$$MACHINE_TYPE:$$PATH; export PATH; \
	for testdir in tests/$(TEST); do \
		(cd tests/$(TEST); sh -x ./run-test); \
	done

refs:
	[ ! -d reference -o -d reference.old ] || mv reference reference.old
	mkdir -p reference
	@for testdir in tests/$(TEST); do \
		./go.update-refs $$testdir; \
	done

clean:
	rm -f output/*
	find tests \( -name core -o -name gmon.out \) -print | xargs rm -f
