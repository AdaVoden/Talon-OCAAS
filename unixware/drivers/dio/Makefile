# Makefile for CIO-DIO 8255 I/O on SysVr4.2 UNIX 386

SHELL = /bin/sh
CCDEFINES = `test -r /etc/conf/cf.d/defines && cat /etc/conf/cf.d/defines; exit 0`
INCLSYS = /usr/include/sys
MODNAME = dio
# TARGETDIR = -R test_install.d

CFLAGS = -O $(CCDEFINES)

OBJS = $(MODNAME).o

# idinstall takes away Driver.o even if use -M and -k
installx:	Space.c Driver.o System Master Node
	mv Driver.o x.Driver.o
	cp x.Driver.o Driver.o
	/etc/conf/bin/idinstall -M $(TARGETDIR) $(MODNAME)
	mv x.Driver.o Driver.o

build:	install
	cd /; /etc/conf/bin/idbuild -B

Driver.o:		$(MODNAME).c $(INCLSYS)/$(MODNAME).h
	$(CC) $(CFLAGS) -c $(MODNAME).c
	mv $(MODNAME).o Driver.o


$(INCLSYS)/$(MODNAME).h:	$(MODNAME).h
	cat $(MODNAME).h > $(INCLSYS)/$(MODNAME).h

clobber:
	rm -f $(MODNAME).o

install:

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $
