CC = cc
CLDFLAGS = 
CFLAGS = $(CLDFLAGS) -O -I../.. -I../../../../libastro
LDFLAGS = $(CLDFLAGS)
LIBS = -lm

# Try using these libs for SVR4.
# LIBS = -lm -lsocket -lnsl

OBJS = gscload.o support.o ../../gsc.o

gscload: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:38 $ $Revision: 1.1.1.1 $ $Name:  $
