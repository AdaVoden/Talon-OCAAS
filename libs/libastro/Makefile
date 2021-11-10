# Makefile for the basic astronomy routines.

CC = gcc
CFLAGS= -O2 -ffast-math -Wall

OBJS =	\
	aa_hadec.o \
	aberration.o \
	actan.o \
	airmass.o \
	anomaly.o \
	ap_as.o \
	auxil.o \
	chap95.o \
	chap95_data.o \
	circum.o \
	comet.o \
	dbfmt.o \
	deep.o \
	deltat.o \
	earthsat.o \
	eq_ecl.o \
	eq_gal.o \
	formats.o \
	helio.o \
	libration.o \
	misc.o \
	mjd.o \
	moon.o \
	mooncolong.o \
	nutation.o \
	obliq.o \
	parallax.o \
	plans.o \
	precess.o \
	reduce.o \
	refract.o \
	riset.o \
	riset_cir.o \
	sdp4.o \
	sgp4.o \
	sphcart.o \
	sun.o \
	thetag.o \
	utc_gst.o \
	vsop87.o \
	vsop87_data.o

libastro.a:	$(OBJS)
	ar rv $@ $(OBJS)

libastro.so:	$(OBJS)
	gcc -shared -o $@ $(OBJS)

pass2: install

install: libastro.so
	rm -f $(TELHOME)/lib/libastro.so
	cp libastro.so $(TELHOME)/lib/libastro.so

clobber:
	rm -f $(OBJS) libastro.a libastro.so

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $
