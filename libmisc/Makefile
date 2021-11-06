CFLAGS= -O2 -ffast-math -fPIC -Wall -I../libastro -I../libfits 

OBJS =			\
	catalogs.o 	\
	ccdcamera.o 	\
	cliserv.o 	\
	configfile.o 	\
	crackini.o	\
	csimc.o 	\
	libfli.o	\
        focustemp.o     \
	funcmax.o	\
	gaussfit.o 	\
	lstsqr.o 	\
	misc.o 		\
	newton.o	\
	photstd.o 	\
	rot.o 		\
	running.o 	\
	scan.o 		\
	strops.o 	\
	telaxes.o	\
	telenv.o	\
	telfits.o	\
	tts.o

libfli.o:	../drivers/fli/lib/libfli.o
	rm -f $@
	ln -s $? $@

ccdcamera.c: fli.h

fli.h:	../drivers/fli/lib/fli.h
	rm -f $@
	ln -s $? $@

libmisc.so:	$(OBJS)
	$(CC) -shared -o $@ $(OBJS)

pass2: install

install: libmisc.so
	rm -f $(TELHOME)/lib/libmisc.so
	cp libmisc.so $(TELHOME)/lib/libmisc.so

libmisc.a:	$(OBJS)
	ar r $@ $(OBJS)

clobber:
	touch x.o
	rm -f *.o libmisc.so

# build test program version of telaxes
# CFLAGS= -O2 -ffast-math -Wall -I../libastro -I../libfits $(MOTIFI) -DTEST_IT
# LDFLAGS = -L. -L../libastro $(MOTIFL)
# LIBS = -lmisc -lastro -lm $(MOTIFLIBS)
# telaxes: telaxes.o
# 	$(CC) $(LDFLAGS) -o $@ telaxes.o $(LIBS)

# test program for catbrowse.c
LDFLAGS = -L. -L../libastro $(MOTIFL)
LIBS = -lmisc -lastro -lm $(MOTIFLIBS)

catbrowsemain: catbrowsemain.o
	$(CC) $(LDFLAGS) -o $@ catbrowsemain.o $(LIBS)
	./catbrowsemain

catbrowsemain.o: catbrowse.c
	$(CC) $(CFLAGS) -DTESTIT -o catbrowsemain.o -c catbrowse.c

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:17 $ $Revision: 1.1.1.1 $ $Name:  $
