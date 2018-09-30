#
# Makefile for SRI-LM man pages
# 
# $Header: /home/srilm/devel/man/RCS/Makefile,v 1.5 1998/12/24 09:48:53 stolcke Exp $
#

GROFF = groff -Tascii -mgan

MAN1PAGES = \
	ngram \
	ngram-count \
	ngram-merge \
	disambig \
	nbest-lattice \
	nbest-mix \
	segment \
	segment-nbest \
	hidden-ngram

MAN3PAGES = \
	Prob \
	Vocab \
	LM \
	File
	
CATS = $(MAN1PAGES:%=cat1/%.1) \
	$(MAN3PAGES:%=cat3/%.3)

HTMLS = $(MAN1PAGES:%=html/%.html) \
	$(MAN3PAGES:%=html/%.html)

all:	cats htmls index

cats:	$(CATS)

index:	whatis windex

windex whatis:	$(CATS)
	catman -w -M .

htmls:	$(HTMLS)

cat1/%.1:	man1/%.1
	$(GROFF) $< >$@
cat3/%.3:	man3/%.3
	$(GROFF) $< >$@
	
#html/%.html:	cat1/%.1
#	rman -f HTML -r '%s.html' $< >$@ 
html/%.html:	man1/%.1
	scripts/man2html $< >$@ 
html/%.html:	man3/%.3
	scripts/man2html $< >$@ 

clean:
	rm -f $(CATS) $(HTMLS)
