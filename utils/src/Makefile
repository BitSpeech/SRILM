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
#    $Header: /home/srilm/devel/utils/src/RCS/Makefile,v 1.24 1998/09/03 07:27:56 stolcke Exp $
#

DECIPHER = $(SRILM)
#MACHINE_TYPE := $(shell $(DECIPHER)/bin/machine-type)

# Include common DECIPHER (TM) variable definitions.
include $(DECIPHER)/common/Makefile.common.variables

# Example Library declarations files.

SCRIPTS = \
	rescore-nbest \
	rescore-nbest-pass1 \
	rescore-nbest-pass2 \
	rescore-nbest-pass3 \
	wordlat-to-boogie \
	extract-skip-probs \
	$(EXPORTED_SCRIPTS)

EXPORTED_SCRIPTS = \
	bo-subset \
	add-dummy-bows \
	make-sub-lm \
	change-lm-vocab \
	add-ppls \
	subtract-ppls \
	hits-from-log \
	ppl-from-log \
	rescore-decipher \
	rescore-acoustic \
	combine-acoustic-scores \
	rescore-reweight \
	rescore-minimize-wer \
	nbest2-to-nbest1 \
	get-gt-counts \
	make-gt-discounts \
	vp2text \
	compute-oov-rate \
	pfsg-to-dot \
	wlat-to-pfsg \
	wlat-to-dot \
	pfsg-to-fsm \
	fsm-to-pfsg \
	make-hiddens-lm \
	make-batch-counts \
	merge-batch-counts \
	continuous-ngram-count \
	make-big-lm \
	compare-ppls \
	compute-best-mix \
	make-diacritic-map \
	bytelog-to-log10 \
	log10-to-bytelog \
	remove-lowprob-ngrams \
	make-ngram-pfsg \
	add-pauses-to-pfsg \
	add-classes-to-pfsg \
	replace-words-with-classes \
	pfsg-from-ngram \
	nbest-error \
	make-nbest-pfsg \
	sentid-to-sclite \
	compute-sclite \
	compare-sclite

all:	

release:	release-scripts

# Include common DECIPHER (TM) target definitions.
include $(DECIPHER)/common/Makefile.common.targets

