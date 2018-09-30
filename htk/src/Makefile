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
#    $Header: /home/srilm/devel/htk/src/RCS/Makefile,v 1.10 2006/01/09 22:47:54 stolcke Exp $
#

# Include common SRILM variable definitions.
include $(SRILM)/common/Makefile.common.variables

# Define variables.

# These paths work at Johns Hopkins CLSP as of 8/1/99
HTK_LIB_DIR = /apps/share/HTK_V2.1/HTKLib
HTK_LATTICE_DIR = /apps/share/LMTK

ifneq ($(wildcard $(HTK_LATTICE_DIR)), )

ifeq ($(MACHINE_TYPE),sparc)
HTK_CPU_TYPE = sun4
endif
ifeq ($(MACHINE_TYPE),sparc-elf)
HTK_CPU_TYPE = sun4_solaris
endif
ifeq ($(MACHINE_TYPE),mips-elf)
HTK_CPU_TYPE = sgi
endif
ifeq ($(MACHINE_TYPE),i686)
HTK_CPU_TYPE = linux
endif

SCRIPTS = \
	$(EXPORTED_SCRIPTS)

EXPORTED_SCRIPTS = \
	JAStar-restart

# Example Library declarations files.

EXTERNAL_LIB_HEADERS = 

INTERNAL_LIB_HEADERS = \
	$(HTK_LATTICE_DIR)/include.h \
	$(HTK_LATTICE_DIR)/Lattice.h \
	$(HTK_LATTICE_DIR)/NGram.h \
	$(HTK_LATTICE_DIR)/Interpolate.h \
	$(HTK_LATTICE_DIR)/MyLangModel.h \
	$(SRCDIR)/SriLM.h
	

# Example Library source files.
LIB_SOURCES =  \
	$(HTK_LATTICE_DIR)/Lattice.c \
	$(HTK_LATTICE_DIR)/NGram.c \
	$(HTK_LATTICE_DIR)/Interpolate.c \
	$(HTK_LATTICE_DIR)/MyLangModel.c \
	$(SRCDIR)/Language.c \
	$(SRCDIR)/SriLM.cc

# Example Library object files.
LIB_OBJECTS = $(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%$(OBJ_SUFFIX),\
		$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%$(OBJ_SUFFIX),\
		  $(patsubst $(HTK_LATTICE_DIR)/%.c,$(OBJDIR)/%$(OBJ_SUFFIX),\
							$(LIB_SOURCES))))

# Example Library.
LIBRARY = $(OBJDIR)/$(LIB_PREFIX)HTKlattice$(LIB_SUFFIX)

# Example programs.
PROGRAM_NAMES = \
	JLPrune \
	JLScore \
	JPerp \
	JAStar \
	JAStar \
	JLRebuild

PROGRAMS = $(PROGRAM_NAMES:%=$(BINDIR)/%$(EXE_SUFFIX))

PROGRAM_SOURCES = $(foreach prog,$(PROGRAM_NAMES),\
			$(wildcard $(SRCDIR)/$(prog).c) \
			$(wildcard $(SRCDIR)/$(prog).cc) \
			$(wildcard $(HTK_LATTICE_DIR)/$(prog).c))
PROGRAM_OBJECTS = $(PROGRAM_NAMES:%=$(OBJDIR)/%$(OBJ_SUFFIX))

ADDITIONAL_INCLUDES = \
	-I$(HTK_LATTICE_DIR) \
	-I$(HTK_LIB_DIR)

# Libraries to be linked with the Example programs.
# (include standard LM and helper libraries)
LIBRARIES = \
	$(LIBRARY) \
	$(ADDITIONAL_LIBRARIES)

# NOTE: Need to link $(LIBRARY) twice to satisfy references from HTK libs
ADDITIONAL_LIBRARIES = \
	$(HTK_LATTICE_DIR)/JLib.$(HTK_CPU_TYPE)$(LIB_SUFFIX) \
	$(HTK_LIB_DIR)/HTKLib.$(HTK_CPU_TYPE)$(LIB_SUFFIX) \
	$(HTK_LIB_DIR)/HTKLibHE.$(HTK_CPU_TYPE)$(LIB_SUFFIX) \
	$(LIBRARY) \
	$(SRILM_LIBDIR)/$(LIB_PREFIX)oolm$(LIB_SUFFIX) \
	$(SRILM_LIBDIR)/$(LIB_PREFIX)dstruct$(LIB_SUFFIX) \
	$(SRILM_LIBDIR)/$(LIB_PREFIX)misc$(LIB_SUFFIX)

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

endif

# Define targets.

all:	$(PROGRAMS)

$(LIBRARY):	$(LIB_OBJECTS)
	$(ARCHIVE) $(AR_OUTPUT_OPTION) $^ $(DEMANGLE_FILTER)
	$(RANLIB) $@ $(DEMANGLE_FILTER)


# Variables and Targets for released system

EXPORTED_HEADERS   = $(EXTERNAL_LIB_HEADERS)
#EXPORTED_LIBRARIES = $(LIBRARY)
EXPORTED_PROGRAMS  = $(PROGRAMS)

release:	release-headers \
		release-libraries \
		release-programs \
		release-scripts

# Include common SRILM target definitions.
include $(SRILM)/common/Makefile.common.targets

# Compile code in the HTK directory
# NOTE: place this after other rules so local sources override precompiled
# code.
$(OBJDIR)/%$(OBJ_SUFFIX): $(HTK_LATTICE_DIR)/%.c
	$(COMPILE.c) $(OPTION_FLAGS) $(OUTPUT_OPTION) $<

# Copy precompiled objects from HTK directory
# NOTE: place this after other rules so local sources override precompiled
# code.
$(OBJDIR)/%$(OBJ_SUFFIX):	$(HTK_LATTICE_DIR)/%.$(HTK_CPU_TYPE)$(OBJ_SUFFIX)
	cp $< $@

