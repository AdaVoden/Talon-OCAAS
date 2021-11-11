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


# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $
