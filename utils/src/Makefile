#
#    File:   Makefile.example
#    Author: The SRI DECIPHER (TM) System
#    Date:   Thu Sep  9 12:04:47 1993
#
#    Description:
#	This is the example makefile to start from when adding new
#       modules to the DECIPHER System.  To use this makefile, first
#       copy it to your directory as the file "Makefile".  Second,
#       replace the word "Example" in the text below with the real name
#	of your library.  Next replace the the example filenames with
#	the names of your actual declarations and source files in the
#	appropriate variable definitions.  Finally clean up by deleting
#	any lines not relevant to your module and updating this header
#       to describe your new module.  Do not forget to use the proper
#       RCS keywords!
#
#    Copyright (c) 1993, SRI International.  All Rights Reserved.
#
#    $Header: /home/srilm/devel/utils/src/RCS/Makefile,v 1.47 2003/02/20 15:17:37 stolcke Exp $
#

# Include common SRILM variable definitions.
include $(SRILM)/common/Makefile.common.variables

# Example Library declarations files.

SCRIPTS = \
	rescore-nbest \
	wordlat-to-lisp \
	extract-skip-probs \
	$(EXPORTED_SCRIPTS)

EXPORTED_SCRIPTS = \
	change-lm-vocab \
	rescore-decipher \
	rescore-acoustic \
	rescore-reweight \
	rescore-minimize-wer \
	make-batch-counts \
	merge-batch-counts \
	make-big-lm \
	make-multiword-pfsg \
	pfsg-from-ngram \
	nbest-error \
	nbest-rover \
	align-with-tags \
	compute-sclite \
	compare-sclite

# gawk scripts that need to be edited before installation
GAWK_SCRIPTS = \
	add-classes-to-pfsg \
	add-dummy-bows \
	add-pauses-to-pfsg \
	add-ppls \
	bytelog-to-log10 \
	classes-to-fsm \
	combine-acoustic-scores \
	compare-ppls \
	compute-best-mix \
	compute-best-rover-mix \
	compute-best-sentence-mix \
	compute-oov-rate \
	continuous-ngram-count \
	extract-skip-probs \
	find-reference-posteriors \
	fsm-to-pfsg \
	get-gt-counts \
	hits-from-log \
	log10-to-bytelog \
	make-abs-discount \
	make-diacritic-map \
	make-gt-discounts \
	make-kn-discounts \
	make-kn-counts \
	make-hiddens-lm \
	make-lm-subset \
	make-nbest-pfsg \
	make-ngram-pfsg \
	make-sub-lm \
	sort-lm \
	reverse-lm \
	merge-nbest \
	nbest-posteriors \
	nbest2-to-nbest1 \
	nbest-vocab \
	pfsg-to-dot \
	pfsg-to-fsm \
	pfsg-vocab \
	ppl-from-log \
	remove-lowprob-ngrams \
	replace-words-with-classes \
	reverse-text \
	reverse-ngram-counts \
	sentid-to-sclite \
	sentid-to-ctm \
	subtract-ppls \
	uniform-classes \
	vp2text \
	wlat-to-dot \
	wlat-to-pfsg \
	wlat-stats \
	wordlat-to-lisp \
	prettify

EXPORTED_PROGRAMS  = $(GAWK_SCRIPTS:%=$(BINDIR)/%)

all:	

release:	release-scripts release-programs

# Include common SRILM target definitions.
include $(SRILM)/common/Makefile.common.targets

#
# Rule to create edited gawk script 
#
$(BINDIR)/%: $(SRCDIR)/%.gawk $(BINDIR_STAMP)
	sed -e '1s,/usr/local/bin/gawk,$(GAWK),' $< >$@.new
	mv $@.new $@

