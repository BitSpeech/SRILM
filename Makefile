#
# Top-level Makefile for SRILM
#
# $Header: /home/srilm/devel/RCS/Makefile,v 1.13 1999/08/02 00:47:59 stolcke Exp $
#

# SRILM = /home/speech/stolcke/project/srilm/devel
MACHINE_TYPE := $(shell $(SRILM)/bin/machine-type)

MODULES = \
	misc \
	dstruct \
	lm \
	utils \
	htk

EXCLUDE = \
	me \
	lattice \
	lm/src/test \
	utils/src/fsmtest \
	EXCLUDE

MAKE_VARS = \
	SRILM=$(SRILM) \
	MACHINE_TYPE=$(MACHINE_TYPE) \
	OPTION=$(OPTION)

World:	dirs
	$(MAKE) init
	$(MAKE) release-headers
	$(MAKE) depend
	$(MAKE) release-libraries
	$(MAKE) release-programs

dirs:
	-mkdir include lib bin

init depend all release clean cleaner cleanest \
release-headers release-libraries release-programs:
	for subdir in $(MODULES); do \
		(cd $$subdir/src; $(MAKE) $(MAKE_VARS) $@); \
	done

# files needed for the web page
WWW_DOCS = CHANGES License INSTALL RELEASE
WWW_DIR = /home/spftp/www/DocumentRoot/projects/srilm

www:	$(WWW_DOCS)
	ginstall -m 444 $(WWW_DOCS) $(WWW_DIR)/docs
	ginstall -m 444 man/html/* $(WWW_DIR)/manpages

TAR = /usr/local/gnu/bin/tar-1.12

package:	EXCLUDE
	$(TAR) cvzXf EXCLUDE ../srilm-`cat RELEASE`.tar.gz .

EXCLUDE:	force
	(find $(EXCLUDE) bin/* lib/* */bin/* */obj/* \
		-type d -print -prune ; \
	find . -name RCS -print) | \
	sed 's,^\./,,' > $@

force:

