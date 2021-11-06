install: libXm.so.2.1
	cp libXm.so.2.1 $(TELHOME)/lib
	rm -f $(TELHOME)/lib/libXm.so.2
	ln -s libXm.so.2.1 $(TELHOME)/lib/libXm.so.2
	rm -f $(TELHOME)/lib/libXm.so
	ln -s libXm.so.2 $(TELHOME)/lib/libXm.so

pass2: install

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:41 $ $Revision: 1.1.1.1 $ $Name:  $
