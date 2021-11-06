# Makefile for telrun

IS =	-I../../libfits \
	-I../../libmisc \
	-I../../libastro

CLDFLAGS =
CFLAGS = $(CLDFLAGS) -O2 -Wall -ffast-math $(IS)
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lmisc -lastro -lfits -lm

OBJS =				\
	telrun.o		\
	fifoio.o		\
	fileio.o		\
	pr_bias.o		\
	pr_thermal.o		\
	pr_flat.o		\
	pr_regscan.o

telrun: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass3:
	rm -f buildcfg.h
	touch buildcfg.h

jsf-version:
	rm -f buildcfg.h
	echo "#define JSF_VERSION 1" > buildcfg.h

pass3jsf-onem: jsf-version
pass3jsf-halfm: jsf-version
pass3jsf-vir: jsf-version

pass4jsf-halfm:
	cp pipeline $(TELHOME)/bin
	cp pipeline.cfg $(TELHOME)/archive/config

pass4jsf-vir:
pass4jsf-onem:
	cp calimagePV $(TELHOME)/bin

pass4:	install

install: telrun postprocess
	rm -f $(TELHOME)/bin/telrun
	cp telrun $(TELHOME)/bin/telrun
	rm -f $(TELHOME)/bin/postprocess
	cp postprocess $(TELHOME)/bin/postprocess

postprocess: postprocess.csh
	cp postprocess.csh postprocess

clobber:
	touch x.o
	rm -f *.o telrun postprocess

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:00 $ $Revision: 1.1.1.1 $ $Name:  $
