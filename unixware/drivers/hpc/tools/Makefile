CLDFLAGS =
CFLAGS = $(CLDFLAGS) -I../../../libastro -O
LDFLAGS = $(CLDFLAGS) -L../../../libastro
LIBS = -lastro 
OBJS = hpctest.o

all:	hpctest hpctrace hpctemp

hpctest: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

hpctrace: hpctrace.o
	$(CC) $(LDFLAGS) -o $@ hpctrace.o

hpctemp: hpctemp.o
	$(CC) $(LDFLAGS) -o $@ hpctemp.o


# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $
