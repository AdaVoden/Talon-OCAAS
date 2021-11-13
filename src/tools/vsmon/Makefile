CLDFLAGS=
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro  -I../../libwcs -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= vsmon.o

vsmon: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: vsmon
	rm -f $(TELHOME)/bin/vsmon
	cp vsmon $(TELHOME)/bin/vsmon

pass5: install

clobber:
	touch x.o
	rm -f *.o vsmon
