CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -Wall -ffast-math -I../../libastro -I../../libwcs -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

OBJS= snsearch.o

snsearch: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

pass5: install

install: snsearch
	rm -f $(TELHOME)/bin/snsearch
	cp snsearch $(TELHOME)/bin/snsearch

clobber:
	touch x.o
	rm -fr *.o snsearch
