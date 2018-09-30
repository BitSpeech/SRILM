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
#    $Header: /home/srilm/devel/lm/src/RCS/Makefile,v 1.78 2006/01/09 22:47:54 stolcke Exp $
#

# Include common SRILM variable definitions.
include $(SRILM)/common/Makefile.common.variables

# This should enable locale-specific string collation for vocabulary sorting
# (it will slow things down somewhat).
#ADDITIONAL_CXXFLAGS = -Dstrcmp=strcoll

# Flags for generating "compact" data structures
COMPACT_FLAGS += -DUSE_SARRAY -DUSE_SARRAY_TRIE -DUSE_SARRAY_MAP2

# Flags for generating "short" data structures
SHORT_FLAGS = $(COMPACT_FLAGS) -DUSE_SHORT_VOCAB -DUSE_SHORT_COUNTS

# Define variables.

TEMPLATE_SOURCES = \
	$(SRCDIR)/NgramStats.cc \
	$(SRCDIR)/Trellis.cc

# Example Library declarations files.

EXTERNAL_LIB_HEADERS = \
	$(SRCDIR)/Prob.h \
	$(SRCDIR)/XCount.h \
	$(SRCDIR)/Vocab.h \
	$(SRCDIR)/VocabMap.h \
	$(SRCDIR)/VocabMultiMap.h \
	$(SRCDIR)/VocabDistance.h \
	$(SRCDIR)/SubVocab.h \
	$(SRCDIR)/MultiwordVocab.h \
	$(SRCDIR)/TextStats.h \
	$(SRCDIR)/LM.h \
	$(SRCDIR)/LMStats.h \
	$(SRCDIR)/RefList.h \
	$(SRCDIR)/NBest.h \
	$(SRCDIR)/NBestSet.h \
	$(SRCDIR)/Ngram.h \
	$(SRCDIR)/NgramStats.h \
	$(SRCDIR)/Discount.h \
	$(SRCDIR)/ClassNgram.h \
	$(SRCDIR)/SimpleClassNgram.h \
	$(SRCDIR)/DFNgram.h \
	$(SRCDIR)/SkipNgram.h \
	$(SRCDIR)/HiddenNgram.h \
	$(SRCDIR)/HiddenSNgram.h \
	$(SRCDIR)/VarNgram.h \
	$(SRCDIR)/DecipherNgram.h \
	$(SRCDIR)/TaggedVocab.h \
	$(SRCDIR)/TaggedNgram.h \
	$(SRCDIR)/TaggedNgramStats.h \
	$(SRCDIR)/StopNgram.h \
	$(SRCDIR)/StopNgramStats.h \
	$(SRCDIR)/NullLM.h \
	$(SRCDIR)/MultiwordLM.h \
	$(SRCDIR)/BayesMix.h \
	$(SRCDIR)/LoglinearMix.h \
	$(SRCDIR)/AdaptiveMix.h \
	$(SRCDIR)/AdaptiveMarginals.h \
	$(SRCDIR)/CacheLM.h \
	$(SRCDIR)/DynamicLM.h \
	$(SRCDIR)/HMMofNgrams.h \
	$(SRCDIR)/Trellis.h \
	$(SRCDIR)/WordAlign.h \
	$(SRCDIR)/MultiAlign.h \
	$(SRCDIR)/WordLattice.h \
	$(SRCDIR)/WordMesh.h \
	$(TEMPLATE_SOURCES)

INTERNAL_LIB_HEADERS = 



# Example Library source files.
LIB_SOURCES = 				\
	$(SRCDIR)/matherr.c \
	$(SRCDIR)/Prob.cc \
	$(SRCDIR)/XCount.cc \
	$(SRCDIR)/Vocab.cc \
	$(SRCDIR)/VocabMap.cc \
	$(SRCDIR)/VocabMultiMap.cc \
	$(SRCDIR)/VocabDistance.cc \
	$(SRCDIR)/SubVocab.cc \
	$(SRCDIR)/MultiwordVocab.cc \
	$(SRCDIR)/TextStats.cc \
	$(SRCDIR)/LM.cc \
	$(SRCDIR)/LMStats.cc \
	$(SRCDIR)/RefList.cc \
	$(SRCDIR)/NBest.cc \
	$(SRCDIR)/NBestSet.cc \
	$(SRCDIR)/NgramLM.cc \
	$(SRCDIR)/NgramStatsInt.cc \
	$(SRCDIR)/NgramStatsShort.cc \
	$(SRCDIR)/NgramStatsFloat.cc \
	$(SRCDIR)/NgramStatsDouble.cc \
	$(SRCDIR)/NgramStatsXCount.cc \
	$(SRCDIR)/Discount.cc \
	$(SRCDIR)/ClassNgram.cc \
	$(SRCDIR)/SimpleClassNgram.cc \
	$(SRCDIR)/DFNgram.cc \
	$(SRCDIR)/SkipNgram.cc \
	$(SRCDIR)/HiddenNgram.cc \
	$(SRCDIR)/HiddenSNgram.cc \
	$(SRCDIR)/VarNgram.cc \
	$(SRCDIR)/DecipherNgram.cc \
	$(SRCDIR)/TaggedVocab.cc \
	$(SRCDIR)/TaggedNgram.cc \
	$(SRCDIR)/TaggedNgramStats.cc \
	$(SRCDIR)/StopNgram.cc \
	$(SRCDIR)/StopNgramStats.cc \
	$(SRCDIR)/MultiwordLM.cc \
	$(SRCDIR)/BayesMix.cc \
	$(SRCDIR)/LoglinearMix.cc \
	$(SRCDIR)/AdaptiveMix.cc \
	$(SRCDIR)/AdaptiveMarginals.cc \
	$(SRCDIR)/CacheLM.cc \
	$(SRCDIR)/DynamicLM.cc \
	$(SRCDIR)/HMMofNgrams.cc \
	$(SRCDIR)/WordAlign.cc \
	$(SRCDIR)/WordLattice.cc \
	$(SRCDIR)/WordMesh.cc \
	$(SRCDIR)/simpleTrigram.cc \
	$(TEMPLATE_SOURCES)

# Example Library object files.
LIB_OBJECTS = $(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%$(OBJ_SUFFIX),\
		$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%$(OBJ_SUFFIX),$(LIB_SOURCES)))

# Example Library.
LIBRARY = $(OBJDIR)/$(LIB_PREFIX)oolm$(LIB_SUFFIX)

ADDITIONAL_INCLUDES += \
	$(TCL_INCLUDE)

ADDITIONAL_LDFLAGS += \
	$(MATHERR_LINK)

ADDITIONAL_LIBRARIES += \
	$(SRILM_LIBDIR)/$(LIB_PREFIX)flm$(LIB_SUFFIX) \
	$(SRILM_LIBDIR)/$(LIB_PREFIX)dstruct$(LIB_SUFFIX) \
	$(SRILM_LIBDIR)/$(LIB_PREFIX)misc$(LIB_SUFFIX) \
	$(TCL_LIBRARY) \
	$(MATH_LIBRARY)

# Example programs.

TEST_PROGRAM_NAMES = \
	testHash \
	testProb \
	testXCount \
	testParseFloat \
	testVocabDistance \
	testNgram \
	hoeffding \
	tolower \
	testLattice \
	testError \
	testNBest \
	testMix
ifeq ($(NO_TCL), )
TEST_PROGRAM_NAMES += \
	testTaggedVocab \
	testVocab 
endif

REAL_PROGRAM_NAMES = \
	ngram \
	ngram-count \
	ngram-merge \
	ngram-class \
	disambig \
	anti-ngram \
	nbest-lattice \
	nbest-mix \
	nbest-optimize \
	nbest-pron-score \
	segment \
	segment-nbest \
	hidden-ngram \
	multi-ngram

PROGRAM_NAMES = \
	$(TEST_PROGRAM_NAMES) \
	$(REAL_PROGRAM_NAMES)

PROGRAMS = $(PROGRAM_NAMES:%=$(BINDIR)/%$(EXE_SUFFIX))

PROGRAM_SOURCES = $(foreach prog,$(PROGRAM_NAMES),\
			$(wildcard $(SRCDIR)/$(prog).c) \
			$(wildcard $(SRCDIR)/$(prog).cc))
PROGRAM_OBJECTS = $(PROGRAM_NAMES:%=$(OBJDIR)/%$(OBJ_SUFFIX))


# Libraries to be linked with the Example programs.
LIBRARIES = $(LIBRARY) \
	    $(ADDITIONAL_LIBRARIES)

# All of the types of files.

ALL_HEADERS = $(EXTERNAL_LIB_HEADERS) \
	      $(INTERNAL_LIB_HEADERS)

ALL_SOURCES = $(LIB_SOURCES) \
	      $(PROGRAM_SOURCES)

ALL_OBJECTS = $(LIB_OBJECTS) \
	      $(PROGRAM_OBJECTS)

ALL_LIBRARIES = $(LIBRARY)

ALL_PROGRAMS = $(PROGRAMS)

ALL_PROGRAM_NAMES = $(PROGRAM_NAMES)


# Define pseudo-targets.


# Make sure the library does not get deleted if the make is interrupted.
.PRECIOUS:	$(LIBRARY)


# Define targets.


all:	$(PROGRAMS)

$(LIBRARY):	$(LIB_OBJECTS)
	$(ARCHIVE) $(AR_OUTPUT_OPTION) $^ $(DEMANGLE_FILTER)
	$(RANLIB) $@ $(DEMANGLE_FILTER)

$(PROGRAMS): $(LIBRARY) $(OTHER_LIBRARIES)

# Variables and Targets for released system

EXPORTED_HEADERS   = $(EXTERNAL_LIB_HEADERS)
EXPORTED_LIBRARIES = $(LIBRARY)
EXPORTED_PROGRAMS  = $(REAL_PROGRAM_NAMES:%=$(BINDIR)/%$(EXE_SUFFIX))

release:	release-headers release-libraries release-programs

# Include common SRILM target definitions.
include $(SRILM)/common/Makefile.common.targets
