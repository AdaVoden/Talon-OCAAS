CC	= gcc
CFLAGS	= -O3 -Wall

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

NAME	= libfli

SRC	= \
  libfli.c

ALL	= $(NAME).a

JUNK	= *.aux *.log *.out example.dxx

.PHONY: all
all: $(ALL)

%.a: $(SRC:.c=.o)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

example: example.c $(NAME).a
	$(CC) $(CFLAGS) $(INC) $< -o $@ -L. -lfli

.PHONY: htmldoc
htmldoc: $(NAME).dxx $(SRC) example.dxx
	$(DOCXX) $(DOCHTML) $<

dvidoc: $(NAME).dvi
psdoc: $(NAME).ps
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

%.dxx: %.dxx.m4
	$(M4) $< > $@

depend: $(DEPEND)

$(DEPEND): $(SRC)
	$(CC) $(CFLAGS) $(INC) -M $(SRC) > $@

.PHONY: clean
clean:
	rm -f $(NAME).o $(NAME).a example $(JUNK)

-include $(DEPEND)
