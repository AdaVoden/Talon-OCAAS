# Makefile to install the user support tools

CLDFLAGS = -ffast-math
CFLAGS = $(CLDFLAGS) -Wall -O2 -I../libfits -I../libmisc -I../libastro
LDFLAGS = $(CLDFLAGS) -L../libfits -L../libmisc -L../libastro
LIBS = -lfits -lmisc -lastro -lm

DIR = $(TELHOME)/bin

ALL = boot killp killTel startTel telsecure

pass5:	install

install: $(ALL)
	cd $(DIR); rm -f $(ALL)
	cp $(ALL) $(DIR)
