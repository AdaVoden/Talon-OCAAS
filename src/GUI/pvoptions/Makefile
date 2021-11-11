# Makefile for pvoptions.

CLDFLAGS =
CFLAGS = $(CLDFLAGS) -Wall -ffast-math -O2 -I../../libmisc -I../../libXmisc $(MOTIFI)
LDFLAGS = -s $(CLDFLAGS) -L$(TELHOME)/lib $(MOTIFL)
LIBS = -lastro -lmisc -lfits -lXmisc -lm $(MOTIFLIBS)

OBJS =	pvoptions.o \
	msg.o \
	onemgui.o \
	halfmgui.o \
	rwcfg.o \
	tips.o

pvoptions: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass3:	
	rm -f buildcfg.h
	touch buildcfg.h

pass3jsf-halfm:
	echo "#define HALFM 1" >> buildcfg.h
	echo "#define ONEM 0" >> buildcfg.h

pass3jsf-onem:
	echo "#define HALFM 0" >> buildcfg.h
	echo "#define ONEM 1" >> buildcfg.h

pass6jsf-halfm:	install

pass6jsf-onem: install

install: pvoptions 
	rm -f $(TELHOME)/bin/pvoptions
	cp pvoptions $(TELHOME)/bin/pvoptions

clobber:
	touch x.o
	rm -f *.o pvoptions 

# For CVS Only -- Do Not Edit
# @(#) $Id: Makefile,v 1.1.1.1 2003/04/15 20:47:03 lostairman Exp $
