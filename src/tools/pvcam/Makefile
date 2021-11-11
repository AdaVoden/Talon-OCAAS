CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -ffast-math -Wall 
LDFLAGS= $(CLDFLAGS)
LIBS =

lpio: lpio.o
	$(CC) $(LDFLAGS) -o $@ lpio.o $(LIBS)

vir-halfm: 
	 cp pvcam.virtual pvcam
	 cp pvcam.cfg-vir pvcam.cfg
	 rm -f $(TELHOME)/bin/pvcam
	 cp pvcam $(TELHOME)/bin/pvcam
	 cp pvcam.cfg $(TELHOME)/archive/config/pvcam.cfg
	 cp naming.pl $(TELHOME)/bin/naming.pl

pass5jsf-vir: vir-halfm

pass5jsf-halfm: lpio 
	cp pvcam.pl pvcam
	cp pvcam.cfg-halfm pvcam.cfg
	rm -f $(TELHOME)/bin/pvcam
	cp pvcam $(TELHOME)/bin/pvcam
	rm -f $(TELHOME)/bin/lpio
	cp lpio $(TELHOME)/bin/lpio
	cp naming.pl $(TELHOME)/bin/naming.pl
	cp createmesh $(TELHOME)/bin

pass8ajsf-halfm: lpio
	rm -f $(TELHOME)/bin/lpio
	cp lpio $(TELHOME)/bin/lpio
	chown root $(TELHOME)/bin/lpio
	chmod 555 $(TELHOME)/bin/lpio
	chmod u+s $(TELHOME)/bin/lpio

test1m:	
	cp pvc1m-mstr.pl $(TELHOME)/bin/pvmstr
	cp pvc1m-slave.pl $(TELHOME)/bin/pvslave_onem-1
	cp pvc1m-slave.pl $(TELHOME)/bin/pvslave_onem-2
	cp pvc1m-slave.pl $(TELHOME)/bin/pvslave_onem-3
	cp pvc1m-slave.pl $(TELHOME)/bin/pvslave_onem-4
	cp testgo.pl $(TELHOME)/bin/testgo
	cp skycam $(TELHOME)/bin/skycam
	cp pipeln1m.pl $(TELHOME)/bin/pipeln1m
	cp pvc1m.cfg.pl $(TELHOME)/archive/config/pvc1m.cfg
	cp pipeln1m.cfg.pl $(TELHOME)/archive/config/pipeln1m.cfg
	cp pvcam.cfg.1M.pl $(TELHOME)/archive/config/pvcam.cfg
	cp naming.pl $(TELHOME)/bin/naming.pl
	cp offsetfits.pl $(TELHOME)/bin/offsetfits
	cp addexpimg.pl $(TELHOME)/bin/addexpimg
	cp datareduce.pl $(TELHOME)/bin/datareduce
	
pass5jsf-onem: lpio
	cp pvc1m-mstr.pl $(TELHOME)/bin/pvmstr
	cp pvc1m-slave.pl $(TELHOME)/bin/pvslave
	cp skycam $(TELHOME)/bin/skycam
	cp pipeln1m.pl $(TELHOME)/bin/pipeln1m
	cp pvc1m.cfg.pl $(TELHOME)/archive/config/pvc1m.cfg
	cp pipeln1m.cfg.pl $(TELHOME)/archive/config/pipeln1m.cfg
	cp pvcam.cfg.1M.pl $(TELHOME)/archive/config/pvcam.cfg
	cp naming.pl $(TELHOME)/bin/naming.pl
	cp offsetfits.pl $(TELHOME)/bin/offsetfits
	cp addexpimg.pl $(TELHOME)/bin/addexpimg
	cp datareduce.pl $(TELHOME)/bin/datareduce
	cp createmesh $(TELHOME)/bin

pass8ajsf-onem: pass8ajsf-halfm	
	
clobber:
	touch x.o
	rm -f pvcam *.o lpio

install:

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:38 $ $Revision: 1.1.1.1 $ $Name:  $
