#
# Top-level Makefile for SRILM-lite
#
# $Header: /home/srilm/lite/RCS/Makefile,v 1.7 2012/12/25 14:21:44 stolcke Exp $
#

SRILM = /home/srilm/lite
MACHINE_TYPE := $(shell $(SRILM)/sbin/machine-type)

RELEASE := $(shell cat RELEASE)

# Include common SRILM variable definitions.
include $(SRILM)/common/Makefile.common.variables

PACKAGE_DIR = ..

MODULES = \
	misc \
	dstruct \
	lm \
	flm

EXCLUDE = \
	common/COMPILE-HOSTS \
	misc/src/SRILMversion.h \
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
	$(MAKE) release-scripts

depend-all:	dirs release-headers
	gawk '{ print $$1, $$2, $$3 }' common/COMPILE-HOSTS | sort -u | \
	while read prog host type; do \
		set -x; $$prog $$host "cd $(SRILM); $(MAKE) DECIPHER=$(DECIPHER) MACHINE_TYPE=$$type OPTION=$$option init depend" < /dev/null; \
	done

compile-all:	dirs
	cat common/COMPILE-HOSTS | \
	while read prog host type option; do \
		set -x; $$prog $$host "cd $(SRILM); $(MAKE) DECIPHER=$(DECIPHER) MACHINE_TYPE=$$type OPTION=$$option init release-libraries release-programs" < /dev/null; \
	done

dirs:
	-mkdir include lib bin

init depend all programs release clean cleaner cleanest sanitize desanitize \
release-headers release-libraries release-programs release-scripts:
	for subdir in $(MODULES); do \
		(cd $$subdir/src; $(MAKE) $(MAKE_VARS) $@) || exit 1; \
	done

test try gzip:
	for subdir in $(MODULES); do \
		[ ! -d $$subdir/test ] || \
		(cd $$subdir/test; $(MAKE) $(MAKE_VARS) $@) || exit 1; \
	done

# files needed for the web page
WWW_DOCS = CHANGES License INSTALL RELEASE
WWW_DIR = /home/spftp/www/DocumentRoot/projects/srilm

www:	$(WWW_DOCS)
	ginstall -m 444 $(WWW_DOCS) $(WWW_DIR)/docs
	ginstall -m 444 man/html/* $(WWW_DIR)/manpages

TAR = /usr/local/gnu/bin/tar

package:	$(PACKAGE_DIR)/EXCLUDE
	(cd misc/src; $(MAKE) $(MAKE_VARS) $(VERSION_HEADER))
	$(TAR) cvhzXf $(PACKAGE_DIR)/EXCLUDE $(PACKAGE_DIR)/srilm-$(RELEASE).tar.gz .
	rm $(PACKAGE_DIR)/EXCLUDE

package_notest:	$(PACKAGE_DIR)/EXCLUDE
	echo test >> $(PACKAGE_DIR)/EXCLUDE
	$(TAR) cvhzXf $(PACKAGE_DIR)/EXCLUDE $(PACKAGE_DIR)/srilm-$(RELEASE)-notest.tar.gz .
	rm $(PACKAGE_DIR)/EXCLUDE

package_bin:	$(PACKAGE_DIR)/EXCLUDE-$(MACHINE_TYPE)
	$(TAR) cvhzXf $(PACKAGE_DIR)/EXCLUDE-$(MACHINE_TYPE) $(PACKAGE_DIR)/srilm-$(RELEASE)-$(MACHINE_TYPE).tar.gz $(INFO) include lib man bin sbin
	rm -f $(PACKAGE_DIR)/EXCLUDE-$(MACHINE_TYPE)

package_x:
	$(MAKE) $(MAKE_VARS) sanitize
	$(MAKE) $(MAKE_VARS) RELEASE=$(RELEASE)_x package
	$(MAKE) $(MAKE_VARS) desanitize

$(PACKAGE_DIR)/EXCLUDE:	force
	rm -f DONE
	(find bin/* lib/* */bin/* */obj/* */test/output -follow -type d -print -prune ; \
	find $(EXCLUDE) include bin -print; \
	find . -follow \( -name Makefile.site.\* -o -name "*.~[0-9]*" -o -name ".#*" -o -name Dependencies.\* -o -name core -o -name "core.[0-9]*" -o -name \*.3rdparty -o -name .gdb_history -o -name out.\* -o -name "*[._]pure[._]*" -o -type l -o -name RCS -o -name CVS -o -name .cvsignore -o -name GZ.files \) -print) | \
	sed 's,^\./,,' > $@

$(PACKAGE_DIR)/EXCLUDE-$(MACHINE_TYPE):	$(PACKAGE_DIR)/EXCLUDE
	fgrep -l /bin/sh bin/* > $(PACKAGE_DIR)/EXCLUDE-shell
	fgrep -v -f $(PACKAGE_DIR)/EXCLUDE-shell $(PACKAGE_DIR)/EXCLUDE | \
	egrep -v 'include|^bin$$|$(MACHINE_TYPE)[^~]*$$' > $@
	-egrep '$(MACHINE_TYPE).*[._]pure[._]' $(PACKAGE_DIR)/EXCLUDE >> $@
	-egrep '$(MACHINE_TYPE)_[gp]' $(PACKAGE_DIR)/EXCLUDE >> $@
	rm -f $(PACKAGE_DIR)/EXCLUDE-shell

force:

