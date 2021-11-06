
# $Id: Makefile,v 1.1.1.1 2003/04/15 20:48:33 lostairman Exp $ 

TELROOT = $(HOME)/telescope
CLDFLAGS = -ffast-math
CFLAGS = $(CLDFLAGS) -Wall -O2 -I/usr/lib -I$(TELROOT)/libmisc -I$(TELROOT)/libastro  
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib 
LIBS = -lfits -lmisc -lastro -lm 
DIR = $(TELHOME)/bin

ALL = focustemp 

OBJS = focustemp.o 

all: $(ALL)

pass5:	install

install: $(ALL)
	cp $(ALL) $(DIR)

focustemp: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clobber:
	touch x.o
	rm -f *.o $(ALL)

