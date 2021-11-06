NAME	= libfli
VERSION	= 0.3

CC	= gcc
CFLAGS	= -g -O3 -Wall -DVERSION=$(VERSION)

INC	= -I../driver

AR	= ar
ARFLAGS	= -rus

DOCDIR	= doc
DOCXX	= doc++
DOCHTML	= --dir $(DOCDIR)
DOCTEX	= --tex

LATEX	= latex
DVIPS	= dvips
PDFLATEX= pdflatex

M4	= m4

DEPEND	= .depend

SRC	=		\
  libfli.c

JUNK	= *.aux *.log *.out

ALL	= $(NAME).a
.PHONY: all
all: $(ALL)

DOC     = htmldoc pdfdoc
.PHONY: doc
doc: $(DOC)

PKGFILES=		\
  Makefile		\
  README		\
  docxx.sty		\
  example.c		\
  example.dxx.m4	\
  fli.h			\
  libfli.c		\
  libfli.dxx		\
  libfli.h		\
  doc			\
  libfli.pdf

PKGDIR  = $(NAME)-$(VERSION)
PACKAGE = $(PKGDIR).tgz

.PHONY: package
package: doc
	mkdir $(PKGDIR)
	cp -r $(PKGFILES) $(PKGDIR)
	tar -cvzf $(PACKAGE) $(PKGDIR)
	rm -rf $(PKGDIR)

%.a: $(SRC:.c=.o)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

example: example.c $(NAME).a
	$(CC) $(CFLAGS) $(INC) $< -o $@ -L. -lfli

.PHONY: htmldoc
htmldoc: $(NAME).dxx $(SRC) example.dxx
	$(DOCXX) $(DOCHTML) $<
	rm -f $(JUNK)

.PHONY: dvidoc
dvidoc: $(NAME).dvi

.PHONY: psdoc
psdoc: $(NAME).ps

.PHONY: pdfdoc
pdfdoc: $(NAME).pdf

%.tex: %.dxx $(SRC) example.dxx
	$(DOCXX) $(DOCTEX) --output $(NAME).tex $<

%.dvi: %.tex
	$(LATEX) $<; $(LATEX) $<
	rm -f $(JUNK)

%.ps: %.dvi
	$(DVIPS) -o $@ $<
	rm -f $(JUNK)

%.pdf: %.tex
	$(PDFLATEX) $<; $(PDFLATEX) $<
	rm -f $(JUNK)

.PHONY: %.dxx
%.dxx: %.dxx.m4
	$(M4) $< > $@

depend: $(DEPEND)

.PHONY: $(DEPEND)
$(DEPEND): $(SRC)
	$(CC) $(CFLAGS) $(INC) -M $(SRC) > $@

.PHONY: clean
clean:
	rm -f $(DEPEND) $(NAME).o $(NAME).a example $(JUNK) $(NAME).dvi \
	      $(NAME).ps $(NAME).pdf example.dxx
	rm -rf $(DOCDIR)
	rm -rf $(PACKAGE) $(PKGDIR)

-include $(DEPEND)
