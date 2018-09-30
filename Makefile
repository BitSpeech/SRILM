#
# Top-level Makefile for SRILM
#
# $Header: /home/srilm/devel/RCS/Makefile,v 1.21 2003/01/03 18:34:48 stolcke Exp $
#

# SRILM = /home/speech/stolcke/project/srilm/devel
MACHINE_TYPE := $(shell $(SRILM)/bin/machine-type)

MODULES = \
	misc \
	dstruct \
	lm \
	lattice \
	utils \
	htk 

EXCLUDE = \
	me \
	fngram \
	lm/src/test \
	utils/src/fsmtest \
	test/output \
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

init depend all programs release clean cleaner cleanest \
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
	find . \( -name "*.~[0-9]*" -o -name core \) -print; \
	find . -name RCS -print) | \
	sed 's,^\./,,' > $@

force:

