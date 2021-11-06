# Makefile for predict

CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro  -I../../libwcs -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm

OBJS= predict.o eclipses.o

predict: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5:	install

install: predict
	rm -f $(TELHOME)/bin/predict
	cp predict $(TELHOME)/bin/predict

clobber:
	touch x.o
	rm -f *.o predict

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:38 $ $Revision: 1.1.1.1 $ $Name:  $
