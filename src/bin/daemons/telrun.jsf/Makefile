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

pass4:	install

install:

installjsf: telrun postprocess pipeline calimagePV
	rm -f $(TELHOME)/bin/telrun
	cp telrun $(TELHOME)/bin/telrun
	rm -f $(TELHOME)/bin/postprocess
	cp postprocess $(TELHOME)/bin/postprocess
	cp pipeline $(TELHOME)/bin/pipeline
	cp calimagePV $(TELHOME)/bin/calimagePV

pass4jsf-halfm: installjsf
pass4jsf-vir: installjsf
pass5jsf-halfm: installjsf
pass5jsf-vir: installjsf
pass5jsf-onem: installjsf

postprocess: postprocess.csh
	cp postprocess.csh postprocess

pipeline: pipeline.pl
	cp pipeline.pl pipeline

calimagePV: calimagePV.pl
	cp calimagePV.pl calimagePV

clobber:
	touch x.o
	rm -f *.o telrun postprocess pipeline calimagePV

