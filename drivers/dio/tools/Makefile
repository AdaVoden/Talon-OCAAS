CLDFLAGS=
CFLAGS=$(CLDFLAGS) -I../../../libmisc
LDFLAGS=$(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lmisc -lastro -lfits -lm

dio: dio.o
	$(CC) $(LDFLAGS) -o $@ $? $(LIBS)

install: dio
	rm -fr $(TELHOME)/bin/dio
	cp dio $(TELHOME)/bin/dio

pass8batf: install

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $
