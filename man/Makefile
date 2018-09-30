#
# Makefile for SRI-LM man pages
# 
# $Header: /home/srilm/devel/man/RCS/Makefile,v 1.15 2003/03/12 08:41:10 stolcke Exp stolcke $
#

GROFF = groff -Tascii -mgan

MAN1PAGES = \
	ngram \
	ngram-count \
	ngram-class \
	ngram-merge \
	disambig \
	nbest-lattice \
	nbest-optimize \
	nbest-mix \
	nbest-pron-score \
	segment \
	segment-nbest \
	hidden-ngram \
	anti-ngram \
	multi-ngram \
	lattice-tool \
	training-scripts \
	lm-scripts \
	ppl-scripts \
	pfsg-scripts \
	nbest-scripts \
	select-vocab

MAN3PAGES = \
	Prob \
	Vocab \
	LM \
	File

MAN5PAGES = \
	ngram-format \
	nbest-format \
	classes-format \
	pfsg-format \
	wlat-format

CATS = $(MAN1PAGES:%=cat1/%.1) \
	$(MAN3PAGES:%=cat3/%.3)\
	$(MAN5PAGES:%=cat5/%.5)

HTMLS = $(MAN1PAGES:%=html/%.html) \
	$(MAN3PAGES:%=html/%.html) \
	$(MAN5PAGES:%=html/%.html)

all:	cats htmls index

cats:	$(CATS)

index:	whatis windex

windex: $(CATS)
	rm -f windex
	scripts/makewhatis .

whatis:	$(CATS)
	rm -f whatis
	scripts/makewhatis.bsd .

htmls:	$(HTMLS)

cat1/%.1:	man1/%.1
	$(GROFF) $< >$@
cat3/%.3:	man3/%.3
	$(GROFF) $< >$@
cat5/%.5:	man5/%.5
	$(GROFF) $< >$@
	
#html/%.html:	cat1/%.1
#	rman -f HTML -r '%s.html' $< >$@ 
html/%.html:	man1/%.1
	scripts/man2html $< >$@ 
html/%.html:	man3/%.3
	scripts/man2html $< >$@ 
html/%.html:	man5/%.5
	scripts/man2html $< >$@ 

clean:
	rm -f $(CATS) $(HTMLS)
