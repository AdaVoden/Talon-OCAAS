B = $(TELHOME)/bin

$B/distimages: distimages.pl
	rm -f $B/distimages
	cp distimages.pl $B/distimages

$B/distemail: distemail.pl
	rm -f $B/distemail
	cp distemail.pl $B/distemail

pass5: install

install: $B/distimages $B/distemail

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
