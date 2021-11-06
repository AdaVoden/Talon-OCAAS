CLDFLAGS = -Wall -O2
CFLAGS = $(CLDFLAGS) -I../../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lmisc -lastro -lfits -lm

OBJ = readatfenc.o ../dio96.o

all: readatfenc

readatfenc: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clobber:
	touch x.o
	rm -fr *.o readatfenc

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:47:59 $ $Revision: 1.1.1.1 $ $Name:  $
