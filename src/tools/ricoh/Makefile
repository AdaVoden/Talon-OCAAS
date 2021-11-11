# this only gets the job done if run as root

CLDFLAGS=
CFLAGS= $(CLDFLAGS)
LDFLAGS= $(CLDFLAGS)
LIBS = -lgen

BIN = /usr/local/bin

all:	ricohmnt ricohumnt ricohfsck ricohfmt

installx: all
	rm -f $(BIN)/ricohmnt
	rm -f $(BIN)/ricohumnt
	rm -f $(BIN)/ricohfsck
	rm -f $(BIN)/ricohfmt
	mv ricohmnt $(BIN)
	rm -f ricohumnt
	rm -f ricohfsck
	rm -f ricohfmt
	ln $(BIN)/ricohmnt $(BIN)/ricohumnt
	ln $(BIN)/ricohmnt $(BIN)/ricohfsck 
	ln $(BIN)/ricohmnt $(BIN)/ricohfmt

ricohmnt: ricohmnt.o
	$(CC) $(LDFLAGS) -o $@ ricohmnt.o $(LIBS)
	chown root ricohmnt
	chmod 111 ricohmnt
	chmod u+s ricohmnt

ricohumnt: ricohmnt
	rm -f ricohumnt
	ln ricohmnt ricohumnt

ricohfsck: ricohmnt
	rm -f ricohfsck
	ln ricohmnt ricohfsck

ricohfmt: ricohmnt
	rm -f ricohfmt
	ln ricohmnt ricohfmt

install:

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $
