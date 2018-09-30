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
#    $Header: /home/srilm/devel/lm/src/RCS/Makefile,v 1.54 2000/07/13 06:16:50 stolcke Exp $
#

DECIPHER = $(SRILM)
#MACHINE_TYPE := $(shell $(DECIPHER)/bin/machine-type)

# Include common DECIPHER (TM) variable definitions.
include $(DECIPHER)/common/Makefile.common.variables

ifneq ($(CC), $(subst gcc,,$(CC)))
CC += -Wreturn-type 
endif

ifneq ($(CXX), $(subst g++,,$(CXX)))
CXX += -Wreturn-type -DINSTANTIATE_TEMPLATES
endif

# This should enable locale-specific string collation for vocabulary sorting
# (it will slow things down somewhat).
#ADDITIONAL_CXXFLAGS = -Dstrcmp=strcoll

# Use SArray to save space (default is LHash for speed)
#ADDITIONAL_CXXFLAGS += -DUSE_SARRAY_TRIE -DUSE_SARRAY
#LIBSUFFIX=-fast

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
	$(SRCDIR)/SubVocab.h \
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
	$(SRCDIR)/BayesMix.h \
	$(SRCDIR)/AdaptiveMix.h \
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
	$(SRCDIR)/SubVocab.cc \
	$(SRCDIR)/TextStats.cc \
	$(SRCDIR)/LM.cc \
	$(SRCDIR)/LMStats.cc \
	$(SRCDIR)/RefList.cc \
	$(SRCDIR)/NBest.cc \
	$(SRCDIR)/NBestSet.cc \
	$(SRCDIR)/Ngram.cc \
	$(SRCDIR)/NgramStatsInt.cc \
	$(SRCDIR)/NgramStatsFloat.cc \
	$(SRCDIR)/NgramStatsDouble.cc \
	$(SRCDIR)/Discount.cc \
	$(SRCDIR)/ClassNgram.cc \
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
	$(SRCDIR)/BayesMix.cc \
	$(SRCDIR)/AdaptiveMix.cc \
	$(SRCDIR)/CacheLM.cc \
	$(SRCDIR)/DynamicLM.cc \
	$(SRCDIR)/HMMofNgrams.cc \
	$(SRCDIR)/WordAlign.cc \
	$(SRCDIR)/WordLattice.cc \
	$(SRCDIR)/WordMesh.cc \
	$(SRCDIR)/simpleTrigram.cc \
	$(TEMPLATE_SOURCES)

# Example Library object files.
LIB_OBJECTS = $(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,\
		$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SOURCES)))

# Example Library.
LIBRARY = $(OBJDIR)/liboolm$(LIBSUFFIX).a

ADDITIONAL_INCLUDES += \
	-I/usr/local/include 

ADDITIONAL_LDFLAGS += \
	-u matherr

OTHER_LIBRARIES = \
	-ldstruct \
	-lmisc
ADDITIONAL_LIBRARIES += \
	$(OTHER_LIBRARIES) \
	-L/usr/local/lib -ltcl \
	-lm

# Example programs.

TEST_PROGRAM_NAMES = \
	testProb \
	testParseFloat \
	testVocab  \
	testTaggedVocab \
	hoeffding \
	tolower \
	testLattice \
	testError \
	testNBest \
	testMix

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
	segment \
	segment-nbest \
	hidden-ngram

PROGRAM_NAMES = \
	$(TEST_PROGRAM_NAMES) \
	$(REAL_PROGRAM_NAMES)

PROGRAMS = $(PROGRAM_NAMES:%=$(BINDIR)/%)

PROGRAM_SOURCES = $(foreach prog,$(PROGRAM_NAMES),\
			$(wildcard $(SRCDIR)/$(prog).c) \
			$(wildcard $(SRCDIR)/$(prog).cc))
PROGRAM_OBJECTS = $(PROGRAM_NAMES:%=$(OBJDIR)/%.o)


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
	$(ARCHIVE) $@ $^ $(DEMANGLE_FILTER)
	$(RANLIB) $@ $(DEMANGLE_FILTER)

$(PROGRAMS): $(LIBRARY) $(OTHER_LIBRARIES)

# Variables and Targets for released system

EXPORTED_HEADERS   = $(EXTERNAL_LIB_HEADERS)
EXPORTED_LIBRARIES = $(LIBRARY)
EXPORTED_PROGRAMS  = $(REAL_PROGRAM_NAMES:%=$(BINDIR)/%)

release:	release-headers release-libraries release-programs

# Include common DECIPHER (TM) target definitions.
include $(DECIPHER)/common/Makefile.common.targets
